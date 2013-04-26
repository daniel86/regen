/*
 * sky-box.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <climits>

#include <regen/meshes/rectangle.h>
#include <regen/states/atomic-states.h>
#include <regen/states/depth-state.h>
#include <regen/states/vao-state.h>
#include <regen/states/shader-configurer.h>

#include "sky.h"
using namespace regen;

static Box::Config cubeCfg()
{
  Box::Config cfg;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.texcoMode = Box::TEXCO_MODE_CUBE_MAP;
  return cfg;
}

SkyBox::SkyBox()
: Box(cubeCfg()), HasShader("sky.skyBox")
{
  vao_ = ref_ptr<VAOState>::manage(new VAOState(shaderState_));

  joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));

  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_depthFunc(GL_LEQUAL);
  joinStates(depth);

  joinStates(shaderState());
  joinStates(vao_);

  shaderDefine("IGNORE_VIEW_TRANSLATION", "TRUE");
}

void SkyBox::createShader(const ShaderState::Config &cfg)
{
  shaderState_->createShader(cfg,shaderKey_);
  vao_->updateVAO(RenderState::get(), this);
}

void SkyBox::setCubeMap(const ref_ptr<TextureCube> &cubeMap)
{
  cubeMap_ = cubeMap;
  if(texState_.get()) {
    disjoinStates(texState_);
  }
  texState_ = ref_ptr<TextureState>::manage(new TextureState(cubeMap_));
  texState_->set_mapTo(TextureState::MAP_TO_COLOR);
  joinStatesFront(texState_);
}
const ref_ptr<TextureCube>& SkyBox::cubeMap() const
{
  return cubeMap_;
}

///////////
///////////
///////////

SkyScattering::SkyScattering(GLuint cubeMapSize, GLboolean useFloatBuffer)
: SkyBox(), Animation(GL_TRUE,GL_FALSE),  dayTime_(0.4), timeScale_(0.00000004)
{
  dayLength_ = 0.8;
  maxSunElevation_ = 30.0;
  minSunElevation_ = -20.0;
  updateInterval_ = 4000.0;
  dt_ = updateInterval_;

  ref_ptr<TextureCube> cubeMap = ref_ptr<TextureCube>::manage(new TextureCube(1));

  cubeMap->startConfig();
  cubeMap->set_format(GL_RGBA);
  if(useFloatBuffer) {
    cubeMap->set_internalFormat(GL_RGBA16F);
  } else {
    cubeMap->set_internalFormat(GL_RGBA);
  }
  cubeMap->set_filter(GL_LINEAR, GL_LINEAR);
  cubeMap->set_size(cubeMapSize,cubeMapSize);
  cubeMap->set_wrapping(GL_CLAMP_TO_EDGE);
  cubeMap->texImage();
  setCubeMap(cubeMap);
  cubeMap->stopConfig();

  // create render target for updating the sky cube map
  fbo_ = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(cubeMapSize,cubeMapSize));
  RenderState::get()->drawFrameBuffer().push(fbo_->id());
  fbo_->drawBuffers().push(DrawBuffers::attachment0());
  // clear negative y to black, -y cube face is not updated
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, cubeMap->id(), 0);
  glClear(GL_COLOR_BUFFER_BIT);
  // for updating bind all layers to GL_COLOR_ATTACHMENT0
  glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cubeMap->id(), 0);
  RenderState::get()->drawFrameBuffer().pop();

  // directional light that approximates the sun
  sun_ = ref_ptr<Light>::manage(new Light(Light::DIRECTIONAL));
  sun_->set_isAttenuated(GL_FALSE);
  sun_->specular()->setVertex3f(0,Vec3f(0.0f));
  sun_->diffuse()->setVertex3f(0,Vec3f(0.0f));
  sun_->direction()->setVertex3f(0,Vec3f(1.0f));
  sunDirection_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("sunDir"));
  sunDirection_->setUniformData(Vec3f(0.0f));

  // create mvp matrix for each cube face
  mvpMatrices_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("mvpMatrices",6));
  mvpMatrices_->setVertexData(1,NULL);
  const Mat4f *views = Mat4f::cubeLookAtMatrices();
  Mat4f proj = Mat4f::projectionMatrix(90.0, 1.0f, 0.1, 2.0);
  for(register GLuint i=0; i<6; ++i) {
    mvpMatrices_->setVertex16f(i, views[i] * proj);
  }

  ///////
  /// Scattering uniforms
  ///////
  rayleigh_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("rayleigh"));
  rayleigh_->setUniformData(Vec3f(0.0f));
  mie_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("mie"));
  mie_->setUniformData(Vec4f(0.0f));
  spotBrightness_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("spotBrightness"));
  spotBrightness_->setUniformData(0.0f);
  scatterStrength_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("scatterStrength"));
  scatterStrength_->setUniformData(0.0f);
  skyAbsorbtion_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("skyAbsorbtion"));
  skyAbsorbtion_->setUniformData(Vec3f(0.0f));

  updateState_ = ref_ptr<State>::manage(new State);
  // upload uniforms
  updateState_->joinShaderInput(sunDirection_);
  updateState_->joinShaderInput(rayleigh_);
  updateState_->joinShaderInput(mie_);
  updateState_->joinShaderInput(spotBrightness_);
  updateState_->joinShaderInput(scatterStrength_);
  updateState_->joinShaderInput(skyAbsorbtion_);

  updateShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  ref_ptr<Mesh> mesh = Rectangle::getUnitQuad();
  ref_ptr<VAOState> vao = ref_ptr<VAOState>::manage(new VAOState(updateShader_));

  updateState_->joinStates(updateShader_);
  updateState_->joinStates(vao);
  updateState_->joinStates(mesh);

  // create shader based on configuration
  ShaderState::Config shaderConfig = ShaderConfigurer::configure(updateState_.get());
  shaderConfig.setVersion(330);
  updateShader_->createShader(shaderConfig, "sky.scattering");
  vao->updateVAO(RenderState::get(), mesh.get());
}

void SkyScattering::set_dayTime(GLdouble time)
{
  dayTime_ = time;
}

void SkyScattering::set_timeScale(GLdouble scale)
{
  timeScale_ = scale;
}

ref_ptr<Light>& SkyScattering::sun()
{ return sun_; }

void SkyScattering::setSunElevation(GLdouble dayLength,
      GLdouble maxElevation,
      GLdouble minElevation)
{
  dayLength_ = dayLength;
  maxSunElevation_ = maxElevation;
  minSunElevation_ = minElevation;
}

void SkyScattering::setRayleighBrightness(GLfloat v)
{
  const Vec3f &rayleigh = rayleigh_->getVertex3f(0);
  rayleigh_->setVertex3f(0, Vec3f(v/10.0, rayleigh.y, rayleigh.z));
}
void SkyScattering::setRayleighStrength(GLfloat v)
{
  const Vec3f &rayleigh = rayleigh_->getVertex3f(0);
  rayleigh_->setVertex3f(0, Vec3f(rayleigh.x, v/1000.0, rayleigh.z));
}
void SkyScattering::setRayleighCollect(GLfloat v)
{
  const Vec3f &rayleigh = rayleigh_->getVertex3f(0);
  rayleigh_->setVertex3f(0, Vec3f(rayleigh.x, rayleigh.y, v/100.0));
}
ref_ptr<ShaderInput3f>& SkyScattering::rayleigh()
{ return rayleigh_; }

void SkyScattering::setMieBrightness(GLfloat v)
{
  const Vec4f &mie = mie_->getVertex4f(0);
  mie_->setVertex4f(0, Vec4f(v/1000.0, mie.y, mie.z, mie.w));
}
void SkyScattering::setMieStrength(GLfloat v)
{
  const Vec4f &mie = mie_->getVertex4f(0);
  mie_->setVertex4f(0, Vec4f(mie.x, v/10000.0, mie.z, mie.w));
}
void SkyScattering::setMieCollect(GLfloat v)
{
  const Vec4f &mie = mie_->getVertex4f(0);
  mie_->setVertex4f(0, Vec4f(mie.x, mie.y, v/100.0, mie.w));
}
void SkyScattering::setMieDistribution(GLfloat v)
{
  const Vec4f &mie = mie_->getVertex4f(0);
  mie_->setVertex4f(0, Vec4f(mie.x, mie.y, mie.z, v/100.0));
}
ref_ptr<ShaderInput4f>& SkyScattering::mie()
{ return mie_; }

void SkyScattering::setSpotBrightness(GLfloat v)
{
  spotBrightness_->setVertex1f(0, v);
}
ref_ptr<ShaderInput1f>& SkyScattering::spotBrightness()
{ return spotBrightness_; }

void SkyScattering::setScatterStrength(GLfloat v)
{
  scatterStrength_->setVertex1f(0, v/1000.0);
}
ref_ptr<ShaderInput1f>& SkyScattering::scatterStrength()
{ return scatterStrength_; }

void SkyScattering::setAbsorbtion(const Vec3f &color)
{
  skyAbsorbtion_->setVertex3f(0, color);
}
ref_ptr<ShaderInput3f>& SkyScattering::absorbtion()
{ return skyAbsorbtion_; }

void SkyScattering::setEarth()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(19.0,359.0,81.0);
  prop.mie = Vec4f(44.0,308.0,39.0,74.0);
  prop.spot = 373.0;
  prop.scatterStrength = 54.0;
  prop.absorption = Vec3f(
      0.18867780436772762,
      0.4978442963618773,
      0.6616065586417131);
  setPlanetProperties(prop);
}

void SkyScattering::setMars()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(33.0,139.0,81.0);
  prop.mie = Vec4f(100.0,264.0,39.0,63.0);
  prop.spot = 1000.0;
  prop.scatterStrength = 28.0;
  prop.absorption = Vec3f(0.66015625, 0.5078125, 0.1953125);
  setPlanetProperties(prop);
}

void SkyScattering::setUranus()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(80.0,136.0,71.0);
  prop.mie = Vec4f(67.0,68.0,0.0,56.0);
  prop.spot = 0.0;
  prop.scatterStrength = 18.0;
  prop.absorption = Vec3f(0.26953125, 0.5234375, 0.8867187);
  setPlanetProperties(prop);
}

void SkyScattering::setVenus()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(25.0,397.0,34.0);
  prop.mie = Vec4f(124.0,298.0,76.0,81.0);
  prop.spot = 0.0;
  prop.scatterStrength = 140.0;
  prop.absorption = Vec3f(0.6640625, 0.5703125, 0.29296875);
  setPlanetProperties(prop);
}

void SkyScattering::setAlien()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(44.0,169.0,71.0);
  prop.mie = Vec4f(60.0,139.0,46.0,86.0);
  prop.spot = 0.0;
  prop.scatterStrength = 26.0;
  prop.absorption = Vec3f(0.24609375, 0.53125, 0.3515625);
  setPlanetProperties(prop);
}

void SkyScattering::setPlanetProperties(PlanetProperties &p)
{
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

/////////////
/////////////

void SkyScattering::set_updateInterval(GLdouble ms)
{ updateInterval_ = ms; }

void SkyScattering::glAnimate(RenderState *rs, GLdouble dt)
{
  dt_ += dt;
  if(dt_<updateInterval_) { return; }
  update(rs,dt_);
  dt_ = 0.0;
}

void SkyScattering::update(RenderState *rs, GLdouble dt)
{
  static Vec3f frontVector(0.0,0.0,1.0);

  dayTime_ += dt*timeScale_;
  if(dayTime_>1.0) { dayTime_=fmod(dayTime_,1.0); }

  const GLdouble nighttime = 1.0 - dayLength_;
  const GLdouble dayStart = 0.5 - 0.5*dayLength_;
  const GLdouble nightStart = dayStart + dayLength_;
  GLdouble elevation;
  if(dayTime_>dayStart && dayTime_<nightStart) {
    GLdouble x = (dayTime_-dayStart)/dayLength_;
    elevation = maxSunElevation_*(1 - pow(2*x-1,4));
  }
  else {
    GLdouble x = ((dayTime_<0.5 ? dayTime_+1.0 : dayTime_) - nightStart)/nighttime;
    elevation = minSunElevation_*(1 - pow(2*x-1,4));
  }

  GLdouble sunAzimuth = dayTime_*M_PI*2.0;
  // sun rotation as seen from horizont space
  Mat4f sunRotation = Mat4f::rotationMatrix(elevation*M_PI/180.0, sunAzimuth, 0.0);

  // update light direction
  Vec3f sunDir = sunRotation.transform(frontVector);
  sunDir.normalize();
  sunDirection_->setVertex3f(0,sunDir);
  sun_->direction()->setVertex3f(0,sunDir);

  GLdouble nightFade = sunDir.y;
  if(nightFade < 0.0) { nightFade = 0.0; }
  // linear interpolate between day and night colors
  const Vec3f &dayColor = skyAbsorbtion_->getVertex3f(0);
  Vec3f color = Vec3f(1.0)-dayColor; // night color
  color = color*(1.0-nightFade) + dayColor*nightFade;
  sun_->diffuse()->setVertex3f(0,color * nightFade);

  rs->drawFrameBuffer().push(fbo_->id());
  rs->viewport().push(fbo_->glViewport());

  updateSky(rs);

  rs->viewport().pop();
  rs->drawFrameBuffer().pop();
}
void SkyScattering::updateSky(RenderState *rs)
{
  updateState_->enable(rs);
  updateState_->disable(rs);
}

