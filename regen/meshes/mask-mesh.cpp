#include <regen/textures/texture-state.h>
#include "mask-mesh.h"

using namespace regen;

MaskMesh::MaskMesh(
			const ref_ptr<Texture2D> &maskTexture,
			const ref_ptr<ModelTransformation> &tf,
			const Config &cfg)
		: Rectangle(cfg.quad),
		  maskTexture_(maskTexture),
		  tf_(tf),
		  meshSize_(cfg.meshSize) {
	auto ts = ref_ptr<TextureState>::alloc(maskTexture_, "maskTexture");
	ts->set_mapTo(TextureState::MAP_TO_VERTEX_MASK);
	ts->set_mapping(TextureState::MAPPING_XZ_PLANE);
	joinStates(ts);
	updateAttributes();
	updateMask(cfg);
}

MaskMesh::MaskMesh(const ref_ptr<MaskMesh> &other)
		: Rectangle(other) {
	maskTexture_ = other->maskTexture_;
	tf_ = other->tf_;
	meshSize_ = other->meshSize_;

	auto ts = ref_ptr<TextureState>::alloc(maskTexture_, "maskTexture");
	ts->set_mapTo(TextureState::MAP_TO_VERTEX_MASK);
	ts->set_mapping(TextureState::MAPPING_XZ_PLANE);
	joinStates(ts);
	joinStates(other->tf_);
}

MaskMesh::Config::Config()
		: meshSize(Vec2f(10.0f)),
		  height(0.0f) {
}

void MaskMesh::updateMask(const Config &cfg) {
	unsigned int quadCountX = std::ceil(cfg.meshSize.x / cfg.quad.posScale.x);
	unsigned int quadCountY = std::ceil(cfg.meshSize.y / cfg.quad.posScale.z);
	Vec2f quadSize_ts = Vec2f(
			cfg.quad.posScale.x / cfg.meshSize.x,
			cfg.quad.posScale.z / cfg.meshSize.y);
	Vec2f quadHalfSize = Vec2f(cfg.quad.posScale.x, cfg.quad.posScale.z) * 0.5f;
	std::vector<Vec4f> instanceData(quadCountX * quadCountY);

	unsigned int numInstances = 0;

	maskTexture_->ensureTextureData();
	auto *maskTextureData = maskTexture_->textureData();

	Vec2f maskUV = quadSize_ts * 0.5f;

	auto baseOffset = tf_->modelOffset()->getVertex(0).r;

	for (unsigned int y = 0; y < quadCountY; ++y) {
		for (unsigned int x = 0; x < quadCountX; ++x) {
			auto maskDensity = maskTexture_->sampleMax<float>(
					maskUV,
					quadSize_ts,
					maskTextureData);
			maskUV.x += quadSize_ts.x;
			if (maskDensity > 0.1) {
				// TODO: generate better fitting quads, but geometry cannot be changed as instancing is used
				//          could use scaling instead though to make instances smaller. This might be fine for some cases.
				//auto corrected_x = static_cast<float>(masked.second.min.x + masked.second.max.x) * 0.5f;
				//auto corrected_y = static_cast<float>(masked.second.min.y + masked.second.max.y) * 0.5f;
				instanceData[numInstances++] = baseOffset + Vec4f(
						static_cast<float>( x ) * cfg.quad.posScale.x + quadHalfSize.x - cfg.meshSize.x * 0.5f,
						cfg.height,
						static_cast<float>( y ) * cfg.quad.posScale.z + quadHalfSize.y - cfg.meshSize.y * 0.5f,
						0.0f);
			}
		}
		maskUV.x = quadSize_ts.x * 0.5f;
		maskUV.y += quadSize_ts.y;
	}

	// update the model offset attribute
	instanceData.resize(numInstances);
	tf_->set_numInstances(numInstances);
	tf_->modelOffset()->setInstanceData(numInstances, 1,
		(byte*)instanceData.data());

	disjoinStates(tf_);

	tf_->tfBuffer()->updateBuffer();
	joinStates(tf_);
}

ref_ptr<MaskMesh> MaskMesh::load(LoadingContext &ctx, scene::SceneInputNode &input, const Rectangle::Config &quadCfg) {
	auto scene = ctx.scene();
	ref_ptr<ModelTransformation> tf;
	ref_ptr<State> dummy = ref_ptr<State>::alloc();

	std::vector<ref_ptr<scene::SceneInputNode>> handledChildren;
	for (auto &n: input.getChildren()) {
		if (n->getCategory() == "transform") {
			handledChildren.push_back(n);
			// load the model transformation
			tf = ModelTransformation::load(ctx, *n.get(), dummy);
			scene->putResource("ModelTransformation", n->getValue("id"), tf);
		}
	}
	for (auto &n: handledChildren) {
		// make sure mesh loading does not attempt to load materials again (this will cause a warning)
		input.removeChild(n);
	}
	if (!tf.get()) {
		tf = ref_ptr<ModelTransformation>::alloc();
	}

	MaskMesh::Config meshCfg;
	meshCfg.quad = quadCfg;
	if (input.hasAttribute("height-map")) {
		meshCfg.heightMap = scene->getResource<Texture2D>(input.getValue("height-map"));
	}
	meshCfg.height = input.getValue<float>("height", 0.0f);
	meshCfg.meshSize = input.getValue<Vec2f>("ground-size", Vec2f(10.0f));
	auto maskTexture = scene->getResource<Texture2D>(input.getValue("mask"));
	if (maskTexture.get() == nullptr) {
		REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load mask texture.");
		return {};
	}

	return ref_ptr<MaskMesh>::alloc(maskTexture, tf, meshCfg);
}
