/*
 * light-pass.cpp
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#include <regen/states/state-configurer.h>
#include <regen/meshes/primitives/box.h>
#include <regen/meshes/primitives/rectangle.h>

#include "light-pass.h"

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
			break;
		case Light::POINT:
			mesh_ = Box::getUnitCube();
			joinStates(ref_ptr<CullFaceState>::alloc(GL_FRONT));
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
		for (auto &in : firstLight.light->inputContainer()->inputs()) {
			if (in.in_->isUniformBlock()) {
				auto *block = dynamic_cast<UniformBlock *>(in.in_.get());
				_cfg.addInput(in.in_->name(), in.in_);
				for (auto &blockUniform : block->uniforms()) {
					if (blockUniform.in_->numInstances()>0) { hasInstancedInputs = true; }
				}
			} else {
				_cfg.addInput(in.in_->name(), in.in_);
				if (in.in_->numInstances()>0) { hasInstancedInputs = true; }
			}
		}
	}
	if (hasInstancedInputs) {
		_cfg.define("HAS_INSTANCES", "TRUE");
	}
	_cfg.define("NUM_SHADOW_LAYER", REGEN_STRING(numShadowLayer_));
	shader_->createShader(_cfg.cfg(), shaderKey_);
	mesh_->updateVAO(RenderState::get(), _cfg.cfg(), shader_->shader());

	for (auto it = lights_.begin(); it != lights_.end(); ++it) { addLightInput(*it); }
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

	// add light uniforms
	switch (lightType_) {
		case Light::DIRECTIONAL:
			addInputLocation(light, light.light->direction(), "lightDirection");
			addInputLocation(light, light.light->diffuse(), "lightDiffuse");
			addInputLocation(light, light.light->specular(), "lightSpecular");
			break;
		case Light::SPOT:
			addInputLocation(light, light.light->position(), "lightPosition");
			addInputLocation(light, light.light->direction(), "lightDirection");
			addInputLocation(light, light.light->radius(), "lightRadius");
			addInputLocation(light, light.light->coneAngle(), "lightConeAngles");
			addInputLocation(light, light.light->diffuse(), "lightDiffuse");
			addInputLocation(light, light.light->specular(), "lightSpecular");
			addInputLocation(light, light.light->coneMatrix(), "modelMatrix");
			break;
		case Light::POINT:
			addInputLocation(light, light.light->position(), "lightPosition");
			addInputLocation(light, light.light->direction(), "lightDirection");
			addInputLocation(light, light.light->radius(), "lightRadius");
			addInputLocation(light, light.light->diffuse(), "lightDiffuse");
			addInputLocation(light, light.light->specular(), "lightSpecular");
			break;
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

	for (auto it = lights_.begin(); it != lights_.end(); ++it) {
		LightPassLight &l = *it;
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
		for (auto jt = l.inputLocations.begin(); jt != l.inputLocations.end(); ++jt) {
			if (lights_.size()>1 || jt->uploadStamp != jt->input->stamp()) {
				jt->input->enableUniform(jt->location);
				jt->uploadStamp = jt->input->stamp();
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
