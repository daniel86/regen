#include "grass-patch.h"
#include "regen/textures/texture-loader.h"
#include "regen/states/depth-state.h"
#include "regen/meshes/mesh-vector.h"

using namespace regen;

GrassPatch::GrassPatch(
				const ref_ptr<ModelTransformation> &tf,
				const ref_ptr<Texture2D> &maskTexture,
				uint32_t maskIndex,
				const Config &cfg) :
		MaskMesh(tf, maskTexture, maskIndex, cfg) {
}

ref_ptr<GrassPatch> GrassPatch::load(
	LoadingContext &ctx,
	scene::SceneInputNode &input,
	const Rectangle::Config &quadCfg) {
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

	return ref_ptr<GrassPatch>::alloc(tf, maskTexture, maskIndex, meshCfg);
}
