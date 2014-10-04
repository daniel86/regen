/*
 * high-clouds.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "high-clouds.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>
#include <regen/meshes/rectangle.h>

using namespace regen;

#define _randf(min, max) \
    (static_cast<float>(rand()) / RAND_MAX * ((max) - (min)) + (min))

static GLfloat* createNoiseSlice(GLuint texSize, GLuint octave)
{
  GLuint size2 = texSize * texSize;
  GLfloat oneOverTexSize = 1.f / static_cast<GLfloat>(texSize);

  osgHimmel::Noise n(1 << (octave + 2), _randf(0.f, 1.f), _randf(0.f, 1.f));

  GLfloat *noise = new float[size2];
  GLuint o;
  for(GLuint s = 0; s < texSize; ++s)
  for(GLuint t = 0; t < texSize; ++t)
  {
    o = t * texSize + s;
    noise[o] = n.noise2(s * oneOverTexSize, t * oneOverTexSize, octave) * 0.5 + 0.5;
  }

  return noise;
}

static ref_ptr<Texture3D> createNoiseArray(GLuint texSize, GLuint octave, GLuint slices)
{
  RenderState *rs = RenderState::get();

  ref_ptr<Texture3D> tex = ref_ptr<Texture3D>::alloc();
  //ref_ptr<Texture2DArray> tex = ref_ptr<Texture2DArray>::alloc();
  tex->begin(rs); {
    tex->set_rectangleSize(texSize, texSize);
    tex->set_depth(slices);
    tex->set_format(GL_LUMINANCE);
    tex->set_internalFormat(GL_LUMINANCE16F_ARB);
    tex->set_pixelType(GL_FLOAT);

    // TODO: need to call texImage before sub image?
    tex->texImage();
    for(unsigned int s = 0; s < slices; ++s) {
      GLfloat *data = createNoiseSlice(texSize, octave);
      tex->texSubImage(s, (GLubyte*)data);
      delete []data;
    }

    tex->filter().push(GL_LINEAR);
    tex->wrapping().push(GL_REPEAT);
  } tex->end(rs);

  return tex;
}


HighCloudLayer::HighCloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize)
: SkyLayer(sky)
{
  RenderState *rs = RenderState::get();
  set_name("HighCloudLayer");
  srand(time(NULL));

  ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
  depth->set_depthFunc(GL_LEQUAL);
  depth->set_depthRange(1.0, 1.0);
  depth->set_useDepthTest(GL_TRUE);
  state()->joinStates(depth);
  state()->joinStates(ref_ptr<BlendState>::alloc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  cloudTexture_ = ref_ptr<Texture2D>::alloc(1);
  cloudTexture_->begin(rs);
  cloudTexture_->set_rectangleSize(textureSize,textureSize);
  cloudTexture_->set_format(GL_LUMINANCE);
  cloudTexture_->set_internalFormat(GL_LUMINANCE16F_ARB);
  cloudTexture_->set_pixelType(GL_FLOAT);
  cloudTexture_->filter().push(GL_LINEAR);
  cloudTexture_->wrapping().push(GL_REPEAT);
  cloudTexture_->texImage();
  cloudTexture_->end(rs);
  state()->joinStates(ref_ptr<TextureState>::alloc(cloudTexture_, "cloudTexture"));

  // create render target for updating the sky cube map
  fbo_ = ref_ptr<FBO>::alloc(textureSize,textureSize);
  rs->drawFrameBuffer().push(fbo_->id());
  fbo_->drawBuffers().push(DrawBuffers::attachment0());
  glClear(GL_COLOR_BUFFER_BIT);
  glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cloudTexture_->id(), 0);
  rs->drawFrameBuffer().pop();

  color_ = ref_ptr<ShaderInput3f>::alloc("color");
  color_->setUniformData(Vec3f(1.f, 1.f, 1.f));
  state()->joinShaderInput(color_);

  altitude_ = ref_ptr<ShaderInput1f>::alloc("altitude");
  altitude_->setUniformData(defaultAltitude());
  state()->joinShaderInput(altitude_);

  scale_ = ref_ptr<ShaderInput2f>::alloc("scale");
  scale_->setUniformData(defaultScale());
  state()->joinShaderInput(scale_);

  shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.clouds.high-layer");
  state()->joinStates(shaderState_->shaderState());

  meshState_ = Rectangle::getUnitQuad();
  state()->joinStates(meshState_);

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
  change_->setUniformData(defaultChange());
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
  updateMesh->updateVAO(rs, shaderConfig, updateShader_->shader());
}


void HighCloudLayer::set_altitude(GLdouble altitude)
{ altitude_->setVertex(0, altitude); }

const ref_ptr<ShaderInput1f>& HighCloudLayer::altitude() const
{ return altitude_; }

const float HighCloudLayer::defaultAltitude()
{ return 8.0f; }

void HighCloudLayer::set_sharpness(GLdouble sharpness)
{ sharpness_->setVertex(0, sharpness); }

const ref_ptr<ShaderInput1f>& HighCloudLayer::sharpness() const
{ return sharpness_; }

void HighCloudLayer::set_coverage(GLdouble coverage)
{ coverage_->setVertex(0, coverage); }

const ref_ptr<ShaderInput1f>& HighCloudLayer::coverage() const
{ return coverage_; }

void HighCloudLayer::set_scale(const Vec2f &scale)
{ scale_->setVertex(0, scale); }

const ref_ptr<ShaderInput2f>& HighCloudLayer::scale() const
{ return scale_; }

const Vec2f HighCloudLayer::defaultScale()
{ return Vec2f(32.0, 32.0); }

void HighCloudLayer::set_change(GLdouble change)
{ change_->setVertex(0, change); }

const ref_ptr<ShaderInput1f>& HighCloudLayer::change() const
{ return change_; }

GLdouble HighCloudLayer::defaultChange()
{ return 0.1f; }

void HighCloudLayer::set_wind(const Vec2f &wind)
{ wind_->setVertex(0, wind); }

const ref_ptr<ShaderInput2f>& HighCloudLayer::wind() const
{ return wind_; }

void HighCloudLayer::set_color(const Vec3f &color)
{ color_->setVertex(0, color); }

const ref_ptr<ShaderInput3f>& HighCloudLayer::color() const
{ return color_; }


ref_ptr<Mesh> HighCloudLayer::getMeshState()
{ return meshState_; }

ref_ptr<HasShader> HighCloudLayer::getShaderState()
{ return shaderState_; }

#define _rad(deg) ((deg) * M_PI / 180.0L)

void HighCloudLayer::updateSkyLayer(RenderState *rs, GLdouble dt)
{
  rs->drawFrameBuffer().push(fbo_->id());
  rs->viewport().push(fbo_->glViewport());

  updateState_->enable(rs);
  updateState_->disable(rs);

  rs->viewport().pop();
  rs->drawFrameBuffer().pop();
}


