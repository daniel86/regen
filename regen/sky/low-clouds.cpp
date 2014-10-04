/*
 * low-clouds.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "low-clouds.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>
#include <regen/meshes/rectangle.h>

using namespace regen;

#define _randf(min, max) \
    (static_cast<float>(rand()) / RAND_MAX * ((max) - (min)) + (min))

// TODO: redundant: high clouds
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


LowCloudLayer::LowCloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize)
: SkyLayer(sky)
{
  RenderState *rs = RenderState::get();
  set_name("LowCloudLayer");
  srand(time(NULL));

  ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
  depth->set_depthFunc(GL_LEQUAL);
  depth->set_depthRange(1.0, 1.0);
  depth->set_useDepthTest(GL_TRUE);
  state()->joinStates(depth);
  state()->joinStates(ref_ptr<BlendState>::alloc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  // TODO: redundant
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

  // TODO: unused
  q_ = ref_ptr<ShaderInput1f>::alloc("q");
  q_->setUniformData(0.0f);
  state()->joinShaderInput(q_);

  state()->joinShaderInput(sky->sunPositionR(), "sunPositionR");

  bottomColor_ = ref_ptr<ShaderInput3f>::alloc("bcolor");
  bottomColor_->setUniformData(Vec3f(1.f, 1.f, 1.f));
  state()->joinShaderInput(bottomColor_);

  topColor_ = ref_ptr<ShaderInput3f>::alloc("tcolor");
  topColor_->setUniformData(Vec3f(1.f, 1.f, 1.f));
  state()->joinShaderInput(topColor_);

  altitude_ = ref_ptr<ShaderInput1f>::alloc("altitude");
  altitude_->setUniformData(defaultAltitude());
  state()->joinShaderInput(altitude_);

  thickness_ = ref_ptr<ShaderInput1f>::alloc("thickness");
  thickness_->setUniformData(3.0f);
  state()->joinShaderInput(thickness_);

  offset_ = ref_ptr<ShaderInput1f>::alloc("offset");
  offset_->setUniformData(-0.5f);
  state()->joinShaderInput(offset_);

  scale_ = ref_ptr<ShaderInput2f>::alloc("scale");
  scale_->setUniformData(defaultScale());
  state()->joinShaderInput(scale_);

  shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.clouds.low-layer");
  state()->joinStates(shaderState_->shaderState());

  meshState_ = Rectangle::getUnitQuad();
  state()->joinStates(meshState_);

  // TODO redundant
  ///////
  /// Update Uniforms
  ///////
  coverage_ = ref_ptr<ShaderInput1f>::alloc("coverage");
  coverage_->setUniformData(0.2f);
  sharpness_ = ref_ptr<ShaderInput1f>::alloc("sharpness");
  sharpness_->setUniformData(0.3f);
  change_ = ref_ptr<ShaderInput1f>::alloc("change");
  change_->setUniformData(defaultChange());
  wind_ = ref_ptr<ShaderInput2f>::alloc("wind");
  wind_->setUniformData(Vec2f(0.f, 0.f));
  ///////
  /// Update State
  ///////
  noise0_ = createNoiseArray(1 << 6, 3, 4);
  noise1_ = createNoiseArray(1 << 7, 4, 4);
  noise2_ = createNoiseArray(1 << 8, 5, 4);
  noise3_ = createNoiseArray(1 << 8, 6, 4);
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


void LowCloudLayer::set_altitude(GLdouble altitude)
{ altitude_->setVertex(0, altitude); }

const ref_ptr<ShaderInput1f>& LowCloudLayer::altitude() const
{ return altitude_; }

const float LowCloudLayer::defaultAltitude()
{ return 2.0f; }

void LowCloudLayer::set_sharpness(GLdouble sharpness)
{ sharpness_->setVertex(0, sharpness); }

const ref_ptr<ShaderInput1f>& LowCloudLayer::sharpness() const
{ return sharpness_; }

void LowCloudLayer::set_coverage(GLdouble coverage)
{ coverage_->setVertex(0, coverage); }

const ref_ptr<ShaderInput1f>& LowCloudLayer::coverage() const
{ return coverage_; }

void LowCloudLayer::set_scale(const Vec2f &scale)
{ scale_->setVertex(0, scale); }

const ref_ptr<ShaderInput2f>& LowCloudLayer::scale() const
{ return scale_; }

const Vec2f LowCloudLayer::defaultScale()
{ return Vec2f(128.0, 128.0); }

void LowCloudLayer::set_change(GLdouble change)
{ change_->setVertex(0, change); }

const ref_ptr<ShaderInput1f>& LowCloudLayer::change() const
{ return change_; }

GLdouble LowCloudLayer::defaultChange()
{ return 0.1f; }

void LowCloudLayer::set_wind(const Vec2f &wind)
{ wind_->setVertex(0, wind); }

const ref_ptr<ShaderInput2f>& LowCloudLayer::wind() const
{ return wind_; }

void LowCloudLayer::set_bottomColor(const Vec3f &color)
{ bottomColor_->setVertex(0, color); }

const ref_ptr<ShaderInput3f>& LowCloudLayer::bottomColor() const
{ return bottomColor_; }

void LowCloudLayer::set_topColor(const Vec3f &color)
{ topColor_->setVertex(0, color); }

const ref_ptr<ShaderInput3f>& LowCloudLayer::topColor() const
{ return topColor_; }

void LowCloudLayer::set_thickness(GLdouble thickness)
{ thickness_->setVertex(0, thickness); }

const ref_ptr<ShaderInput1f>& LowCloudLayer::thickness() const
{ return thickness_; }

void LowCloudLayer::set_offset(GLdouble offset)
{ offset_->setVertex(0, offset); }

const ref_ptr<ShaderInput1f>& LowCloudLayer::offset() const
{ return offset_; }


ref_ptr<Mesh> LowCloudLayer::getMeshState()
{ return meshState_; }

ref_ptr<HasShader> LowCloudLayer::getShaderState()
{ return shaderState_; }

#define _rad(deg) ((deg) * M_PI / 180.0L)

void LowCloudLayer::updateSkyLayer(RenderState *rs, GLdouble dt)
{
  // TODO: starmap and planets also require this ... - find better place
  const float fov = sky_->camera()->fov()->getVertex(0); // himmel.getCameraFovHint();
  const float height = sky_->viewport()->getVertex(0).x;
  q_->setUniformData(sqrt(2.0) * 2.0 * tan(_rad(fov * 0.5)) / height);

  rs->drawFrameBuffer().push(fbo_->id());
  rs->viewport().push(fbo_->glViewport());

  updateState_->enable(rs);
  updateState_->disable(rs);

  rs->viewport().pop();
  rs->drawFrameBuffer().pop();
}


