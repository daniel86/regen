/*
 * sky-box.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <climits>

#include "sky-box.h"
#include <ogle/states/render-state.h>
#include <ogle/states/cull-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/states/material-state.h>
#include <ogle/textures/cube-image-texture.h>
#include <ogle/textures/image-texture.h>

static const GLdouble degToRad = 2.0*M_PI/360.0;

static UnitCube::Config cubeCfg(GLfloat far)
{
  UnitCube::Config cfg;
  cfg.posScale = Vec3f(0.8*far/sqrt(2.0)); // XXX
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
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
///////////
///////////

DynamicSky::DynamicSky(
    ref_ptr<MeshState> orthoQuad,
    GLfloat far,
    GLuint cubeMapSize,
    GLboolean useFloatBuffer)
: SkyBox(far),
  Animation(),
  orthoQuad_(orthoQuad),
  dayTime_(0.4),
  timeScale_(0.00000004),
  updateInterval_(5000.0),
  dt_(0.0)
{
  moons_ = NULL;
  moonData_ = NULL;

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
  sun_->set_ambient(Vec3f(0.15f));
  sun_->set_diffuse(Vec3f(0.0f));
  sun_->set_direction(Vec3f(1.0f));
  sunDirection_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("sunDir"));
  sunDirection_->setUniformData(Vec3f(0.0f));
  sunDistance_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("sunDistance"));
  sunDistance_->setUniformData(0.0f);

  // create mvp matrix for each cube face
  mvpMatrices_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("mvpMatrices",6));
  mvpMatrices_->setVertexData(1,NULL);
  const Mat4f *views = getCubeLookAtMatrices();
  Mat4f proj = projectionMatrix(90.0, 1.0f, 0.1, 2.0);
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
  starMapRotation_->setUniformData(identity4f());

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
  updateState_->joinStates(ref_ptr<State>::cast(orthoQuad));

  // create shader based on configuration
  ShaderConfig shaderConfig;
  updateState_->configureShader(&shaderConfig);
  shaderConfig.define("HAS_GEOMETRY_SHADER", "TRUE");
  shaderConfig.setVersion("400");
  updateShader_->createShader(shaderConfig, "sky.scattering");

  dt_ = updateInterval_*1.01;
}
DynamicSky::~DynamicSky()
{
  glDeleteFramebuffers(1, &fbo_);
  if(moons_!=NULL) { delete []moons_; }
  if(moonData_!=NULL) { delete []moonData_; }
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

void DynamicSky::setEarth(GLdouble longitude, GLdouble latitude)
{
  PlanetProperties prop;
  prop.tilt = 23.5;
  prop.sunDistance = 1.0;
  prop.diameter = 12756.0;
  prop.longitude = longitude;
  prop.latitude = latitude;
  prop.rayleigh = Vec3f(19.0,359.0,81.0);
  prop.mie = Vec4f(44.0,308.0,39.0,74.0);
  prop.spot = 373.0;
  prop.scatterStrength = 54.0;
  prop.absorbtion = Vec3f(
      0.18867780436772762,
      0.4978442963618773,
      0.6616065586417131);
  setPlanetProperties(prop);

  MoonProperties *moons = new MoonProperties[1];
  moons[0].inclination = 5.14;
  moons[0].distance = 384400.0;
  moons[0].period = 27.322;
  moons[0].diameter = 3474.0;
  moons[0].color = Vec3f(1.0);
  moons[0].moonMap = "res/textures/moons/luna.png";
  setMoons(moons, 1);
}

void DynamicSky::setMars(GLdouble longitude, GLdouble latitude)
{
  PlanetProperties prop;
  prop.tilt = 25.5;
  prop.sunDistance = 1.67;
  prop.diameter = 6794.0;
  prop.longitude = longitude;
  prop.latitude = latitude;
  prop.rayleigh = Vec3f(33.0,139.0,81.0);
  prop.mie = Vec4f(100.0,264.0,39.0,63.0);
  prop.spot = 1000.0;
  prop.scatterStrength = 28.0;
  prop.absorbtion = Vec3f(0.66015625, 0.5078125, 0.1953125);
  setPlanetProperties(prop);

  MoonProperties *moons = new MoonProperties[2];
  // phobos
  moons[0].inclination = 1.1;
  moons[0].distance = 9380.0;
  moons[0].period = 0.319;
  moons[0].diameter = 27.0;
  moons[0].color = Vec3f(1.0);
  moons[0].moonMap = "res/textures/moons/phobos.png";
  // deimos
  moons[1].inclination = 1.8;
  moons[1].distance = 23460.0;
  moons[1].period = 1.26;
  moons[1].diameter = 15.0;
  moons[1].color = Vec3f(1.0);
  moons[1].moonMap = "res/textures/moons/deimos.png";
  setMoons(moons, 2);
}

void DynamicSky::setUranus(GLdouble longitude, GLdouble latitude)
{
  PlanetProperties prop;
  prop.tilt = 97.0;
  prop.sunDistance = 19.2;
  prop.diameter = 51120.0;
  prop.longitude = longitude;
  prop.latitude = latitude;
  prop.rayleigh = Vec3f(80.0,136.0,71.0);
  prop.mie = Vec4f(67.0,68.0,0.0,56.0);
  prop.spot = 0.0;
  prop.scatterStrength = 18.0;
  prop.absorbtion = Vec3f(0.26953125, 0.5234375, 0.8867187);
  setPlanetProperties(prop);
  setMoons(NULL, 0);
}

void DynamicSky::setVenus(GLdouble longitude, GLdouble latitude)
{
  PlanetProperties prop;
  prop.tilt = 177.0;
  prop.sunDistance = 0.723;
  prop.diameter = 12100.0;
  prop.longitude = longitude;
  prop.latitude = latitude;
  prop.rayleigh = Vec3f(25.0,397.0,34.0);
  prop.mie = Vec4f(124.0,298.0,76.0,81.0);
  prop.spot = 0.0;
  prop.scatterStrength = 140.0;
  prop.absorbtion = Vec3f(0.6640625, 0.5703125, 0.29296875);
  setPlanetProperties(prop);
  setMoons(NULL, 0);
}

void DynamicSky::setAlien(GLdouble longitude, GLdouble latitude)
{
  PlanetProperties prop;
  prop.tilt = 50.5;
  prop.sunDistance = 4.2;
  prop.diameter = 24000.0;
  prop.longitude = longitude;
  prop.latitude = latitude;
  prop.rayleigh = Vec3f(44.0,169.0,71.0);
  prop.mie = Vec4f(60.0,139.0,46.0,86.0);
  prop.spot = 0.0;
  prop.scatterStrength = 26.0;
  prop.absorbtion = Vec3f(0.24609375, 0.53125, 0.3515625);
  setPlanetProperties(prop);
  setMoons(NULL, 0);
}

void DynamicSky::setPlanetProperties(PlanetProperties &p)
{
  const GLdouble toAstroUnit = 1.0/149597871.0;

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

  sunDistance_->setVertex1f(0, p.sunDistance);
  planetDiameter_ = p.diameter*toAstroUnit;
  // find planet axis
  GLdouble tiltRad = 2.0*M_PI*p.tilt/360.0;
  planetAxis_ = Vec3f(sin(tiltRad), cos(tiltRad), 0.0);
  normalize(planetAxis_);
  // find axis of camera coordinate space
  Mat4f locRotation_ = xyzRotationMatrix(
      degToRad*p.latitude, degToRad*p.longitude, 0.0);
  yAxis_ = transformVec3(locRotation_, Vec3f(0.0,0.0,1.0));
  zAxis_ = transformVec3(locRotation_, Vec3f(1.0,0.0,0.0));
  timeOffset_ = (270.0 - p.longitude)/360.0;
  dt_ = updateInterval_*1.01;
}

/////////////
/////////////

void DynamicSky::setMoons(MoonProperties *moons, GLuint numMoons)
{
  numMoons_ = numMoons;
  if(moons_!=NULL) { delete []moons_; }
  if(moonData_!=NULL) { delete []moonData_; }
  if(moons==NULL) { return; }

  moons_ = moons;
  moonVertexSize_ = 0;
  moonVertexSize_ += sizeof(GLfloat)*4; // position+size
  moonVertexSize_ += sizeof(GLfloat)*3; // color
  moonData_ = new byte[moonVertexSize_*numMoons];

  GLuint vertexOffset = 0;
  moonDirection_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("moonPosition"));
  moonDirection_->set_offset(vertexOffset);
  moonDirection_->set_stride(moonVertexSize_);
  moonDirection_->set_numVertices(numMoons);
  vertexOffset += sizeof(GLfloat)*4;
  moonColor_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("moonColor"));
  moonColor_->set_offset(vertexOffset);
  moonColor_->set_stride(moonVertexSize_);
  moonColor_->set_numVertices(numMoons);
  vertexOffset += sizeof(GLfloat)*3;

  moonMaps_ = ref_ptr<Texture2DArray>::manage(new Texture2DArray(1));
  moonMaps_->bind();
  moonMaps_->set_filter(GL_LINEAR, GL_LINEAR);
  moonMaps_->set_numTextures(numMoons);
  GLuint layer=0;
  for(GLuint i=0; i<numMoons; ++i)
  {
    MoonProperties &m = moons[i];
    GLdouble tiltRad = 2.0*M_PI*m.inclination/360.0;
    m.axis = Vec3f(sin(tiltRad), cos(tiltRad), 0.0);
    // create texture array for moons
    m.texture = ref_ptr<Texture>::manage(new ImageTexture(m.moonMap));

    moonMaps_->bind();
    moonMaps_->set_format(m.texture->format());
    moonMaps_->set_internalFormat(m.texture->internalFormat());
    moonMaps_->set_pixelType(m.texture->pixelType());
    moonMaps_->set_size(m.texture->width(), m.texture->height());
    if(layer==0) {
      moonMaps_->texImage();
    }
    moonMaps_->texSubImage(layer, (GLubyte*)m.texture->data());
    layer +=  1;
  }

  moonState_ = ref_ptr<State>::manage(new State);
  moonState_->joinShaderInput(ref_ptr<ShaderInput>::cast(mvpMatrices_));
  moonState_->joinShaderInput(ref_ptr<ShaderInput>::cast(sunDirection_));
  moonState_->joinShaderInput(ref_ptr<ShaderInput>::cast(sunDistance_));
  moonState_->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));
  //moonState_->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  moonState_->joinStates(ref_ptr<State>::manage(new CullDisableState));
  moonShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  moonState_->joinStates(ref_ptr<State>::cast(moonShader_));
  // create the moon shader
  ShaderConfig shaderCfg;
  moonState_->configureShader(&shaderCfg);
  shaderCfg.define("HAS_GEOMETRY_SHADER", "TRUE");
  shaderCfg.setVersion("400");
  moonShader_->createShader(shaderCfg, "sky.moon");

  moonMapChannel_ = 0;
  moonShader_->shader()->setTexture(&moonMapChannel_, "moonColorTexture");
}

void DynamicSky::updateMoons(
    const Vec3f &cameraY,
    const Vec3f &cameraZ,
    const Vec3f &location)
{
  const GLuint bufferSize = moonVertexSize_*numMoons_;
  const GLdouble toAstroUnit = 1.0/149597871.0;

  Quaternion moonRotation;
  Vec3f *moonDataPtr = (Vec3f*)moonData_;
  for(GLuint i=0; i<numMoons_; ++i)
  {
    MoonProperties &m = moons_[i];
    // rotates moon around earth
    m.theta += dt_*timeScale_/m.period;
    if(m.theta>1.0) m.theta=fmod(m.theta,1.0);
    Vec3f p = Vec3f(0.0,0.0,m.distance);

    moonRotation.setAxisAngle(m.axis, m.theta*2.0*M_PI);
    Vec3f moonToPlanet = moonRotation.rotate(p);

    Vec3f locationToMoon = location - moonToPlanet;
    Mat4f transformToMoon = lookAtCameraInverse(
        getLookAtMatrix(locationToMoon, cameraZ, cameraY) );
    m.dir = transformVec3(transformToMoon, locationToMoon);
    normalize(m.dir);
    *moonDataPtr = m.dir * m.distance * toAstroUnit;
    ++moonDataPtr;

    GLfloat angularDiameter = atan( m.diameter / m.distance );
    GLfloat moonSize = sin(angularDiameter);
    GLfloat *floatDataPtr = (GLfloat*)moonDataPtr;
    *floatDataPtr = moonSize*10.0;
    ++floatDataPtr;
    moonDataPtr = (Vec3f*)floatDataPtr;

    // set the moon color
    *moonDataPtr = m.color;
    ++moonDataPtr;
  }

  GLuint moonVBO_;
  glGenBuffers(1, &moonVBO_);
  glBindBuffer(GL_ARRAY_BUFFER, moonVBO_);
  glBufferData(GL_ARRAY_BUFFER, bufferSize, moonData_, GL_STATIC_DRAW);
  moonDirection_->enable(0);
  moonColor_->enable(1);

  moonMaps_->activateBind(0);
  moonState_->enable(&rs_);
  glDrawArrays(GL_POINTS, 0, numMoons_);
  moonState_->disable(&rs_);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDeleteBuffers(1, &moonVBO_);
}

/////////////
/////////////

void DynamicSky::setStarMap(ref_ptr<TextureCube> &starMap)
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
  starMapState_->joinStates(ref_ptr<State>::cast(orthoQuad_));
  // create the star shader
  ShaderConfig shaderCfg;
  starMapState_->configureShader(&shaderCfg);
  shaderCfg.define("HAS_GEOMETRY_SHADER", "TRUE");
  shaderCfg.setVersion("400");
  starMapShader_->createShader(shaderCfg, "sky.starMap");
}

void DynamicSky::setStarMapBrightness(GLfloat brightness)
{
  starMapBrightness_->setVertex1f(0,brightness);
}
ref_ptr<ShaderInput1f>& DynamicSky::setStarMapBrightness()
{
  return starMapBrightness_;
}

void DynamicSky::updateStarMap()
{
  starMap_->activateBind(0);
  starMapState_->enable(&rs_);
  starMapState_->disable(&rs_);
}

/////////////
/////////////

void DynamicSky::animate(GLdouble dt)
{
}

void DynamicSky::updateGraphics(GLdouble dt)
{
  static Vec3f frontVector(0.0,0.0,1.0);

  dt_ += dt;
  if(dt_<updateInterval_) { return; }
  dayTime_ += dt_*timeScale_;
  if(dayTime_>1.0) { dayTime_=fmod(dayTime_,1.0); }

  GLdouble t = dayTime_ + timeOffset_;
  if(t>1.0) { t-=1.0; }
  else if(t<0.0) { t+=1.0; }

  Vec3f &sunDir = sunDirection_->getVertex3f(0);
  GLfloat &sunDistance = sunDistance_->getVertex1f(0);

  Quaternion planetRotation;
  planetRotation.setAxisAngle(planetAxis_, t*2.0*M_PI);
  Vec3f cameraY = planetRotation.rotate(yAxis_);
  Vec3f cameraZ = planetRotation.rotate(zAxis_);
  Vec3f location = 0.5*planetDiameter_*cameraY;

  // compute current sun position
  Vec3f sunToPlanet(sunDistance,0.0,0.0);
  // rotate planet using the tilt
  Vec3f locationToSun = location - sunToPlanet;
  Mat4f transformToSun = lookAtCameraInverse(
      getLookAtMatrix(locationToSun, cameraZ, cameraY) );
  sunDir = transformVec3(transformToSun, locationToSun);
  normalize(sunDir);
  sun_->set_direction(sunDir);

  GLdouble nightFade = sunDir.y;
  if(nightFade < 0.0) { nightFade = 0.0; }
  // linear interpolate between day and night colors
  Vec3f &dayColor = skyAbsorbtion_->getVertex3f(0);
  Vec3f color = Vec3f(1.0)-dayColor; // night color
  color = color*(1.0-nightFade) + dayColor*nightFade;
  sun_->set_diffuse(color * nightFade);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, cubeMap_->width(), cubeMap_->height());
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  updateSky();
  if(numMoons_>0) {
    updateMoons(cameraY, cameraZ, location);
    glBindBuffer(GL_ARRAY_BUFFER, orthoQuad_->vertexBuffer());
  }
  if(starMap_.get()!=NULL) {
    // star map is blended using dst alpha, stars appear behind the moon, sun and
    // bright day light
    updateStarMap();
  }
  dt_ = 0.0;
}

void DynamicSky::updateSky()
{
  updateState_->enable(&rs_);
  updateState_->disable(&rs_);
}

