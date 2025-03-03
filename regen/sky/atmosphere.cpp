/*
 * atmosphere.cpp
 *
 *  Created on: Jan 4, 2014
 *      Author: daniel
 */

#include <regen/meshes/primitives/rectangle.h>
#include <regen/states/state-configurer.h>

#include "atmosphere.h"

using namespace regen;

Atmosphere::Atmosphere(
		const ref_ptr<Sky> &sky,
		unsigned int cubeMapSize,
		bool useFloatBuffer,
		unsigned int levelOfDetail)
		: SkyLayer(sky) {
	updateMesh_ = Rectangle::getUnitQuad();

	state()->joinStates(ref_ptr<BlendState>::alloc(GL_SRC_ALPHA, GL_ONE));

	ref_ptr<TextureCube> cubeMap = ref_ptr<TextureCube>::alloc(1);
	cubeMap->begin(RenderState::get());
	cubeMap->set_format(GL_RGBA);
	if (useFloatBuffer) {
		cubeMap->set_internalFormat(GL_RGBA16F);
	} else {
		cubeMap->set_internalFormat(GL_RGBA);
	}
	cubeMap->filter().push(GL_LINEAR);
	cubeMap->set_rectangleSize(cubeMapSize, cubeMapSize);
	cubeMap->wrapping().push(GL_CLAMP_TO_EDGE);
	cubeMap->texImage();
	cubeMap->end(RenderState::get());

	// create render target for updating the sky cube map
	fbo_ = ref_ptr<FBO>::alloc(cubeMapSize, cubeMapSize);
	RenderState::get()->drawFrameBuffer().push(fbo_->id());
	fbo_->drawBuffers().push(DrawBuffers::attachment0());
	// clear negative y to black, -y cube face is not updated
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						   GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, cubeMap->id(), 0);
	glClear(GL_COLOR_BUFFER_BIT);
	// for updating bind all layers to GL_COLOR_ATTACHMENT0
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cubeMap->id(), 0);
	RenderState::get()->drawFrameBuffer().pop();

	drawState_ = ref_ptr<SkyBox>::alloc(levelOfDetail);
	drawState_->setCubeMap(cubeMap);
	//state()->joinStates(drawState_);

	///////
	/// Update Uniforms
	///////
	rayleigh_ = ref_ptr<ShaderInput3f>::alloc("rayleigh");
	rayleigh_->setUniformData(Vec3f(0.0f));
	mie_ = ref_ptr<ShaderInput4f>::alloc("mie");
	mie_->setUniformData(Vec4f(0.0f));
	spotBrightness_ = ref_ptr<ShaderInput1f>::alloc("spotBrightness");
	spotBrightness_->setUniformData(0.0f);
	scatterStrength_ = ref_ptr<ShaderInput1f>::alloc("scatterStrength");
	scatterStrength_->setUniformData(0.0f);
	skyAbsorption_ = ref_ptr<ShaderInput3f>::alloc("skyAbsorption");
	skyAbsorption_->setUniformData(Vec3f(0.0f));
	///////
	/// Update State
	///////
	auto atmosphereUBO = ref_ptr<UniformBlock>::alloc("Atmosphere");
	atmosphereUBO->addUniform(sky->sun()->direction(), "sunDir");
	atmosphereUBO->addUniform(mie_);
	atmosphereUBO->addUniform(rayleigh_);
	atmosphereUBO->addUniform(spotBrightness_);
	atmosphereUBO->addUniform(skyAbsorption_);
	atmosphereUBO->addUniform(scatterStrength_);
	updateState_->joinShaderInput(atmosphereUBO);
	updateShader_ = ref_ptr<ShaderState>::alloc();
	updateState_->joinStates(updateShader_);
	updateState_->joinShaderInput(sky_->worldTime()->in);
}

void Atmosphere::createUpdateShader() {
	StateConfig shaderConfig = StateConfigurer::configure(updateState_.get());
	shaderConfig.setVersion(330);
	updateShader_->createShader(shaderConfig, "regen.weather.atmosphere");
	updateMesh_->updateVAO(RenderState::get(), shaderConfig, updateShader_->shader());
}

void Atmosphere::setRayleighBrightness(float v) {
	auto v_rayleigh = rayleigh_->mapClientVertex<Vec3f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_rayleigh.w = Vec3f(v / 10.0f, v_rayleigh.r.y, v_rayleigh.r.z);
}

void Atmosphere::setRayleighStrength(float v) {
	auto v_rayleigh = rayleigh_->mapClientVertex<Vec3f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_rayleigh.w = Vec3f(v_rayleigh.r.x, v / 1000.0f, v_rayleigh.r.z);
}

