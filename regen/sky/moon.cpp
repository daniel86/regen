/*
 * moon.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "moon.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/state-configurer.h>
#include <regen/meshes/rectangle.h>
#include <regen/textures/texture-loader.h>

using namespace regen;


MoonLayer::MoonLayer(const ref_ptr<Sky> &sky, const std::string &moonMapFile)
		: SkyLayer(sky) {
	setupMoonTextureCube(moonMapFile);

	moonOrientation_ = ref_ptr<ShaderInputMat4>::alloc("moonOrientationMatrix");
	moonOrientation_->setUniformData(Mat4f::identity());
	state()->joinShaderInput(moonOrientation_);

	scale_ = ref_ptr<ShaderInput1f>::alloc("scale");
	scale_->setUniformData(defaultScale());
	state()->joinShaderInput(scale_);

	scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
	scattering_->setUniformData(defaultScattering());
	state()->joinShaderInput(scattering_);

	sunShine_ = ref_ptr<ShaderInput4f>::alloc("sunShine");
	sunShine_->setUniformData(Vec4f(defaultSunShineColor(), defaultSunShineIntensity()));
	state()->joinShaderInput(sunShine_);

	earthShine_ = ref_ptr<ShaderInput3f>::alloc("earthShine");
	earthShine_->setUniformData(Vec3f(0.0));
	state()->joinShaderInput(earthShine_);
	earthShineColor_ = defaultEarthShineColor();
	earthShineIntensity_ = defaultEarthShineIntensity();

	shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.moon");
	meshState_ = ref_ptr<Rectangle>::alloc(sky->skyQuad());
}

void MoonLayer::setupMoonTextureCube(const std::string &moonMapFile) {
	ref_ptr<TextureCube> texture = textures::loadCube(moonMapFile);
	state()->joinStates(ref_ptr<TextureState>::alloc(texture, "moonmapCube"));
}


GLdouble MoonLayer::defaultScale() { return 0.1; }

GLdouble MoonLayer::defaultScattering() { return 4.0; }

Vec3f MoonLayer::defaultSunShineColor() { return {0.923, 0.786, 0.636}; }

GLdouble MoonLayer::defaultSunShineIntensity() { return 128.0; }

Vec3f MoonLayer::defaultEarthShineColor() { return {0.88, 0.96, 1.0}; }

GLdouble MoonLayer::defaultEarthShineIntensity() { return 4.0; }


void MoonLayer::set_scale(GLdouble scale) { scale_->setVertex(0, scale); }

const ref_ptr<ShaderInput1f> &MoonLayer::scale() const { return scale_; }

void MoonLayer::set_scattering(GLdouble scattering) { scattering_->setVertex(0, scattering); }

const ref_ptr<ShaderInput1f> &MoonLayer::scattering() const { return scattering_; }

const ref_ptr<ShaderInput4f> &MoonLayer::sunShine() const { return sunShine_; }

void MoonLayer::set_sunShineColor(const Vec3f &color) {
	GLfloat intensity = sunShine_->getVertexPtr(0).w;
	sunShine_->setVertex(0, Vec4f(color, intensity));
}

void MoonLayer::set_sunShineIntensity(GLdouble intensity) {
	Vec3f &color = sunShine_->getVertexPtr(0).xyz_();
	sunShine_->setVertex(0, Vec4f(color, intensity));
}

const ref_ptr<ShaderInput3f> &MoonLayer::earthShine() const { return earthShine_; }

void MoonLayer::set_earthShineColor(const Vec3f &color) { earthShineColor_ = color; }

void MoonLayer::set_earthShineIntensity(GLdouble intensity) { earthShineIntensity_ = intensity; }

ref_ptr<Mesh> MoonLayer::getMeshState() { return meshState_; }

ref_ptr<HasShader> MoonLayer::getShaderState() { return shaderState_; }

void MoonLayer::updateSkyLayer(RenderState *rs, GLdouble dt) {
	moonOrientation_->setVertex(0, sky_->astro().getMoonOrientation());
	earthShine_->setVertex(0, earthShineColor_ *
							  (sky_->astro().getEarthShineIntensity() * earthShineIntensity_));
}

