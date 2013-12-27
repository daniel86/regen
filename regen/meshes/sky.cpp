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
#include <regen/states/state-configurer.h>

#include "sky.h"
using namespace regen;

static Box::Config cubeCfg(GLuint levelOfDetail)
{
  Box::Config cfg;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.texcoMode = Box::TEXCO_MODE_CUBE_MAP;
  cfg.usage = VBO::USAGE_STATIC;
  cfg.levelOfDetail = levelOfDetail;
  return cfg;
}

SkyBox::SkyBox(GLuint levelOfDetail)
: Box(cubeCfg(levelOfDetail)), HasShader("regen.models.sky-box")
{
  joinStates(ref_ptr<CullFaceState>::alloc(GL_FRONT));

  ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
  depth->set_depthFunc(GL_LEQUAL);
  joinStates(depth);

  joinStates(shaderState());

  shaderDefine("IGNORE_VIEW_TRANSLATION", "TRUE");
}

void SkyBox::setCubeMap(const ref_ptr<TextureCube> &cubeMap)
{
  cubeMap_ = cubeMap;
  if(texState_.get()) {
    disjoinStates(texState_);
  }
  texState_ = ref_ptr<TextureState>::alloc(cubeMap_);
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

SkyScattering::SkyScattering(
    GLuint cubeMapSize,
    GLboolean useFloatBuffer,
    GLuint levelOfDetail)
: SkyBox(levelOfDetail),
  Animation(GL_TRUE,GL_FALSE),
  dayTime_(0.4),
  timeScale_(0.00000004)
{
  dayLength_ = 0.8;
  maxSunElevation_ = 30.0;
  minSunElevation_ = -20.0;
  updateInterval_ = 4000.0;
  dt_ = updateInterval_;

  ref_ptr<TextureCube> cubeMap = ref_ptr<TextureCube>::alloc(1);
  RenderState *rs = RenderState::get();

  cubeMap->begin(rs);
  cubeMap->set_format(GL_RGBA);
  if(useFloatBuffer) {
    cubeMap->set_internalFormat(GL_RGBA16F);
  } else {
    cubeMap->set_internalFormat(GL_RGBA);
  }
  cubeMap->filter().push(GL_LINEAR);
  cubeMap->set_rectangleSize(cubeMapSize,cubeMapSize);
  cubeMap->wrapping().push(GL_CLAMP_TO_EDGE);
  cubeMap->texImage();
  setCubeMap(cubeMap);
  cubeMap->end(rs);

  // create render target for updating the sky cube map
  fbo_ = ref_ptr<FBO>::alloc(cubeMapSize,cubeMapSize);
  rs->drawFrameBuffer().push(fbo_->id());
  fbo_->drawBuffers().push(DrawBuffers::attachment0());
  // clear negative y to black, -y cube face is not updated
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, cubeMap->id(), 0);
  glClear(GL_COLOR_BUFFER_BIT);
  // for updating bind all layers to GL_COLOR_ATTACHMENT0
  glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cubeMap->id(), 0);
  rs->drawFrameBuffer().pop();

  // directional light that approximates the sun
  sun_ = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
  sun_->set_isAttenuated(GL_FALSE);
  sun_->specular()->setVertex(0,Vec3f(0.0f));
  sun_->diffuse()->setVertex(0,Vec3f(0.0f));
  sun_->direction()->setVertex(0,Vec3f(1.0f));
  sunDirection_ = ref_ptr<ShaderInput3f>::alloc("sunDir");
  sunDirection_->setUniformData(Vec3f(0.0f));

  // create mvp matrix for each cube face
  mvpMatrices_ = ref_ptr<ShaderInputMat4>::alloc("mvpMatrices",6);
  mvpMatrices_->setVertexData(1,NULL);
  const Mat4f *views = Mat4f::cubeLookAtMatrices();
  Mat4f proj = Mat4f::projectionMatrix(90.0, 1.0f, 0.1, 2.0);
  for(register GLuint i=0; i<6; ++i) {
    mvpMatrices_->setVertex(i, views[i] * proj);
  }

  ///////
  /// Scattering uniforms
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

  updateState_ = ref_ptr<State>::alloc();
  // upload uniforms
  updateState_->joinShaderInput(sunDirection_);
  updateState_->joinShaderInput(rayleigh_);
  updateState_->joinShaderInput(mie_);
  updateState_->joinShaderInput(spotBrightness_);
  updateState_->joinShaderInput(scatterStrength_);
  updateState_->joinShaderInput(skyAbsorbtion_);

  updateShader_ = ref_ptr<ShaderState>::alloc();
  ref_ptr<Mesh> mesh = Rectangle::getUnitQuad();

  updateState_->joinStates(updateShader_);
  updateState_->joinStates(mesh);

  // create shader based on configuration
  StateConfig shaderConfig = StateConfigurer::configure(updateState_.get());
  shaderConfig.setVersion(330);
  updateShader_->createShader(shaderConfig, "regen.filter.scattering");
  mesh->updateVAO(rs, shaderConfig, updateShader_->shader());
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
  const Vec3f &rayleigh = rayleigh_->getVertex(0);
  rayleigh_->setVertex(0, Vec3f(v/10.0, rayleigh.y, rayleigh.z));
}
void SkyScattering::setRayleighStrength(GLfloat v)
{
  const Vec3f &rayleigh = rayleigh_->getVertex(0);
  rayleigh_->setVertex(0, Vec3f(rayleigh.x, v/1000.0, rayleigh.z));
}
void SkyScattering::setRayleighCollect(GLfloat v)
{
  const Vec3f &rayleigh = rayleigh_->getVertex(0);
  rayleigh_->setVertex(0, Vec3f(rayleigh.x, rayleigh.y, v/100.0));
}
ref_ptr<ShaderInput3f>& SkyScattering::rayleigh()
{ return rayleigh_; }

