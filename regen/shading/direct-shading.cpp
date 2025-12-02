#include <regen/utility/string-util.h>

#include "direct-shading.h"
#include "regen/camera/light-camera-parabolic.h"
#include "regen/scene/node-processor.h"

using namespace regen;

#define REGEN_LIGHT_NAME(x, id) REGEN_STRING(x << id)

DirectShading::DirectShading() : State(), idCounter_(0) {
	shaderDefine("NUM_LIGHTS", "0");

	ambientLight_ = ref_ptr<ShaderInput3f>::alloc("ambientLight");
	ambientLight_->setUniformData(Vec3f::create(0.2f));
	ambientLight_->setSchema(InputSchema::color());
	setInput(ambientLight_);
}

static std::string shadowFilterMode(ShadowFilterMode f) {
	switch (f) {
		case SHADOW_FILTERING_NONE:
			return "Single";
		case SHADOW_FILTERING_PCF_GAUSSIAN:
			return "Gaussian";
		case SHADOW_FILTERING_VSM:
			return "VSM";
	}
	return "Single";
}

void DirectShading::updateDefine(DirectLight &l, uint32_t lightIndex) {
	shaderDefine(
			REGEN_STRING("LIGHT" << lightIndex << "_ID"),
			REGEN_STRING(l.id_));
	shaderDefine(
			REGEN_LIGHT_NAME("LIGHT_IS_ATTENUATED", l.id_),
			l.light_->isAttenuated() ? "TRUE" : "FALSE");

	std::string lightType = "UNKNOWN";
	switch (l.light_->lightType()) {
		case Light::DIRECTIONAL:
			lightType = "DIRECTIONAL";
			break;
		case Light::POINT:
			lightType = "POINT";
			// define POINT_LIGHT_TYPE={CUBE, PARABOLIC} based on type of light camera
			if (l.camera_.get()) {
				auto *parabolic = dynamic_cast<ParabolicCamera *>(l.camera_.get());
				if (parabolic) {
					shaderDefine(REGEN_LIGHT_NAME("POINT_LIGHT_TYPE", l.id_), "PARABOLIC");
				} else {
					shaderDefine(REGEN_LIGHT_NAME("POINT_LIGHT_TYPE", l.id_), "CUBE");
				}
			}
			break;
		case Light::SPOT:
			lightType = "SPOT";
			break;
	}
	shaderDefine(REGEN_LIGHT_NAME("LIGHT_TYPE", l.id_), lightType);

	// handle shadow map defined
	if (l.shadow_.get()) {
		shaderDefine(REGEN_LIGHT_NAME("USE_SHADOW_MAP", l.id_), "TRUE");
		shaderDefine(REGEN_LIGHT_NAME("SHADOW_MAP_FILTER", l.id_), shadowFilterMode(l.shadowFilter_));

		if (dynamic_cast<Texture3D *>(l.shadow_.get())) {
			auto *tex3d = dynamic_cast<Texture3D *>(l.shadow_.get());
			shaderDefine(REGEN_LIGHT_NAME("NUM_SHADOW_LAYER", l.id_), REGEN_STRING(tex3d->depth()));
		}
		if (l.shadowColor_.get()) {
			shaderDefine(REGEN_LIGHT_NAME("USE_SHADOW_COLOR", l.id_), "TRUE");
		}
	} else {
		shaderDefine(REGEN_LIGHT_NAME("USE_SHADOW_MAP", l.id_), "FALSE");
	}
}

void DirectShading::addLight(const ref_ptr<Light> &light) {
	addLight(light, ref_ptr<LightCamera>(),
			 ref_ptr<Texture>(), ref_ptr<Texture>(), SHADOW_FILTERING_NONE);
}

void DirectShading::addLight(
		const ref_ptr<Light> &light,
		const ref_ptr<LightCamera> &camera,
		const ref_ptr<Texture> &shadow,
		const ref_ptr<Texture> &shadowColor,
		ShadowFilterMode shadowFilter) {
	uint32_t lightID = ++idCounter_;
	uint32_t lightIndex = lights_.size();

	{
		DirectLight dl;
		dl.id_ = lightID;
		dl.light_ = light;
		dl.camera_ = camera;
		dl.shadow_ = shadow;
		dl.shadowColor_ = shadowColor;
		dl.shadowFilter_ = shadowFilter;
		lights_.push_back(dl);
	}
	DirectLight &directLight = *lights_.rbegin();
	// remember the number of lights used
	shaderDefine("NUM_LIGHTS", REGEN_STRING(lightIndex + 1));
	updateDefine(directLight, lightIndex);

	// join light shader inputs using a name override
	{
		for (auto &it : light->inputs()) {
			if (it.in_->isBufferBlock()) {
				// if the input is a uniform block, we add all uniforms to the shader
				// to avoid name clash.
				auto *block = dynamic_cast<BufferBlock *>(it.in_.get());
				auto *ubo = dynamic_cast<UBO *>(block);
				if (ubo) {
					setInput(it.in_,
						REGEN_LIGHT_NAME(block->name(), lightID),
						REGEN_STRING(lightID));
				} else {
					REGEN_WARN("Unexpected input type for light: "
							   << it.in_->name() << " (" << block->name() << ")");
					for (auto &blockUniform: block->stagedInputs()) {
						setInput(
								blockUniform.in_,
								REGEN_LIGHT_NAME(blockUniform.name_, lightID));
					}
				}
			} else {
				setInput(
						it.in_,
						REGEN_LIGHT_NAME(it.in_->name(), lightID));
			}
		}
	}

	if (camera.get()) {
		setInput(camera->lightCamera()->sh_projParams(), REGEN_LIGHT_NAME("lightProjParams", lightID));
		setInput(camera->shadowBuffer(), "Shadow", REGEN_STRING(lightID));
	}
	if (shadow.get()) {
		directLight.shadowSizeInv_ = createUniform<ShaderInput2f>(
			REGEN_LIGHT_NAME("shadowInverseSize", lightID),
			shadow->sizeInverse());
		directLight.shadowSize_ = createUniform<ShaderInput2f>(
			REGEN_LIGHT_NAME("shadowSize", lightID),
			shadow->size());
		setInput(directLight.shadowSizeInv_);
		setInput(directLight.shadowSize_);

		directLight.shadowMap_ =
				ref_ptr<TextureState>::alloc(shadow, REGEN_LIGHT_NAME("shadowTexture", lightID));
		directLight.shadowMap_->set_mapping(TextureState::MAPPING_CUSTOM);
		directLight.shadowMap_->set_mapTo(TextureState::MAP_TO_CUSTOM);
		joinStates(directLight.shadowMap_);
		if (shadowColor.get()) {
			directLight.shadowColorMap_ =
					ref_ptr<TextureState>::alloc(shadow, REGEN_LIGHT_NAME("shadowColorTexture", lightID));
			joinStates(directLight.shadowColorMap_);
		}
	}
}

