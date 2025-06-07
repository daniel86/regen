#include "moon.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/textures/texture-loader.h>

using namespace regen;

MoonLayer::MoonLayer(const ref_ptr<Sky> &sky, const std::string &moonMapFile)
		: SkyLayer(sky) {
	setupMoonTextureCube(moonMapFile);

	moonUniforms_ = ref_ptr<UBO>::alloc("Moon");
	state()->joinShaderInput(moonUniforms_);

	moonOrientation_ = ref_ptr<ShaderInputMat4>::alloc("moonOrientationMatrix");
	moonOrientation_->setUniformData(Mat4f::identity());
	moonUniforms_->addBlockInput(moonOrientation_);

	sunShine_ = ref_ptr<ShaderInput4f>::alloc("sunShine");
	sunShine_->setUniformData(Vec4f(defaultSunShineColor(), defaultSunShineIntensity()));
	moonUniforms_->addBlockInput(sunShine_);

	earthShine_ = ref_ptr<ShaderInput3f>::alloc("earthShine");
	earthShine_->setUniformData(Vec3f(0.0));
	moonUniforms_->addBlockInput(earthShine_);
	earthShineColor_ = defaultEarthShineColor();
	earthShineIntensity_ = defaultEarthShineIntensity();

	scale_ = ref_ptr<ShaderInput1f>::alloc("scale");
	scale_->setUniformData(defaultScale());
	moonUniforms_->addBlockInput(scale_);

	scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
	scattering_->setUniformData(defaultScattering());
	moonUniforms_->addBlockInput(scattering_);

	shaderState_ = ref_ptr<HasShader>::alloc("regen.weather.moon");
	meshState_ = ref_ptr<Rectangle>::alloc(sky->skyQuad());
}

void MoonLayer::setupMoonTextureCube(const std::string &moonMapFile) {
	ref_ptr<TextureCube> texture = textures::loadCube(moonMapFile);
	state()->joinStates(ref_ptr<TextureState>::alloc(texture, "moonmapCube"));
}

float MoonLayer::defaultScale() {
	return 0.1;
}

float MoonLayer::defaultScattering() {
	return 4.0;
}

Vec3f MoonLayer::defaultSunShineColor() {
	return {0.923, 0.786, 0.636};
}

float MoonLayer::defaultSunShineIntensity() {
	return 128.0;
}

Vec3f MoonLayer::defaultEarthShineColor() {
	return {0.88, 0.96, 1.0};
}

float MoonLayer::defaultEarthShineIntensity() {
	return 4.0;
}

void MoonLayer::set_sunShineColor(const Vec3f &color) {
	auto v_sunShine = sunShine_->mapClientVertex<Vec4f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_sunShine.w = Vec4f(color, v_sunShine.r.w);
}

void MoonLayer::set_sunShineIntensity(float intensity) {
	auto v_color = sunShine_->mapClientVertex<Vec4f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_color.w = Vec4f(v_color.r.xyz_(), intensity);
}

void MoonLayer::updateSkyLayer(RenderState *rs, GLdouble dt) {
	moonOrientation_->setVertex(0, sky_->astro().getMoonOrientation());
	earthShine_->setVertex(0, earthShineColor_ *
							  (sky_->astro().getEarthShineIntensity() * earthShineIntensity_));
}
