/*
 * direct.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <regen/utility/string-util.h>

#include "direct-shading.h"

using namespace regen;

#define __NAME__(x, id) REGEN_STRING(x << id)

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
			__NAME__("LIGHT_IS_ATTENUATED", l.id_),
			l.light_->isAttenuated() ? "TRUE" : "FALSE");

	std::string lightType = "UNKNOWN";
	switch (l.light_->lightType()) {
		case Light::DIRECTIONAL:
			lightType = "DIRECTIONAL";
			break;
		case Light::POINT:
			lightType = "POINT";
			break;
		case Light::SPOT:
			lightType = "SPOT";
			break;
	}
	shaderDefine(__NAME__("LIGHT_TYPE", l.id_), lightType);

	// handle shadow map defined
	if (l.shadow_.get()) {
		shaderDefine(__NAME__("USE_SHADOW_MAP", l.id_), "TRUE");
		shaderDefine(__NAME__("SHADOW_MAP_FILTER", l.id_), shadowFilterMode(l.shadowFilter_));

		if (dynamic_cast<Texture3D *>(l.shadow_.get())) {
			Texture3D *tex3d = dynamic_cast<Texture3D *>(l.shadow_.get());
			shaderDefine(__NAME__("NUM_SHADOW_LAYER", l.id_), REGEN_STRING(tex3d->depth()));
		}
		if (l.shadowColor_.get()) {
			shaderDefine(__NAME__("USE_SHADOW_COLOR", l.id_), "TRUE");
		}
	} else {
		shaderDefine(__NAME__("USE_SHADOW_MAP", l.id_), "FALSE");
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
		for (ShaderInputList::const_iterator it = in.begin(); it != in.end(); ++it) { joinShaderInput(it->in_, __NAME__(
					it->in_->name(), lightID));
		}
	}

	if (camera.get()) {
		joinShaderInput(camera->far(), __NAME__("lightFar", lightID));
		joinShaderInput(camera->near(), __NAME__("lightNear", lightID));
		joinShaderInput(camera->lightMatrix(), __NAME__("lightMatrix", lightID));
	}
	if (shadow.get()) {
		joinShaderInput(shadow->sizeInverse(), __NAME__("shadowInverseSize", lightID));
		joinShaderInput(shadow->size(), __NAME__("shadowSize", lightID));
		directLight.shadowMap_ =
				ref_ptr<TextureState>::alloc(shadow, __NAME__("shadowTexture", lightID));
		joinStates(directLight.shadowMap_);
		if (shadowColor.get()) {
			directLight.shadowColorMap_ =
					ref_ptr<TextureState>::alloc(shadow, __NAME__("shadowColorTexture", lightID));
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
		for (ShaderInputList::const_iterator it = in.begin(); it != in.end(); ++it) { disjoinShaderInput(it->in_); }
	}
	if (directLight.camera_.get()) {
		disjoinShaderInput(directLight.camera_->far());
		disjoinShaderInput(directLight.camera_->near());
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
	for (std::list<DirectLight>::iterator it = lights_.begin(); it != lights_.end(); ++it) {
		updateDefine(*it, lightIndex);
		++lightIndex;
	}
}

const ref_ptr<ShaderInput3f> &DirectShading::ambientLight() const { return ambientLight_; }
