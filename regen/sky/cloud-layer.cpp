/*
 * cloud-layer.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "cloud-layer.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/state-configurer.h>

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

	shaderState_ = ref_ptr<HasShader>::alloc("regen.weather.clouds.cloud-layer");
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
	updateMesh_ = Rectangle::getUnitQuad();
	updateState_->joinShaderInput(fbo_->inverseViewport());
	updateState_->joinStates(ref_ptr<TextureState>::alloc(noise0_, "noise0"));
	updateState_->joinStates(ref_ptr<TextureState>::alloc(noise1_, "noise1"));
	updateState_->joinStates(ref_ptr<TextureState>::alloc(noise2_, "noise2"));
	updateState_->joinStates(ref_ptr<TextureState>::alloc(noise3_, "noise3"));
	updateState_->joinShaderInput(coverage_);
	updateState_->joinShaderInput(sharpness_);
	updateState_->joinShaderInput(change_);
	updateState_->joinShaderInput(wind_);
	updateState_->joinShaderInput(sky->worldTime()->in);
	updateShader_ = ref_ptr<ShaderState>::alloc();
	updateState_->joinStates(updateShader_);
}

void CloudLayer::createUpdateShader() {
	StateConfig shaderConfig = StateConfigurer::configure(updateState_.get());
	shaderConfig.setVersion(330);
	updateShader_->createShader(shaderConfig, "regen.weather.clouds.pre-noise");
	updateMesh_->updateVAO(RenderState::get(), shaderConfig, updateShader_->shader());
}

float CloudLayer::defaultAltitudeHigh() { return 8.0f; }

float CloudLayer::defaultAltitudeLow() { return 2.0f; }

Vec2f CloudLayer::defaultScaleHigh() { return {32.0, 32.0}; }

Vec2f CloudLayer::defaultScaleLow() { return {128.0, 128.0}; }

float CloudLayer::defaultChangeHigh() { return 0.1f; }

float CloudLayer::defaultChangeLow() { return 0.1f; }

void CloudLayer::updateSkyLayer(RenderState *rs, GLdouble dt) {
	rs->drawFrameBuffer().push(fbo_->id());
	glClear(GL_COLOR_BUFFER_BIT);
	rs->viewport().push(fbo_->glViewport());

	updateState_->enable(rs);
	updateMesh_->enable(rs);
	updateMesh_->disable(rs);
	updateState_->disable(rs);

	rs->viewport().pop();
	rs->drawFrameBuffer().pop();
}