void DirectShading::removeLight(const ref_ptr<Light> &l) {
	std::list<DirectLight>::iterator it;
	for (it = lights_.begin(); it != lights_.end(); ++it) {
		ref_ptr<Light> &x = it->light_;
		if (x.get() == l.get()) {
			break;
		}
	}
	if (it == lights_.end()) { return; }

	DirectLight &directLight = *it;
	{
		for (const auto &jt: l->inputs()) { removeInput(jt.in_); }
	}
	if (directLight.camera_.get()) {
		removeInput(directLight.camera_->lightCamera()->sh_projParams());
		removeInput(directLight.camera_->sh_lightMatrix());
	}
	if (directLight.shadow_.get()) {
		removeInput(directLight.shadowSizeInv_);
		removeInput(directLight.shadowSize_);
		disjoinStates(directLight.shadowMap_);
		if (directLight.shadowColor_.get()) {
			disjoinStates(directLight.shadowColorMap_);
		}
	}
	lights_.erase(it);

	uint32_t numLights = lights_.size(), lightIndex = 0;
	// update shader defines
	shaderDefine("NUM_LIGHTS", REGEN_STRING(numLights));
	for (auto &light: lights_) {
		updateDefine(light, lightIndex);
		++lightIndex;
	}
}

const ref_ptr<ShaderInput3f> &DirectShading::ambientLight() const { return ambientLight_; }

ref_ptr<DirectShading> DirectShading::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	ref_ptr<scene::SceneInputNode> lightNode = input.getFirstChild("direct-lights");
	ref_ptr<scene::SceneInputNode> passNode = input.getFirstChild("direct-pass");
	if (lightNode.get() == nullptr) {
		REGEN_WARN("Missing direct-lights attribute for " << input.getDescription() << ".");
		return {};
	}
	if (passNode.get() == nullptr) {
		REGEN_WARN("Missing direct-pass attribute for " << input.getDescription() << ".");
		return {};
	}

	ref_ptr<DirectShading> shadingState = ref_ptr<DirectShading>::alloc();
	ref_ptr<StateNode> shadingNode = ref_ptr<StateNode>::alloc(shadingState);
	ctx.parent()->addChild(shadingNode);

	if (input.hasAttribute("ambient")) {
		shadingState->ambientLight()->setVertex(0,
												input.getValue<Vec3f>("ambient", Vec3f::create(0.1f)));
	}

	// load lights
	for (auto &n: lightNode->getChildren()) {
		if (n->getCategory() != "light") {
			REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
			continue;
		}

		ref_ptr<Light> light = scene->getResource<Light>(n->getName());
		if (light.get() == nullptr) {
			REGEN_WARN("Unable to find Light for '" << n->getDescription() << "'.");
			continue;
		}

		auto shadowFiltering =
				n->getValue<ShadowFilterMode>("shadow-filter", SHADOW_FILTERING_NONE);
		ref_ptr<Texture> shadowMap;
		ref_ptr<Texture> shadowColorMap;
		ref_ptr<LightCamera> shadowCamera;
		if (n->hasAttribute("shadow-camera")) {
			shadowCamera = ref_ptr<LightCamera>::dynamicCast(
					scene->getResource<Camera>(n->getValue("shadow-camera")));
			if (shadowCamera.get() == nullptr) {
				REGEN_WARN("Unable to find LightCamera for '" << n->getDescription() << "'.");
			}
		}
		if (n->hasAttribute("shadow-buffer") || n->hasAttribute("shadow-texture")) {
			shadowMap = TextureState::getTexture(scene, *n.get(),
												 "shadow-texture", "shadow-buffer",
												 "shadow-attachment");
			if (shadowMap.get() == nullptr) {
				REGEN_WARN("Unable to find ShadowMap for '" << n->getDescription() << "'.");
			}
		}
		if (n->hasAttribute("shadow-color-attachment") || n->hasAttribute("shadow-color-texture")) {
			shadowColorMap = TextureState::getTexture(scene, *n.get(),
													  "shadow-color-texture", "shadow-buffer",
													  "shadow-color-attachment");
			if (shadowMap.get() == nullptr) {
				REGEN_WARN("Unable to find Shadow-Color-Map for '" << n->getDescription() << "'.");
			}
		}
		shadingState->addLight(light, shadowCamera, shadowMap, shadowColorMap, shadowFiltering);
	}

	// parse passNode
	scene::SceneNodeProcessor().processInput(scene, *passNode.get(), shadingNode);

	return shadingState;
}
