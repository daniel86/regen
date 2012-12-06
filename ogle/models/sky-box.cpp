/*
 * sky-box.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "sky-box.h"
#include <ogle/states/render-state.h>
#include <ogle/states/cull-state.h>
#include <ogle/states/material-state.h>

static UnitCube::Config cubeCfg(GLfloat far)
{
  UnitCube::Config cfg;
  cfg.posScale = Vec3f(far);
  cfg.isNormalRequired = GL_FALSE;
  cfg.texcoMode = UnitCube::TEXCO_MODE_CUBE_MAP;
  return cfg;
}

SkyBox::SkyBox(GLfloat far)
: UnitCube(cubeCfg(far))
{
  joinStates(ref_ptr<State>::manage(new CullFrontFaceState));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->set_shading(Material::NO_SHADING);
  material->setConstantUniforms(GL_TRUE);
  joinStates(ref_ptr<State>::cast(material));
}

void SkyBox::setCubeMap(ref_ptr<TextureCube> &cubeMap)
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
ref_ptr<TextureCube>& SkyBox::cubeMap()
{
  return cubeMap_;
}

void SkyBox::resize(GLfloat far)
{
  updateAttributes(cubeCfg(far));
}

void SkyBox::configureShader(ShaderConfig *cfg)
{
  UnitCube::configureShader(cfg);
  cfg->setIgnoreCameraTranslation();
}

void SkyBox::enable(RenderState *rs)
{
  ignoredViewRotation_ = rs->ignoreViewRotation();
  rs->set_ignoreViewRotation(GL_TRUE);
  UnitCube::enable(rs);
}
void SkyBox::disable(RenderState *rs)
{
  rs->set_ignoreViewRotation(ignoredViewRotation_);
  UnitCube::disable(rs);
}

///////////

SkyAtmosphere::SkyAtmosphere(
    ref_ptr<MeshState> orthoQuad, GLfloat far, GLuint cubeMapSize)
: SkyBox(far),
  Animation(),
  dayTime_(0.5),
  timeScale_(0.0001),
  updateInterval_(40.0),
  dt_(0.0)
{
  ref_ptr<TextureCube> cubeMap = ref_ptr<TextureCube>::manage(new TextureCube(1));
  cubeMap->bind();
  cubeMap->set_format(GL_RGB);
  cubeMap->set_internalFormat(GL_RGB);
  // TODO: float texture?
  //cubeMap->set_internalFormat(GL_RGB16F);
#ifdef USE_MIPMAPPING
  cubeMap->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
#else
  cubeMap->set_filter(GL_LINEAR, GL_LINEAR);
#endif
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
  sun_->set_ambient(Vec3f(0.15f));
  sun_->set_diffuse(Vec3f(0.0f));
  sun_->set_direction(Vec3f(1.0f));

  // init uniforms.
  // TODO: allow user to switch to const
  lightDir_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightDir"));
  lightDir_->setUniformData(Vec3f(0.0f));
  rayleigh_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("rayleigh"));
  rayleigh_->setUniformData(Vec3f(0.0f));
  mie_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("mie"));
  mie_->setUniformData(Vec4f(0.0f));
  spotBrightness_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("spotBrightness"));
  spotBrightness_->setUniformData(0.0f);
  scatterStrength_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("scatterStrength"));
  scatterStrength_->setUniformData(0.0f);
  skyAbsorbtion_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("skyColor"));
  skyAbsorbtion_->setUniformData(Vec3f(0.0f));

  updateState_ = ref_ptr<State>::manage(new State);
  // upload uniforms
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(lightDir_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(rayleigh_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(mie_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(spotBrightness_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(scatterStrength_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(skyAbsorbtion_));
  // enable shader
  updateShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  updateState_->joinStates(ref_ptr<State>::cast(updateShader_));
  // do the draw call
  updateState_->joinStates(ref_ptr<State>::cast(orthoQuad));

  // create shader based on configuration
  ShaderConfig shaderConfig;
  updateState_->configureShader(&shaderConfig);
  shaderConfig.define("HAS_GEOMETRY_SHADER", "TRUE");
  shaderConfig.setVersion("400");
  updateShader_->createShader(shaderConfig, "scattering");

  setSunElevation(0.5, 35.0, -35.0, 0.0);
  //setPolarDay();
  //setEquator(GL_TRUE);
  setEarth();
}

void SkyAtmosphere::setSunElevation(
    GLdouble time,
    GLdouble maxAngle,
    GLdouble minAngle,
    GLdouble orientation)
{
  maxElevationTime_ = time;
  maxElevationAngle_ = maxAngle*2.0*M_PI/360.0;
  minElevationAngle_ = minAngle*2.0*M_PI/360.0;
  maxElevationOrientation_ = orientation*2.0*M_PI/360.0;
}

void SkyAtmosphere::set_updateInterval(GLdouble ms)
{
  updateInterval_ = ms;
}

void SkyAtmosphere::set_dayTime(GLdouble time)
{
  dayTime_ = time;
}

void SkyAtmosphere::set_timeScale(GLdouble scale)
{
  timeScale_ = scale;
}

ref_ptr<DirectionalLight>& SkyAtmosphere::sun()
{
  return sun_;
}

void SkyAtmosphere::setRayleighBrightness(GLfloat v)
{
  rayleigh_->getVertex3f(0).x = v/10.0;
}
void SkyAtmosphere::setRayleighStrength(GLfloat v)
{
  rayleigh_->getVertex3f(0).y = v/1000.0;
}
void SkyAtmosphere::setRayleighCollect(GLfloat v)
{
  rayleigh_->getVertex3f(0).z = v/100.0;
}
ref_ptr<ShaderInput3f>& SkyAtmosphere::rayleigh()
{
  return rayleigh_;
}

void SkyAtmosphere::setMieBrightness(GLfloat v)
{
  mie_->getVertex4f(0).x = v/1000.0;
}
void SkyAtmosphere::setMieStrength(GLfloat v)
{
  mie_->getVertex4f(0).y = v/10000.0;
}
void SkyAtmosphere::setMieCollect(GLfloat v)
{
  mie_->getVertex4f(0).z = v/100.0;
}
void SkyAtmosphere::setMieDistribution(GLfloat v)
{
  mie_->getVertex4f(0).w = v/100.0;
}
ref_ptr<ShaderInput4f>& SkyAtmosphere::mie()
{
  return mie_;
}

void SkyAtmosphere::setSpotBrightness(GLfloat v)
{
  spotBrightness_->setVertex1f(0, v);
}
ref_ptr<ShaderInput1f>& SkyAtmosphere::spotBrightness()
{
  return spotBrightness_;
}

void SkyAtmosphere::setScatterStrength(GLfloat v)
{
  scatterStrength_->setVertex1f(0, v/1000.0);
}
ref_ptr<ShaderInput1f>& SkyAtmosphere::scatterStrength()
{
  return scatterStrength_;
}

void SkyAtmosphere::setAbsorbtion(const Vec3f &color)
{
  skyAbsorbtion_->setVertex3f(0, color);
}
ref_ptr<ShaderInput3f>& SkyAtmosphere::skyColor()
{
  return skyAbsorbtion_;
}

void SkyAtmosphere::setEarth()
{
  setRayleighBrightness(19.0);
  setRayleighStrength(359.0);
  setRayleighCollect(81.0);
  setMieBrightness(44.0);
  setMieStrength(308.0);
  setMieCollect(39.0);
  setMieDistribution(74.0);
  setSpotBrightness(373.0);
  setScatterStrength(54.0);
  setAbsorbtion(Vec3f(
      0.18867780436772762,
      0.4978442963618773,
      0.6616065586417131));
}

void SkyAtmosphere::setMars()
{
  setRayleighBrightness(33.0);
  setRayleighStrength(139.0);
  setRayleighCollect(81.0);
  setMieBrightness(100.0);
  setMieStrength(264.0);
  setMieCollect(39.0);
  setMieDistribution(63.0);
  setSpotBrightness(1000.0);
  setScatterStrength(28.0);
  setAbsorbtion(Vec3f(0.66015625, 0.5078125, 0.1953125));
}

void SkyAtmosphere::setUranus()
{
  setRayleighBrightness(80.0);
  setRayleighStrength(136.0);
  setRayleighCollect(71.0);
  setMieBrightness(67.0);
  setMieStrength(68.0);
  setMieCollect(0.0);
  setMieDistribution(56.0);
  setSpotBrightness(0.0);
  setScatterStrength(18.0);
  setAbsorbtion(Vec3f(0.26953125, 0.5234375, 0.8867187));
}

void SkyAtmosphere::setVenus()
{
  setRayleighBrightness(25.0);
  setRayleighStrength(397.0);
  setRayleighCollect(34.0);
  setMieBrightness(124.0);
  setMieStrength(298.0);
  setMieCollect(76.0);
  setMieDistribution(81.0);
  setSpotBrightness(0.0);
  setScatterStrength(140.0);
  setAbsorbtion(Vec3f(0.6640625, 0.5703125, 0.29296875));
}

void SkyAtmosphere::setAlien()
{
  setRayleighBrightness(44.0);
  setRayleighStrength(169.0);
  setRayleighCollect(71.0);
  setMieBrightness(60.0);
  setMieStrength(139.0);
  setMieCollect(46.0);
  setMieDistribution(86.0);
  setSpotBrightness(0.0);
  setScatterStrength(26.0);
  setAbsorbtion(Vec3f(0.24609375, 0.53125, 0.3515625));
}

void SkyAtmosphere::animate(GLdouble dt)
{
}

void SkyAtmosphere::updateGraphics(GLdouble dt)
{
  static Vec3f frontVector(0.0,0.0,1.0);

  dt_ += dt;
  if(dt_<updateInterval_) { return; }
  dayTime_ += dt_*timeScale_;
  if(dayTime_>1.0) { dayTime_=fmod(dayTime_,1.0); }
  dt_ = 0.0;

  // compute current sun position
  {
    GLdouble timeDiff = maxElevationTime_-dayTime_;
    GLdouble sunAzimuth = maxElevationOrientation_ + timeDiff*M_PI*2.0;
    GLdouble sunAltitude = minElevationAngle_ +
        (maxElevationAngle_ - minElevationAngle_)*cos(timeDiff*M_PI);
    // sun rotation as seen from horizont space
    Mat4f sunRotation = xyzRotationMatrix(sunAltitude, sunAzimuth, 0.0);

    // update light direction
    Vec3f &lightDir = lightDir_->getVertex3f(0);
    lightDir = transformVec3(sunRotation, frontVector);
    normalize(lightDir);
    sun_->set_direction(lightDir);

    // update diffuse color based on elevation
    Vec3f &dayColor = skyAbsorbtion_->getVertex3f(0);
    Vec3f color = Vec3f(1.0)-dayColor; // night color
    GLdouble nightFade = abs(timeDiff*2.0);
    // linear interpolate between day and night colors
    color = color*nightFade + dayColor*(1.0-nightFade);
    // change diffuse color brightness depending on elevation
    GLdouble brigthness = -exp(-5.0*sunAltitude/maxElevationAngle_) + 1.0;
    if(brigthness < 0.0) { brigthness = 0.0; }
    sun_->set_diffuse(color * brigthness);
  }

  updateSky();
}

void SkyAtmosphere::updateSky()
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, cubeMap_->width(), cubeMap_->height());
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  updateState_->enable(&rs_);
  updateState_->disable(&rs_);

#ifdef USE_MIPMAPPING
  cubeMap_->setupMipmaps(GL_DONT_CARE);
#endif
}