void Atmosphere::setRayleighCollect(float v) {
	auto v_rayleigh = rayleigh_->mapClientVertex<Vec3f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_rayleigh.w = Vec3f(v_rayleigh.r.x, v_rayleigh.r.y, v / 100.0f);
}

void Atmosphere::setMieBrightness(float v) {
	auto v_mie = mie_->mapClientVertex<Vec4f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_mie.w = Vec4f(v / 1000.0f, v_mie.r.y, v_mie.r.z, v_mie.r.w);
}

void Atmosphere::setMieStrength(float v) {
	auto v_mie = mie_->mapClientVertex<Vec4f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_mie.w = Vec4f(v_mie.r.x, v / 10000.0f, v_mie.r.z, v_mie.r.w);
}

void Atmosphere::setMieCollect(float v) {
	auto v_mie = mie_->mapClientVertex<Vec4f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_mie.w = Vec4f(v_mie.r.x, v_mie.r.y, v / 100.0f, v_mie.r.w);
}

void Atmosphere::setMieDistribution(float v) {
	auto v_mie = mie_->mapClientVertex<Vec4f>(ShaderData::READ | ShaderData::WRITE, 0);
	v_mie.w = Vec4f(v_mie.r.x, v_mie.r.y, v_mie.r.z, v / 100.0f);
}

void Atmosphere::setSpotBrightness(float v) {
	spotBrightness_->setVertex(0, v);
}

void Atmosphere::setScatterStrength(float v) {
	scatterStrength_->setVertex(0, v / 1000.0f);
}

void Atmosphere::setAbsorption(const Vec3f &color) {
	skyAbsorption_->setVertex(0, color);
}

void Atmosphere::setEarth() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(19.0, 359.0, 81.0);
	prop.mie = Vec4f(44.0, 308.0, 39.0, 74.0);
	prop.spot = 8.0;
	prop.scatterStrength = 54.0;
	prop.absorption = Vec3f(
			0.18867780436772762,
			0.4978442963618773,
			0.6616065586417131);
	setProperties(prop);
}

void Atmosphere::setMars() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(33.0, 139.0, 81.0);
	prop.mie = Vec4f(100.0, 264.0, 39.0, 63.0);
	prop.spot = 1000.0;
	prop.scatterStrength = 28.0;
	prop.absorption = Vec3f(0.66015625, 0.5078125, 0.1953125);
	setProperties(prop);
}

void Atmosphere::setUranus() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(80.0, 136.0, 71.0);
	prop.mie = Vec4f(67.0, 68.0, 0.0, 56.0);
	prop.spot = 0.0;
	prop.scatterStrength = 18.0;
	prop.absorption = Vec3f(0.26953125, 0.5234375, 0.8867187);
	setProperties(prop);
}

void Atmosphere::setVenus() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(25.0, 397.0, 34.0);
	prop.mie = Vec4f(124.0, 298.0, 76.0, 81.0);
	prop.spot = 0.0;
	prop.scatterStrength = 140.0;
	prop.absorption = Vec3f(0.6640625, 0.5703125, 0.29296875);
	setProperties(prop);
}

void Atmosphere::setAlien() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(44.0, 169.0, 71.0);
	prop.mie = Vec4f(60.0, 139.0, 46.0, 86.0);
	prop.spot = 0.0;
	prop.scatterStrength = 26.0;
	prop.absorption = Vec3f(0.24609375, 0.53125, 0.3515625);
	setProperties(prop);
}

void Atmosphere::setProperties(AtmosphereProperties &p) {
	setRayleighBrightness(p.rayleigh.x);
	setRayleighStrength(p.rayleigh.y);
	setRayleighCollect(p.rayleigh.z);
	setMieBrightness(p.mie.x);
	setMieStrength(p.mie.y);
	setMieCollect(p.mie.z);
	setMieDistribution(p.mie.w);
	setSpotBrightness(p.spot);
	setScatterStrength(p.scatterStrength);
	setAbsorption(p.absorption);
}

const ref_ptr<TextureCube> &Atmosphere::cubeMap() const { return drawState_->cubeMap(); }

void Atmosphere::updateSkyLayer(RenderState *rs, GLdouble dt) {
	rs->drawFrameBuffer().push(fbo_->id());
	rs->viewport().push(fbo_->glViewport());

	updateState_->enable(rs);
	updateMesh_->enable(rs);
	updateMesh_->disable(rs);
	updateState_->disable(rs);

	rs->viewport().pop();
	rs->drawFrameBuffer().pop();
}
