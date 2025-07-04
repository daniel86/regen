#include "cloud-layer.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/state-configurer.h>

using namespace regen;

static GLfloat *createNoiseSlice(GLuint texSize, GLuint octave) {
	GLuint size2 = texSize * texSize;
	GLfloat oneOverTexSize = 1.f / static_cast<float>(texSize);
	osgHimmel::Noise n(1 << (octave + 2),
					   math::random<float>(0.f, 1.f),
					   math::random<float>(0.f, 1.f));

	auto *noise = new float[size2];
	GLuint o;
	for (GLuint s = 0; s < texSize; ++s)
		for (GLuint t = 0; t < texSize; ++t) {
			o = t * texSize + s;
			noise[o] = n.noise2(
					static_cast<float>(s) * oneOverTexSize,
					static_cast<float>(t) * oneOverTexSize, octave) * 0.5f + 0.5f;
		}

	return noise;
}

static ref_ptr<Texture3D> createNoiseArray(GLuint texSize, GLuint octave, GLuint slices) {
	ref_ptr<Texture3D> tex = ref_ptr<Texture3D>::alloc();
	//ref_ptr<Texture2DArray> tex = ref_ptr<Texture2DArray>::alloc();
	tex->set_rectangleSize(texSize, texSize);
	tex->set_depth(slices);
	tex->set_format(GL_RED);
	tex->set_internalFormat(GL_R16F);
	tex->set_pixelType(GL_FLOAT);
	tex->allocTexture();
	for (uint32_t s = 0; s < slices; ++s) {
		GLfloat *data = createNoiseSlice(texSize, octave);
		tex->updateSubImage(static_cast<int>(s), (GLubyte *) data);
		delete[]data;
	}
	tex->set_filter(GL_LINEAR);
	tex->set_wrapping(GL_REPEAT);

	return tex;
}


CloudLayer::CloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize)
		: SkyLayer(sky) {
	state()->joinStates(ref_ptr<BlendState>::alloc(GL_SRC_ALPHA, GL_ONE));

	cloudTexture_ = ref_ptr<Texture2D>::alloc();
	cloudTexture_->set_rectangleSize(textureSize, textureSize);
	cloudTexture_->set_format(GL_RED);
	cloudTexture_->set_internalFormat(GL_R16F);
	cloudTexture_->set_pixelType(GL_FLOAT);
	cloudTexture_->allocTexture();
	cloudTexture_->set_filter(GL_LINEAR);
	cloudTexture_->set_wrapping(GL_REPEAT);
	state()->joinStates(ref_ptr<TextureState>::alloc(cloudTexture_, "cloudTexture"));

	// create render target for updating the sky cube map
	fbo_ = ref_ptr<FBO>::alloc(textureSize, textureSize);
	glNamedFramebufferTexture(
			fbo_->id(),
			GL_COLOR_ATTACHMENT0,
			cloudTexture_->id(),
			0);
	fbo_->clearColor({0.0, 0.0, 0.0, 1.0});

	color_ = ref_ptr<ShaderInput3f>::alloc("color");
	color_->setUniformData(Vec3f(1.f, 1.f, 1.f));
	state()->joinShaderInput(color_);

	altitude_ = ref_ptr<ShaderInput1f>::alloc("altitude");
	altitude_->setUniformData(8.0f);
	state()->joinShaderInput(altitude_);

	bottomColor_ = ref_ptr<ShaderInput3f>::alloc("bcolor");
	bottomColor_->setUniformData(Vec3f(1.f, 1.f, 1.f));
	state()->joinShaderInput(bottomColor_);

	thickness_ = ref_ptr<ShaderInput1f>::alloc("thickness");
	thickness_->setUniformData(3.0f);
	state()->joinShaderInput(thickness_);

	topColor_ = ref_ptr<ShaderInput3f>::alloc("tcolor");
	topColor_->setUniformData(Vec3f(1.f, 1.f, 1.f));
	state()->joinShaderInput(topColor_);

	offset_ = ref_ptr<ShaderInput1f>::alloc("offset");
	offset_->setUniformData(-0.5f);
	state()->joinShaderInput(offset_);

	scale_ = ref_ptr<ShaderInput2f>::alloc("scale");
	scale_->setUniformData(Vec2f(32.0, 32.0));
	state()->joinShaderInput(scale_);

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
	updateMesh_->updateVAO(shaderConfig, updateShader_->shader());
}

float CloudLayer::defaultAltitudeHigh() {
	return 8.0f;
}

float CloudLayer::defaultAltitudeLow() {
	return 2.0f;
}

Vec2f CloudLayer::defaultScaleHigh() {
	return {32.0, 32.0};
}

Vec2f CloudLayer::defaultScaleLow() {
	return {128.0, 128.0};
}

float CloudLayer::defaultChangeHigh() {
	return 0.1f;
}

float CloudLayer::defaultChangeLow() {
	return 0.1f;
}

void CloudLayer::updateSkyLayer(RenderState *rs, GLdouble dt) {
	static const Vec4f clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	fbo_->clearColor(clearColor);
	rs->viewport().push(fbo_->glViewport());

	updateState_->enable(rs);
	updateMesh_->draw(rs);
	updateState_->disable(rs);

	rs->viewport().pop();
}


