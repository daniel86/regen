#include <regen/textures/texture-state.h>
#include "mask-mesh.h"

using namespace regen;

MaskMesh::MaskMesh(
			const ref_ptr<ModelTransformation> &tf,
			const ref_ptr<Texture2D> &maskTexture,
			uint32_t maskIndex,
			const Config &cfg)
		: Rectangle(cfg.quad),
		  maskMeshCfg_(cfg),
		  tf_(tf),
		  maskTexture_(maskTexture),
		  maskIndex_(maskIndex),
		  meshSize_(cfg.meshSize) {
	shaderDefine("VERTEX_MASK_INDEX", REGEN_STRING(maskIndex_));
	if (maskTexture_.get()) {
		maskTextureState_ = ref_ptr<TextureState>::alloc(maskTexture_, "maskTexture");
		maskTextureState_->set_mapTo(TextureState::MAP_TO_VERTEX_MASK);
		maskTextureState_->set_mapping(TextureState::MAPPING_XZ_PLANE);
		joinStates(maskTextureState_);
	}
}

MaskMesh::Config::Config()
		: meshSize(Vec2f::create(10.0f)),
		  height(0.0f) {
}

void MaskMesh::updateMask() {
	unsigned int quadCountX = std::ceil(maskMeshCfg_.meshSize.x / rectangleConfig_.posScale.x);
	unsigned int quadCountY = std::ceil(maskMeshCfg_.meshSize.y / rectangleConfig_.posScale.z);
	Vec2f quadSize_ts = Vec2f(
			rectangleConfig_.posScale.x / maskMeshCfg_.meshSize.x,
			rectangleConfig_.posScale.z / maskMeshCfg_.meshSize.y);
	Vec2f quadHalfSize = Vec2f(rectangleConfig_.posScale.x, rectangleConfig_.posScale.z) * 0.5f;
	std::vector<Vec4f> instanceData(quadCountX * quadCountY);

	unsigned int numInstances = 0;

	if (maskTexture_.get()) {
		maskTexture_->ensureTextureData();
	}

	Vec2f maskUV = quadSize_ts * 0.5f;

	auto baseOffset = tf_->modelOffset()->getVertex(0).r;

	for (unsigned int y = 0; y < quadCountY; ++y) {
		for (unsigned int x = 0; x < quadCountX; ++x) {
			float maskDensity = 0.0f;
			if (!maskTexture_.get()) {
				maskDensity = 1.0f;
			} else if (maskTexture_->format()==GL_RGBA) {
				maskDensity = maskTexture_->sampleMax<Vec4f>(maskUV,
					quadSize_ts, maskTexture_->textureData(), maskIndex_);
			} else if (maskTexture_->format()==GL_RGB) {
				maskDensity = maskTexture_->sampleMax<Vec3f>(maskUV,
					quadSize_ts, maskTexture_->textureData(), maskIndex_);
			} else if (maskTexture_->format()==GL_RG) {
				maskDensity = maskTexture_->sampleMax<Vec2f>(maskUV,
					quadSize_ts, maskTexture_->textureData(), maskIndex_);
			} else {
				maskDensity = maskTexture_->sampleMax<float>(maskUV,
					quadSize_ts, maskTexture_->textureData());
			}
			maskUV.x += quadSize_ts.x;
			if (maskDensity > 0.1) {
				// Note: The quads are no perfect fit, as we use instancing and cannot change the geometry per instance.
				instanceData[numInstances++] = baseOffset + Vec4f(
						static_cast<float>( x ) * rectangleConfig_.posScale.x + quadHalfSize.x - maskMeshCfg_.meshSize.x * 0.5f,
						maskMeshCfg_.height,
						static_cast<float>( y ) * rectangleConfig_.posScale.z + quadHalfSize.y - maskMeshCfg_.meshSize.y * 0.5f,
						0.0f);
			}
		}
		maskUV.x = quadSize_ts.x * 0.5f;
		maskUV.y += quadSize_ts.y;
	}

	if (numInstances==0) {
		REGEN_WARN("No mask instances generated, creating one default instance.");
		numInstances = 1; // at least one instance to avoid issues with zero-sized buffers
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
		tf->setModelMat(0, Mat4f::identity());
		tf->tfBuffer()->updateBuffer();
	}

	MaskMesh::Config meshCfg;
	meshCfg.quad = quadCfg;
	if (input.hasAttribute("height-map")) {
		meshCfg.heightMap = scene->getResource<Texture2D>(input.getValue("height-map"));
	}
	meshCfg.height = input.getValue<float>("height", 0.0f);
	meshCfg.meshSize = input.getValue<Vec2f>("ground-size", Vec2f::create(10.0f));

	ref_ptr<Texture2D> maskTexture;
	uint32_t maskIndex = input.getValue<uint32_t >("mask-index", 0u);
	if (input.hasAttribute("mask")) {
		maskTexture = scene->getResource<Texture2D>(input.getValue("mask"));
	} else if (input.hasAttribute("material-weights")) {
		maskIndex = input.getValue<uint32_t >("material-index", maskIndex);
		uint32_t materialTextureIdx = maskIndex / 4;
		auto materialTextureName = REGEN_STRING(
			input.getValue("material-weights") << "-" << materialTextureIdx);
		maskTexture = scene->getResource<Texture2D>(materialTextureName);
		maskIndex = maskIndex % 4;
	}
	if (maskTexture.get() == nullptr) {
		REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load mask texture.");
		return {};
	}

	auto maskMesh = ref_ptr<MaskMesh>::alloc(tf, maskTexture, maskIndex, meshCfg);
	maskMesh->updateAttributes();
	maskMesh->updateMask();
	return maskMesh;
}