void SkyScattering::setMieBrightness(GLfloat v)
{
  const Vec4f &mie = mie_->getVertex(0);
  mie_->setVertex(0, Vec4f(v/1000.0, mie.y, mie.z, mie.w));
}
void SkyScattering::setMieStrength(GLfloat v)
{
  const Vec4f &mie = mie_->getVertex(0);
  mie_->setVertex(0, Vec4f(mie.x, v/10000.0, mie.z, mie.w));
}
void SkyScattering::setMieCollect(GLfloat v)
{
  const Vec4f &mie = mie_->getVertex(0);
  mie_->setVertex(0, Vec4f(mie.x, mie.y, v/100.0, mie.w));
}
void SkyScattering::setMieDistribution(GLfloat v)
{
  const Vec4f &mie = mie_->getVertex(0);
  mie_->setVertex(0, Vec4f(mie.x, mie.y, mie.z, v/100.0));
}
ref_ptr<ShaderInput4f>& SkyScattering::mie()
{ return mie_; }

void SkyScattering::setSpotBrightness(GLfloat v)
{
  spotBrightness_->setVertex(0, v);
}
ref_ptr<ShaderInput1f>& SkyScattering::spotBrightness()
{ return spotBrightness_; }

void SkyScattering::setScatterStrength(GLfloat v)
{
  scatterStrength_->setVertex(0, v/1000.0);
}
ref_ptr<ShaderInput1f>& SkyScattering::scatterStrength()
{ return scatterStrength_; }

void SkyScattering::setAbsorbtion(const Vec3f &color)
{
  skyAbsorbtion_->setVertex(0, color);
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
  GL_ERROR_LOG();
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
  Vec3f sunDir = sunRotation.transformVector(frontVector);
  sunDir.normalize();
  sunDirection_->setVertex(0,sunDir);
  sun_->direction()->setVertex(0,sunDir);

  GLdouble nightFade = sunDir.y;
  if(nightFade < 0.0) { nightFade = 0.0; }
  // linear interpolate between day and night colors
  const Vec3f &dayColor = skyAbsorbtion_->getVertex(0);
  Vec3f color = Vec3f(1.0)-dayColor; // night color
  color = color*(1.0-nightFade) + dayColor*nightFade;
  sun_->diffuse()->setVertex(0,color);

  rs->drawFrameBuffer().push(fbo_->id());
  rs->viewport().push(fbo_->glViewport());

  updateSky(rs);

  rs->viewport().pop();
  rs->drawFrameBuffer().pop();
  GL_ERROR_LOG();
}
void SkyScattering::updateSky(RenderState *rs)
{
  updateState_->enable(rs);
  updateState_->disable(rs);
}

