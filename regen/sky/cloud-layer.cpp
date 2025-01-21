/*
 * cloud-layer.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "cloud-layer.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>
#include <regen/meshes/rectangle.h>

using namespace regen;

#define _randf(min, max) \
    (static_cast<float>(rand()) / RAND_MAX * ((max) - (min)) + (min))

static GLfloat *createNoiseSlice(GLuint texSize, GLuint octave) {
	GLuint size2 = texSize * texSize;
	GLfloat oneOverTexSize = 1.f / static_cast<GLfloat>(texSize);

	osgHimmel::Noise n(1 << (octave + 2), _randf(0.f, 1.f), _randf(0.f, 1.f));

	auto *noise = new float[size2];
	GLuint o;
	for (GLuint s = 0; s < texSize; ++s)
		for (GLuint t = 0; t < texSize; ++t) {
			o = t * texSize + s;
			noise[o] = n.noise2(s * oneOverTexSize, t * oneOverTexSize, octave) * 0.5 + 0.5;
		}

	return noise;
}

static ref_ptr<Texture3D> createNoiseArray(GLuint texSize, GLuint octave, GLuint slices) {
	RenderState *rs = RenderState::get();

	ref_ptr<Texture3D> tex = ref_ptr<Texture3D>::alloc();
	//ref_ptr<Texture2DArray> tex = ref_ptr<Texture2DArray>::alloc();
	tex->begin(rs);
	{
		tex->set_rectangleSize(texSize, texSize);
		tex->set_depth(slices);
		tex->set_format(GL_RED);
		tex->set_internalFormat(GL_R16F);
		tex->set_pixelType(GL_FLOAT);

		tex->texImage();
		for (unsigned int s = 0; s < slices; ++s) {
			GLfloat *data = createNoiseSlice(texSize, octave);
			tex->texSubImage(s, (GLubyte *) data);
			delete[]data;
		}

		tex->filter().push(GL_LINEAR);
		tex->wrapping().push(GL_REPEAT);
	}
	tex->end(rs);

	return tex;
}


CloudLayer::CloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize)
		: SkyLayer(sky) {
	state()->joinStates(ref_ptr<BlendState>::alloc(GL_SRC_ALPHA, GL_ONE));

	cloudTexture_ = ref_ptr<Texture2D>::alloc(1);
	cloudTexture_->begin(RenderState::get());
	cloudTexture_->set_rectangleSize(textureSize, textureSize);
	cloudTexture_->set_format(GL_RED);
	cloudTexture_->set_internalFormat(GL_R16F);
	cloudTexture_->set_pixelType(GL_FLOAT);
	cloudTexture_->filter().push(GL_LINEAR);
	cloudTexture_->wrapping().push(GL_REPEAT);
	cloudTexture_->texImage();
	cloudTexture_->end(RenderState::get());
	GL_ERROR_LOG();
	state()->joinStates(ref_ptr<TextureState>::alloc(cloudTexture_, "cloudTexture"));

	// create render target for updating the sky cube map
	fbo_ = ref_ptr<FBO>::alloc(textureSize, textureSize);
	RenderState::get()->drawFrameBuffer().push(fbo_->id());
	fbo_->drawBuffers().push(DrawBuffers::attachment0());
	glClear(GL_COLOR_BUFFER_BIT);
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cloudTexture_->id(), 0);
	RenderState::get()->drawFrameBuffer().pop();

	cloudUniforms_ = ref_ptr<UniformBlock>::alloc("DrawClouds");
	state()->joinShaderInput(cloudUniforms_);

	color_ = ref_ptr<ShaderInput3f>::alloc("color");
	color_->setUniformData(Vec3f(1.f, 1.f, 1.f));
	cloudUniforms_->addUniform(color_);

	altitude_ = ref_ptr<ShaderInput1f>::alloc("altitude");
	altitude_->setUniformData(8.0f);
	cloudUniforms_->addUniform(altitude_);

	bottomColor_ = ref_ptr<ShaderInput3f>::alloc("bcolor");
	bottomColor_->setUniformData(Vec3f(1.f, 1.f, 1.f));
	cloudUniforms_->addUniform(bottomColor_);

	thickness_ = ref_ptr<ShaderInput1f>::alloc("thickness");
	thickness_->setUniformData(3.0f);
	cloudUniforms_->addUniform(thickness_);

	topColor_ = ref_ptr<ShaderInput3f>::alloc("tcolor");
	topColor_->setUniformData(Vec3f(1.f, 1.f, 1.f));
	cloudUniforms_->addUniform(topColor_);

	offset_ = ref_ptr<ShaderInput1f>::alloc("offset");
	offset_->setUniformData(-0.5f);
	cloudUniforms_->addUniform(offset_);

	scale_ = ref_ptr<ShaderInput2f>::alloc("scale");
	scale_->setUniformData(Vec2f(32.0, 32.0));
	cloudUniforms_->addUniform(scale_);

	shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.clouds.cloud-layer");
	meshState_ = ref_ptr<Rectangle>::alloc(sky->skyQuad());

	///////
	/// Update Uniforms
	///////
	noise0_ = createNoiseArray(1 << 6, 3, 4);
	noise1_ = createNoiseArray(1 << 7, 4, 4);
	noise2_ = createNoiseArray(1 << 8, 5, 4);
	noise3_ = createNoiseArray(1 << 8, 6, 4);
	coverage_ = ref_ptr<ShaderInput1f>::alloc("coverage");
	coverage_->setUniformData(0.2f);
	sharpness_ = ref_ptr<ShaderInput1f>::alloc("sharpness");
	sharpness_->setUniformData(0.5f);
	change_ = ref_ptr<ShaderInput1f>::alloc("change");
	change_->setUniformData(0.1f);
	wind_ = ref_ptr<ShaderInput2f>::alloc("wind");
	wind_->setUniformData(Vec2f(0.f, 0.f));
	///////
	/// Update State
	///////
	ref_ptr<Mesh> updateMesh = Rectangle::getUnitQuad();
	updateState_ = ref_ptr<State>::alloc();
	updateState_->joinShaderInput(sky->timeUniform());
	updateState_->joinShaderInput(fbo_->inverseViewport());
	updateState_->joinStates(ref_ptr<TextureState>::alloc(noise0_, "noise0"));
	updateState_->joinStates(ref_ptr<TextureState>::alloc(noise1_, "noise1"));
	updateState_->joinStates(ref_ptr<TextureState>::alloc(noise2_, "noise2"));
	updateState_->joinStates(ref_ptr<TextureState>::alloc(noise3_, "noise3"));
	updateState_->joinShaderInput(coverage_);
	updateState_->joinShaderInput(sharpness_);
	updateState_->joinShaderInput(change_);
	updateState_->joinShaderInput(wind_);
	updateShader_ = ref_ptr<ShaderState>::alloc();
	updateState_->joinStates(updateShader_);
	updateState_->joinStates(updateMesh);
	///////
	/// Update Shader
	///////
	StateConfig shaderConfig = StateConfigurer::configure(updateState_.get());
	shaderConfig.setVersion(330);
	updateShader_->createShader(shaderConfig, "regen.sky.clouds.pre-noise");
	updateMesh->updateVAO(RenderState::get(), shaderConfig, updateShader_->shader());
}


float CloudLayer::defaultAltitudeHigh() { return 8.0f; }

float CloudLayer::defaultAltitudeLow() { return 2.0f; }

Vec2f CloudLayer::defaultScaleHigh() { return {32.0, 32.0}; }

Vec2f CloudLayer::defaultScaleLow() { return {128.0, 128.0}; }

GLdouble CloudLayer::defaultChangeHigh() { return 0.1f; }

GLdouble CloudLayer::defaultChangeLow() { return 0.1f; }

const ref_ptr<Texture2D> &CloudLayer::cloudTexture() const { return cloudTexture_; }

const ref_ptr<FBO> &CloudLayer::cloudTextureFBO() const { return fbo_; }

void CloudLayer::set_color(const Vec3f &color) { color_->setVertex(0, color); }

const ref_ptr<ShaderInput3f> &CloudLayer::color() const { return color_; }

void CloudLayer::set_altitude(GLdouble altitude) { altitude_->setVertex(0, altitude); }

const ref_ptr<ShaderInput1f> &CloudLayer::altitude() const { return altitude_; }

void CloudLayer::set_sharpness(GLdouble sharpness) { sharpness_->setVertex(0, sharpness); }

const ref_ptr<ShaderInput1f> &CloudLayer::sharpness() const { return sharpness_; }

void CloudLayer::set_coverage(GLdouble coverage) { coverage_->setVertex(0, coverage); }

const ref_ptr<ShaderInput1f> &CloudLayer::coverage() const { return coverage_; }

void CloudLayer::set_scale(const Vec2f &scale) { scale_->setVertex(0, scale); }

const ref_ptr<ShaderInput2f> &CloudLayer::scale() const { return scale_; }

void CloudLayer::set_change(GLdouble change) { change_->setVertex(0, change); }

const ref_ptr<ShaderInput1f> &CloudLayer::change() const { return change_; }

void CloudLayer::set_wind(const Vec2f &wind) { wind_->setVertex(0, wind); }

const ref_ptr<ShaderInput2f> &CloudLayer::wind() const { return wind_; }

void CloudLayer::set_bottomColor(const Vec3f &color) { bottomColor_->setVertex(0, color); }

const ref_ptr<ShaderInput3f> &CloudLayer::bottomColor() const { return bottomColor_; }

void CloudLayer::set_topColor(const Vec3f &color) { topColor_->setVertex(0, color); }

const ref_ptr<ShaderInput3f> &CloudLayer::topColor() const { return topColor_; }

void CloudLayer::set_thickness(GLdouble thickness) { thickness_->setVertex(0, thickness); }

const ref_ptr<ShaderInput1f> &CloudLayer::thickness() const { return thickness_; }

void CloudLayer::set_offset(GLdouble offset) { offset_->setVertex(0, offset); }

const ref_ptr<ShaderInput1f> &CloudLayer::offset() const { return offset_; }

ref_ptr<Mesh> CloudLayer::getMeshState() { return meshState_; }

ref_ptr<HasShader> CloudLayer::getShaderState() { return shaderState_; }

void CloudLayer::updateSkyLayer(RenderState *rs, GLdouble dt) {
	rs->drawFrameBuffer().push(fbo_->id());
	glClear(GL_COLOR_BUFFER_BIT);
	rs->viewport().push(fbo_->glViewport());

	updateState_->enable(rs);
	updateState_->disable(rs);

	rs->viewport().pop();
	rs->drawFrameBuffer().pop();
}


