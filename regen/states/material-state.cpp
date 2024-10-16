/*
 * material.cpp
 *
 *  Created on: 22.03.2011
 *      Author: daniel
 */

#include "texture-state.h"
#include "atomic-states.h"

#include "material-state.h"

using namespace regen;

Material::Material()
		: HasInputState(VBO::USAGE_DYNAMIC),
		  fillMode_(GL_FILL),
		  textures_(0) {
	materialAmbient_ = ref_ptr<ShaderInput3f>::alloc("matAmbient");
	materialAmbient_->setUniformData(Vec3f(0.0f));
	setInput(materialAmbient_);

	materialDiffuse_ = ref_ptr<ShaderInput3f>::alloc("matDiffuse");
	materialDiffuse_->setUniformData(Vec3f(1.0f));
	setInput(materialDiffuse_);

	materialSpecular_ = ref_ptr<ShaderInput3f>::alloc("matSpecular");
	materialSpecular_->setUniformData(Vec3f(0.0f));
	setInput(materialSpecular_);

	materialShininess_ = ref_ptr<ShaderInput1f>::alloc("matShininess");
	materialShininess_->setUniformData(0.0f);
	setInput(materialShininess_);

	materialAlpha_ = ref_ptr<ShaderInput1f>::alloc("matAlpha");
	materialAlpha_->setUniformData(1.0f);
	setInput(materialAlpha_);

	materialRefractionIndex_ = ref_ptr<ShaderInput1f>::alloc("matRefractionIndex");
	materialRefractionIndex_->setUniformData(0.95f);
	setInput(materialRefractionIndex_);

	shaderDefine("HAS_MATERIAL", "TRUE");
}

const ref_ptr<ShaderInput3f> &Material::ambient() const { return materialAmbient_; }

const ref_ptr<ShaderInput3f> &Material::diffuse() const { return materialDiffuse_; }

const ref_ptr<ShaderInput3f> &Material::specular() const { return materialSpecular_; }

const ref_ptr<ShaderInput1f> &Material::shininess() const { return materialShininess_; }

const ref_ptr<ShaderInput1f> &Material::alpha() const { return materialAlpha_; }

const ref_ptr<ShaderInput1f> &Material::refractionIndex() const { return materialRefractionIndex_; }

void Material::set_fillMode(GLenum fillMode) {
	if (fillMode == fillMode_) return;
	disjoinStates(fillModeState_);
	fillMode_ = fillMode;
	fillModeState_ = ref_ptr<FillModeState>::alloc(fillMode_);
	joinStates(fillModeState_);
}

GLenum Material::fillMode() const { return fillMode_; }

void Material::set_twoSided(GLboolean twoSided) {
	if (twoSidedState_.get()) {
		disjoinStates(twoSidedState_);
	}
	twoSidedState_ = ref_ptr<ToggleState>::alloc(RenderState::CULL_FACE, twoSided);
	joinStates(twoSidedState_);
	shaderDefine("HAS_TWO_SIDES", twoSided ? "TRUE" : "FALSE");
}

GLboolean Material::twoSided() const { return twoSidedState_.get() != nullptr; }

void Material::set_jade() {
	materialAmbient_->setUniformData(Vec3f(0.14, 0.22, 0.16));
	materialDiffuse_->setUniformData(Vec3f(0.54, 0.89, 0.63));
	materialSpecular_->setUniformData(Vec3f(0.32, 0.32, 0.32));
	materialShininess_->setUniformData(12.8);
}

void Material::set_ruby() {
	materialAmbient_->setUniformData(Vec3f(0.17, 0.01, 0.01));
	materialDiffuse_->setUniformData(Vec3f(0.61, 0.04, 0.04));
	materialSpecular_->setUniformData(Vec3f(0.73, 0.63, 0.63));
	materialShininess_->setUniformData(76.8);
}

void Material::set_chrome() {
	materialAmbient_->setUniformData(Vec3f(0.25, 0.25, 0.25));
	materialDiffuse_->setUniformData(Vec3f(0.40, 0.40, 0.40));
	materialSpecular_->setUniformData(Vec3f(0.77, 0.77, 0.77));
	materialShininess_->setUniformData(76.8);
}

void Material::set_gold() {
	materialAmbient_->setUniformData(Vec3f(0.25, 0.20, 0.07));
	materialDiffuse_->setUniformData(Vec3f(0.75, 0.61, 0.23));
	materialSpecular_->setUniformData(Vec3f(0.63, 0.65, 0.37));
	materialShininess_->setUniformData(51.2);
}

void Material::set_copper() {
	materialAmbient_->setUniformData(Vec3f(0.19, 0.07, 0.02));
	materialDiffuse_->setUniformData(Vec3f(0.70, 0.27, 0.08));
	specular()->setUniformData(Vec3f(0.26, 0.14, 0.09));
	materialShininess_->setUniformData(12.8);
}

void Material::set_silver() {
	materialAmbient_->setUniformData(Vec3f(0.19, 0.19, 0.19));
	materialDiffuse_->setUniformData(Vec3f(0.51, 0.51, 0.51));
	materialSpecular_->setUniformData(Vec3f(0.51, 0.51, 0.51));
	materialShininess_->setUniformData(51.2);
}

void Material::set_pewter() {
	materialAmbient_->setUniformData(Vec3f(0.11, 0.06, 0.11));
	materialDiffuse_->setUniformData(Vec3f(0.43, 0.47, 0.54));
	materialSpecular_->setUniformData(Vec3f(0.33, 0.33, 0.52));
	materialShininess_->setUniformData(9.8);
}
