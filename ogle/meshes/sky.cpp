/*
 * sky-box.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <climits>

#include "sky.h"
#include <ogle/meshes/rectangle.h>
#include <ogle/states/atomic-states.h>
#include <ogle/states/depth-state.h>
#include <ogle/states/shader-configurer.h>

static const GLdouble degToRad = 2.0*M_PI/360.0;

static Box::Config cubeCfg()
{
  Box::Config cfg;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.texcoMode = Box::TEXCO_MODE_CUBE_MAP;
  return cfg;
}

SkyBox::SkyBox()
: Box(cubeCfg())
{
  joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));

  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_depthFunc(GL_LEQUAL);
  joinStates(ref_ptr<State>::cast(depth));

  shaderDefine("IGNORE_VIEW_TRANSLATION", "TRUE");
}

void SkyBox::setCubeMap(const ref_ptr<TextureCube> &cubeMap)
{
  cubeMap_ = cubeMap;
  if(texState_.get()) {
    disjoinStates(ref_ptr<State>::cast(texState_));
  }
  texState_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(cubeMap_)));
  texState_->setMapTo(MAP_TO_COLOR);
  joinStates(ref_ptr<State>::cast(texState_));
}
const ref_ptr<TextureCube>& SkyBox::cubeMap() const
{
  return cubeMap_;
}

///////////
///////////
///////////

DynamicSky::DynamicSky(GLuint cubeMapSize, GLboolean useFloatBuffer)
: SkyBox(),
  Animation(),
  dayTime_(0.4),
  timeScale_(0.00000004),
  updateInterval_(4000.0),
  dt_(0.0)
{
  dayLength_ = 0.8;
  maxSunElevation_ = 30.0;
  minSunElevation_ = -20.0;

  ref_ptr<TextureCube> cubeMap = ref_ptr<TextureCube>::manage(new TextureCube(1));
  cubeMap->bind();
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

  // create render target for updating the sky cube map
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  // clear negative y to black, -y cube face is not updated
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, cubeMap->id(), 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glClearColor(0.0f,0.0f,0.0f,0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  // for updating bind all layers to GL_COLOR_ATTACHMENT0
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cubeMap->id(), 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // directional light that approximates the sun
  sun_ = ref_ptr<DirectionalLight>::manage(new DirectionalLight);
  sun_->set_isAttenuated(GL_FALSE);
  sun_->set_specular(Vec3f(0.0f));
  sun_->set_diffuse(Vec3f(0.0f));
  sun_->set_direction(Vec3f(1.0f));
  sunDirection_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("sunDir"));
  sunDirection_->setUniformData(Vec3f(0.0f));

  // create mvp matrix for each cube face
  mvpMatrices_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("mvpMatrices",6));
  mvpMatrices_->setVertexData(1,NULL);
  const Mat4f *views = getCubeLookAtMatrices();
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
  ///////
  /// Star map uniforms
  ///////
  starMapBrightness_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("starMapBrightness"));
  starMapBrightness_->setUniformData(1.0);
  starMapRotation_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("starMapRotation"));
  starMapRotation_->setUniformData(Mat4f::identity());

  updateState_ = ref_ptr<State>::manage(new State);
  // upload uniforms
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(sunDirection_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(rayleigh_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(mie_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(spotBrightness_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(scatterStrength_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(skyAbsorbtion_));
  // enable shader
  updateShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  updateState_->joinStates(ref_ptr<State>::cast(updateShader_));
  // do the draw call
  updateState_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));

  // create shader based on configuration
  ShaderConfig shaderConfig = ShaderConfigurer::configure(updateState_.get());
  shaderConfig.setVersion(400);
  updateShader_->createShader(shaderConfig, "sky.scattering");

  dt_ = updateInterval_*1.01;
}
DynamicSky::~DynamicSky()
{
  glDeleteFramebuffers(1, &fbo_);
}

void DynamicSky::set_updateInterval(GLdouble ms)
{
  updateInterval_ = ms;
  dt_ = updateInterval_*1.01;
}

void DynamicSky::set_dayTime(GLdouble time)
{
  dayTime_ = time;
}

void DynamicSky::set_timeScale(GLdouble scale)
{
  timeScale_ = scale;
}

ref_ptr<DirectionalLight>& DynamicSky::sun()
{
  return sun_;
}

void DynamicSky::setSunElevation(GLdouble dayLength,
      GLdouble maxElevation,
      GLdouble minElevation)
{
  dayLength_ = dayLength;
  maxSunElevation_ = maxElevation;
  minSunElevation_ = minElevation;
}

void DynamicSky::setRayleighBrightness(GLfloat v)
{
  rayleigh_->getVertex3f(0).x = v/10.0;
}
void DynamicSky::setRayleighStrength(GLfloat v)
{
  rayleigh_->getVertex3f(0).y = v/1000.0;
}
void DynamicSky::setRayleighCollect(GLfloat v)
{
  rayleigh_->getVertex3f(0).z = v/100.0;
}
ref_ptr<ShaderInput3f>& DynamicSky::rayleigh()
{
  return rayleigh_;
}

void DynamicSky::setMieBrightness(GLfloat v)
{
  mie_->getVertex4f(0).x = v/1000.0;
}
void DynamicSky::setMieStrength(GLfloat v)
{
  mie_->getVertex4f(0).y = v/10000.0;
}
void DynamicSky::setMieCollect(GLfloat v)
{
  mie_->getVertex4f(0).z = v/100.0;
}
void DynamicSky::setMieDistribution(GLfloat v)
{
  mie_->getVertex4f(0).w = v/100.0;
}
ref_ptr<ShaderInput4f>& DynamicSky::mie()
{
  return mie_;
}

void DynamicSky::setSpotBrightness(GLfloat v)
{
  spotBrightness_->setVertex1f(0, v);
}
ref_ptr<ShaderInput1f>& DynamicSky::spotBrightness()
{
  return spotBrightness_;
}

void DynamicSky::setScatterStrength(GLfloat v)
{
  scatterStrength_->setVertex1f(0, v/1000.0);
}
ref_ptr<ShaderInput1f>& DynamicSky::scatterStrength()
{
  return scatterStrength_;
}

void DynamicSky::setAbsorbtion(const Vec3f &color)
{
  skyAbsorbtion_->setVertex3f(0, color);
}
ref_ptr<ShaderInput3f>& DynamicSky::absorbtion()
{
  return skyAbsorbtion_;
}

void DynamicSky::setEarth()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(19.0,359.0,81.0);
  prop.mie = Vec4f(44.0,308.0,39.0,74.0);
  prop.spot = 373.0;
  prop.scatterStrength = 54.0;
  prop.absorbtion = Vec3f(
      0.18867780436772762,
      0.4978442963618773,
      0.6616065586417131);
  setPlanetProperties(prop);
}

void DynamicSky::setMars()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(33.0,139.0,81.0);
  prop.mie = Vec4f(100.0,264.0,39.0,63.0);
  prop.spot = 1000.0;
  prop.scatterStrength = 28.0;
  prop.absorbtion = Vec3f(0.66015625, 0.5078125, 0.1953125);
  setPlanetProperties(prop);
}

void DynamicSky::setUranus()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(80.0,136.0,71.0);
  prop.mie = Vec4f(67.0,68.0,0.0,56.0);
  prop.spot = 0.0;
  prop.scatterStrength = 18.0;
  prop.absorbtion = Vec3f(0.26953125, 0.5234375, 0.8867187);
  setPlanetProperties(prop);
}

void DynamicSky::setVenus()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(25.0,397.0,34.0);
  prop.mie = Vec4f(124.0,298.0,76.0,81.0);
  prop.spot = 0.0;
  prop.scatterStrength = 140.0;
  prop.absorbtion = Vec3f(0.6640625, 0.5703125, 0.29296875);
  setPlanetProperties(prop);
}

void DynamicSky::setAlien()
{
  PlanetProperties prop;
  prop.rayleigh = Vec3f(44.0,169.0,71.0);
  prop.mie = Vec4f(60.0,139.0,46.0,86.0);
  prop.spot = 0.0;
  prop.scatterStrength = 26.0;
  prop.absorbtion = Vec3f(0.24609375, 0.53125, 0.3515625);
  setPlanetProperties(prop);
}

void DynamicSky::setPlanetProperties(PlanetProperties &p)
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
  setAbsorbtion(p.absorbtion);
  dt_ = updateInterval_*1.01;
}

/////////////
/////////////

void DynamicSky::setStarMap(ref_ptr<Texture> starMap)
{
  starMap_ = starMap;

  starMapState_ = ref_ptr<State>::manage(new State);
  starMapState_->joinShaderInput(ref_ptr<ShaderInput>::cast(sunDirection_));
  starMapState_->joinShaderInput(ref_ptr<ShaderInput>::cast(skyAbsorbtion_));
  starMapState_->joinShaderInput(ref_ptr<ShaderInput>::cast(rayleigh_));
  starMapState_->joinShaderInput(ref_ptr<ShaderInput>::cast(scatterStrength_));
  starMapState_->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_BACK_TO_FRONT)));
  starMapState_->joinShaderInput(ref_ptr<ShaderInput>::cast(starMapRotation_));
  starMapState_->joinShaderInput(ref_ptr<ShaderInput>::cast(starMapBrightness_));
  starMapShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  starMapState_->joinStates(ref_ptr<State>::cast(starMapShader_));
  starMapState_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
  // create the star shader
  ShaderConfig shaderConfig = ShaderConfigurer::configure(starMapState_.get());
  shaderConfig.setVersion(400);
  starMapShader_->createShader(shaderConfig, "sky.starMap");
}

void DynamicSky::setStarMapBrightness(GLfloat brightness)
{
  starMapBrightness_->setVertex1f(0,brightness);
}
ref_ptr<ShaderInput1f>& DynamicSky::setStarMapBrightness()
{
  return starMapBrightness_;
}

void DynamicSky::updateStarMap(RenderState *rs)
{
  starMap_->activate(0);
  starMapState_->enable(rs);
  starMapState_->disable(rs);
}

/////////////
/////////////

void DynamicSky::glAnimate(RenderState *rs, GLdouble dt)
{
  static Vec3f frontVector(0.0,0.0,1.0);

  dt_ += dt;
  if(dt_<updateInterval_) { return; }
  dayTime_ += dt_*timeScale_;
  if(dayTime_>1.0) { dayTime_=fmod(dayTime_,1.0); }

  Vec3f &sunDir = sunDirection_->getVertex3f(0);

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
  sunDir = sunRotation.transform(frontVector);
  sunDir.normalize();
  sun_->set_direction(sunDir);

  GLdouble nightFade = sunDir.y;
  if(nightFade < 0.0) { nightFade = 0.0; }
  // linear interpolate between day and night colors
  Vec3f &dayColor = skyAbsorbtion_->getVertex3f(0);
  Vec3f color = Vec3f(1.0)-dayColor; // night color
  color = color*(1.0-nightFade) + dayColor*nightFade;
  sun_->set_diffuse(color * nightFade);

  // XXX: use render state
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, cubeMap_->width(), cubeMap_->height());
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  updateSky(rs);
  if(starMap_.get()!=NULL) {
    // star map is blended using dst alpha, stars appear behind the moon, sun and
    // bright day light
    updateStarMap(rs);
  }
  dt_ = 0.0;
}
void DynamicSky::animate(GLdouble dt){}
GLboolean DynamicSky::useGLAnimation() const {
  return GL_TRUE;
}
GLboolean DynamicSky::useAnimation() const {
  return GL_FALSE;
}

void DynamicSky::updateSky(RenderState *rs)
{
  updateState_->enable(rs);
  updateState_->disable(rs);
}

