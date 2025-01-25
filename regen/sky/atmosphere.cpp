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
		GLuint cubeMapSize,
		GLboolean useFloatBuffer,
		GLuint levelOfDetail)
		: SkyLayer(sky) {
	ref_ptr<Mesh> updateMesh = Rectangle::getUnitQuad();

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
	skyAbsorbtion_ = ref_ptr<ShaderInput3f>::alloc("skyAbsorbtion");
	skyAbsorbtion_->setUniformData(Vec3f(0.0f));
	///////
	/// Update State
	///////
	updateState_ = ref_ptr<State>::alloc();
	auto atmosphereUBO = ref_ptr<UniformBlock>::alloc("Atmosphere");
	atmosphereUBO->addUniform(sky->sun()->direction(), "sunDir");
	atmosphereUBO->addUniform(mie_);
	atmosphereUBO->addUniform(rayleigh_);
	atmosphereUBO->addUniform(spotBrightness_);
	atmosphereUBO->addUniform(skyAbsorbtion_);
	atmosphereUBO->addUniform(scatterStrength_);
	updateState_->joinShaderInput(atmosphereUBO);
	updateShader_ = ref_ptr<ShaderState>::alloc();
	updateState_->joinStates(updateShader_);
	updateState_->joinStates(updateMesh);
	///////
	/// Update Shader
	///////
	StateConfig shaderConfig = StateConfigurer::configure(updateState_.get());
	shaderConfig.setVersion(330);
	updateShader_->createShader(shaderConfig, "regen.sky.atmosphere");
	updateMesh->updateVAO(RenderState::get(), shaderConfig, updateShader_->shader());
}

void Atmosphere::setRayleighBrightness(GLfloat v) {
	const Vec3f &rayleigh = rayleigh_->getVertex(0);
	rayleigh_->setVertex(0, Vec3f(v / 10.0, rayleigh.y, rayleigh.z));
}

void Atmosphere::setRayleighStrength(GLfloat v) {
	const Vec3f &rayleigh = rayleigh_->getVertex(0);
	rayleigh_->setVertex(0, Vec3f(rayleigh.x, v / 1000.0, rayleigh.z));
}

void Atmosphere::setRayleighCollect(GLfloat v) {
	const Vec3f &rayleigh = rayleigh_->getVertex(0);
	rayleigh_->setVertex(0, Vec3f(rayleigh.x, rayleigh.y, v / 100.0));
}

ref_ptr<ShaderInput3f> &Atmosphere::rayleigh() { return rayleigh_; }

void Atmosphere::setMieBrightness(GLfloat v) {
	const Vec4f &mie = mie_->getVertex(0);
	mie_->setVertex(0, Vec4f(v / 1000.0, mie.y, mie.z, mie.w));
}

void Atmosphere::setMieStrength(GLfloat v) {
	const Vec4f &mie = mie_->getVertex(0);
	mie_->setVertex(0, Vec4f(mie.x, v / 10000.0, mie.z, mie.w));
}

void Atmosphere::setMieCollect(GLfloat v) {
	const Vec4f &mie = mie_->getVertex(0);
	mie_->setVertex(0, Vec4f(mie.x, mie.y, v / 100.0, mie.w));
}

void Atmosphere::setMieDistribution(GLfloat v) {
	const Vec4f &mie = mie_->getVertex(0);
	mie_->setVertex(0, Vec4f(mie.x, mie.y, mie.z, v / 100.0));
}

ref_ptr<ShaderInput4f> &Atmosphere::mie() { return mie_; }

void Atmosphere::setSpotBrightness(GLfloat v) {
	spotBrightness_->setVertex(0, v);
}

ref_ptr<ShaderInput1f> &Atmosphere::spotBrightness() { return spotBrightness_; }

void Atmosphere::setScatterStrength(GLfloat v) {
	scatterStrength_->setVertex(0, v / 1000.0);
}

ref_ptr<ShaderInput1f> &Atmosphere::scatterStrength() { return scatterStrength_; }

void Atmosphere::setAbsorbtion(const Vec3f &color) {
	skyAbsorbtion_->setVertex(0, color);
}

ref_ptr<ShaderInput3f> &Atmosphere::absorbtion() { return skyAbsorbtion_; }

void Atmosphere::setEarth() {
	AtmosphereProperties prop;
	prop.rayleigh = Vec3f(19.0, 359.0, 81.0);
	prop.mie = Vec4f(44.0, 308.0, 39.0, 74.0);
	prop.spot = 373.0;
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
	setAbsorbtion(p.absorption);
}

const ref_ptr<TextureCube> &Atmosphere::cubeMap() const { return drawState_->cubeMap(); }

ref_ptr<Mesh> Atmosphere::getMeshState() { return drawState_; }

ref_ptr<HasShader> Atmosphere::getShaderState() { return drawState_; }

void Atmosphere::updateSkyLayer(RenderState *rs, GLdouble dt) {
	rs->drawFrameBuffer().push(fbo_->id());
	rs->viewport().push(fbo_->glViewport());

	updateState_->enable(rs);
	updateState_->disable(rs);

	rs->viewport().pop();
	rs->drawFrameBuffer().pop();
}
