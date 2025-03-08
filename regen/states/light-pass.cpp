/*
 * light-pass.cpp
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#include <regen/states/state-configurer.h>
#include <regen/meshes/primitives/box.h>
#include <regen/meshes/primitives/rectangle.h>
#include <regen/textures/texture-3d.h>

#include "light-pass.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

LightPass::LightPass(Light::Type type, const std::string &shaderKey)
		: State(), lightType_(type), shaderKey_(shaderKey) {
	switch (lightType_) {
		case Light::DIRECTIONAL:
			mesh_ = Rectangle::getUnitQuad();
			break;
		case Light::SPOT:
			mesh_ = ConeClosed::getBaseCone();
			joinStates(ref_ptr<CullFaceState>::alloc(GL_FRONT));
			// enable depth clamping to avoid faces to be clipped away when out of depth range.
			joinStates(ref_ptr<ToggleState>::alloc(RenderState::DEPTH_CLAMP, GL_TRUE));
			break;
		case Light::POINT:
			mesh_ = Box::getUnitCube();
			joinStates(ref_ptr<CullFaceState>::alloc(GL_FRONT));
			// enable depth clamping to avoid faces to be clipped away when out of depth range.
			joinStates(ref_ptr<ToggleState>::alloc(RenderState::DEPTH_CLAMP, GL_TRUE));
			break;
	}
	shadowFiltering_ = SHADOW_FILTERING_NONE;
	numShadowLayer_ = 1;

	shader_ = ref_ptr<ShaderState>::alloc();
	joinStates(shader_);
	setShadowFiltering(SHADOW_FILTERING_NONE);
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

void LightPass::setShadowFiltering(ShadowFilterMode mode) {
	shadowFiltering_ = mode;
	shaderDefine("SHADOW_MAP_FILTER", shadowFilterMode(mode));
}

void LightPass::addLight(
		const ref_ptr<Light> &l,
		const ref_ptr<LightCamera> &lightCamera,
		const ref_ptr<Texture> &shadowTexture,
		const ref_ptr<Texture> &shadowColorTexture,
		const std::list<ref_ptr<ShaderInput> > &inputs) {
	LightPassLight light;
	lights_.push_back(light);

	auto it = lights_.end();
	--it;
	lightIterators_[l.get()] = it;

	it->light = l;
	it->camera = lightCamera;
	it->shadow = shadowTexture;
	it->shadowColor = shadowColorTexture;
	it->inputs = inputs;
	if (shadowTexture.get()) {
		auto *tex3D = dynamic_cast<Texture3D *>(shadowTexture.get());
		if (tex3D == nullptr) {
			numShadowLayer_ = 1;
		} else {
			numShadowLayer_ = tex3D->depth();
		}
		shaderDefine("USE_SHADOW_MAP", "TRUE");
		if (shadowColorTexture.get()) {
			shaderDefine("USE_SHADOW_COLOR", "TRUE");
		}
	}

	if (shader_->shader().get()) { addLightInput(*it); }
}

void LightPass::removeLight(Light *l) {
	lights_.erase(lightIterators_[l]);
}

GLboolean LightPass::empty() const {
	return lights_.empty();
}

GLboolean LightPass::hasLight(Light *l) const {
	return lightIterators_.count(l) > 0;
}

void LightPass::createShader(const StateConfig &cfg) {
	StateConfigurer _cfg(cfg);
	_cfg.addState(this);
	_cfg.addState(mesh_.get());
	bool hasInstancedInputs = false;
	if (!lights_.empty()) {
		// add first light to shader to set up shader defines and also instance count
		auto &firstLight = lights_.front();
		for (auto &in: firstLight.light->inputContainer()->inputs()) {
			if (in.in_->isUniformBlock()) {
				auto *block = dynamic_cast<UniformBlock *>(in.in_.get());
				_cfg.addInput(in.in_->name(), in.in_);
				for (auto &blockUniform: block->uniforms()) {
					if (blockUniform.in_->numInstances() > 0) { hasInstancedInputs = true; }
				}
			} else {
				_cfg.addInput(in.in_->name(), in.in_);
				if (in.in_->numInstances() > 0) { hasInstancedInputs = true; }
			}
		}
	}
	if (hasInstancedInputs) {
		_cfg.define("HAS_INSTANCES", "TRUE");
	}
	_cfg.define("NUM_SHADOW_LAYER", REGEN_STRING(numShadowLayer_));
	shader_->createShader(_cfg.cfg(), shaderKey_);
	mesh_->updateVAO(RenderState::get(), _cfg.cfg(), shader_->shader());

	for (auto &light: lights_) { addLightInput(light); }
	shadowMapLoc_ = shader_->shader()->uniformLocation("shadowTexture");
	shadowColorLoc_ = shader_->shader()->uniformLocation("shadowColorTexture");
}

void LightPass::addLightInput(LightPassLight &light) {
	// clear list of uniform loactions
	light.inputLocations.clear();
	// add user specified uniforms
	for (auto jt = light.inputs.begin(); jt != light.inputs.end(); ++jt) {
		ref_ptr<ShaderInput> &in = *jt;
		addInputLocation(light, in, in->name());
	}

	// add shadow uniforms
	if (light.camera.get()) {
		addInputLocation(light, light.camera->lightCamera()->far(), "lightFar");
		addInputLocation(light, light.camera->lightCamera()->near(), "lightNear");
		addInputLocation(light, light.camera->lightMatrix(), "lightMatrix");
	}
	if (light.shadow.get()) {
		addInputLocation(light, light.shadow->sizeInverse(), "shadowInverseSize");
		addInputLocation(light, light.shadow->size(), "shadowSize");
	}

	// add light UBO, and special light type uniforms
	addInputLocation(light, light.light->lightUBO(), "Light");
	if (lightType_ == Light::SPOT) {
		addInputLocation(light, light.light->coneMatrix(), "modelMatrix");
	}
}

void LightPass::addInputLocation(LightPassLight &l,
								 const ref_ptr<ShaderInput> &in, const std::string &name) {
	Shader *s = shader_->shader().get();
	GLint loc = s->uniformLocation(name);
	if (loc > 0) {
		l.inputLocations.emplace_back(in, loc);
	}
}

void LightPass::enable(RenderState *rs) {
	State::enable(rs);
	auto smChannel = rs->reserveTextureChannel();
	auto smColorChannel = rs->reserveTextureChannel();

	for (auto &l: lights_) {
		ref_ptr<Texture> shadowTex;
		// activate shadow map if specified
		if (l.shadow.get()) {
			l.shadow->begin(rs, smChannel);
			glUniform1i(shadowMapLoc_, smChannel);
		}
		if (l.shadowColor.get()) {
			l.shadowColor->begin(rs, smColorChannel);
			glUniform1i(shadowColorLoc_, smColorChannel);
		}
		// enable light pass uniforms
		for (auto &inputLocation: l.inputLocations) {
			if (lights_.size() > 1 || inputLocation.uploadStamp != inputLocation.input->stamp()) {
				inputLocation.input->enableUniform(inputLocation.location);
				inputLocation.uploadStamp = inputLocation.input->stamp();
			}
		}

		mesh_->enable(rs);
		mesh_->disable(rs);

		if (l.shadow.get()) { l.shadow->end(rs, smChannel); }
		if (l.shadowColor.get()) { l.shadowColor->end(rs, smColorChannel); }
	}

	rs->releaseTextureChannel();
	rs->releaseTextureChannel();
}

ref_ptr<LightPass> LightPass::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	if (!input.hasAttribute("shader")) {
		REGEN_WARN("Missing shader attribute for " << input.getDescription() << ".");
		return {};
	}
	ref_ptr<LightPass> x = ref_ptr<LightPass>::alloc(
			input.getValue<Light::Type>("type", Light::SPOT),
			input.getValue("shader"));

	x->setShadowFiltering(input.getValue<ShadowFilterMode>(
			"shadow-filter", SHADOW_FILTERING_NONE));
	bool useShadows = false, toggle = true;

	for (auto &n: input.getChildren()) {

		ref_ptr<Light> light = scene->getResource<Light>(n->getName());
		if (light.get() == nullptr) {
			continue;
		}
		std::list<ref_ptr<ShaderInput> > inputs;

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
		if ((n->hasAttribute("shadow-buffer") && n->hasAttribute("shadow-color-attachment")) ||
			n->hasAttribute("shadow-color-texture")) {
			shadowColorMap = TextureState::getTexture(scene, *n.get(),
													  "shadow-color-texture", "shadow-buffer",
													  "shadow-color-attachment");
		}
		if (toggle) {
			useShadows = (shadowMap.get() != nullptr);
			toggle = false;
		} else if ((shadowMap.get() != nullptr) != useShadows) {
			REGEN_WARN("Ignoring " << input.getDescription() << ".");
			continue;
		}

		// Each light pass can have a set of ShaderInput's
		for (auto &m: n->getChildren()) {
			if (m->getCategory() == "input") {
				inputs.push_back(scene::ShaderInputProcessor::createShaderInput(scene, *m.get(), x));
			} else {
				REGEN_WARN("Unhandled node " << m->getDescription() << ".");
			}
		}

		x->addLight(light, shadowCamera, shadowMap, shadowColorMap, inputs);
	}

	StateConfigurer shaderConfigurer;
	shaderConfigurer.addNode(ctx.parent().get());
	shaderConfigurer.addState(x.get());
	x->createShader(shaderConfigurer.cfg());

	return x;
}
