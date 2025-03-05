/*
 * direct.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <regen/utility/string-util.h>

#include "direct-shading.h"
#include "regen/camera/light-camera-parabolic.h"
#include <regen/textures/texture-3d.h>

using namespace regen;

#define REGEN_LIGHT_NAME(x, id) REGEN_STRING(x << id)

DirectShading::DirectShading() : State(), idCounter_(0) {
	shaderDefine("NUM_LIGHTS", "0");

	ambientLight_ = ref_ptr<ShaderInput3f>::alloc("ambientLight");
	ambientLight_->setUniformData(Vec3f(0.2f));
	joinShaderInput(ambientLight_);
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

void DirectShading::updateDefine(DirectLight &l, GLuint lightIndex) {
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
	GLuint lightID = ++idCounter_;
	GLuint lightIndex = lights_.size();

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
		const ShaderInputList &in = light->inputContainer()->inputs();
		for (auto it = in.begin(); it != in.end(); ++it) {
			if (it->in_->isUniformBlock()) {
				// if the input is a uniform block, we add all uniforms to the shader
				// to avoid name clash.
				// TODO: find a way to use UBO without the name clash
				auto *block = dynamic_cast<UniformBlock *>(it->in_.get());
				for (auto &blockUniform : block->uniforms()) {
					joinShaderInput(
							blockUniform.in_,
							REGEN_LIGHT_NAME(blockUniform.name_, lightID));
				}
			} else {
				joinShaderInput(
						it->in_,
						REGEN_LIGHT_NAME(it->in_->name(), lightID));
			}
		}
	}

	if (camera.get()) {
		joinShaderInput(camera->lightCamera()->far(), REGEN_LIGHT_NAME("lightFar", lightID));
		joinShaderInput(camera->lightCamera()->near(), REGEN_LIGHT_NAME("lightNear", lightID));
		joinShaderInput(camera->lightMatrix(), REGEN_LIGHT_NAME("lightMatrix", lightID));
	}
	if (shadow.get()) {
		joinShaderInput(shadow->sizeInverse(), REGEN_LIGHT_NAME("shadowInverseSize", lightID));
		joinShaderInput(shadow->size(), REGEN_LIGHT_NAME("shadowSize", lightID));
		directLight.shadowMap_ =
				ref_ptr<TextureState>::alloc(shadow, REGEN_LIGHT_NAME("shadowTexture", lightID));
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
		const ShaderInputList &in = l->inputContainer()->inputs();
		for (const auto & jt : in) { disjoinShaderInput(jt.in_); }
	}
	if (directLight.camera_.get()) {
		disjoinShaderInput(directLight.camera_->lightCamera()->far());
		disjoinShaderInput(directLight.camera_->lightCamera()->near());
		disjoinShaderInput(directLight.camera_->lightMatrix());
	}
	if (directLight.shadow_.get()) {
		disjoinShaderInput(directLight.shadow_->sizeInverse());
		disjoinShaderInput(directLight.shadow_->size());
		disjoinStates(directLight.shadowMap_);
		if (directLight.shadowColor_.get()) {
			disjoinStates(directLight.shadowColorMap_);
		}
	}
	lights_.erase(it);

	GLuint numLights = lights_.size(), lightIndex = 0;
	// update shader defines
	shaderDefine("NUM_LIGHTS", REGEN_STRING(numLights));
	for (auto & light : lights_) {
		updateDefine(light, lightIndex);
		++lightIndex;
	}
}

const ref_ptr<ShaderInput3f> &DirectShading::ambientLight() const { return ambientLight_; }
