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
#include <ogle/states/material-state.h>
#include <ogle/textures/cube-image-texture.h>

static const GLdouble degToRad = 2.0*M_PI/360.0;

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
///////////
///////////

#define SUN_MAX_ELEVATION  45.0
#define SUN_MIN_ELEVATION -65.0

DynamicSky::DynamicSky(
    ref_ptr<MeshState> orthoQuad,
    GLfloat far, GLuint cubeMapSize)
: SkyBox(far),
  Animation(),
  dayTime_(0.5),
  timeScale_(0.00001),
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

  // XXX glDeleteFramebuffers missing
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
  planetRotation_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("planetRotation"));
  planetRotation_->setUniformData(identity4f());
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
  skyAbsorbtion_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("skyAbsorbtion"));
  skyAbsorbtion_->setUniformData(Vec3f(0.0f));
  starVisibility_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("brightStarVisibility"));
  starVisibility_->setUniformData(0.0f);
  starMapChannel_ = 0;
  milkywayVisibility_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("milkyWayVisibility"));
  milkywayVisibility_->setUniformData(0.0f);
  milkyWayMapChannel_ = 1;

  updateState_ = ref_ptr<State>::manage(new State);
  // upload uniforms
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(planetRotation_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(lightDir_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(rayleigh_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(mie_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(spotBrightness_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(scatterStrength_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(skyAbsorbtion_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(starVisibility_));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(milkywayVisibility_));
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

  if(1) {
    starBrightness_ = 1.0f;
    milkyWayBrightness_ = 0.05f;

    // TODO: config...
    ref_ptr<StarSkyMap> stars = ref_ptr<StarSkyMap>::manage(new StarSkyMap(256));
    stars->readStarFile("res/stars.bin", 9110);
    stars->update();
    setBrightStarMap(ref_ptr<TextureCube>::cast(stars));

    ref_ptr<TextureCube> milkyway = ref_ptr<TextureCube>::manage(
        new CubeImageTexture("res/textures/cube-milkyway.png", GL_RGB, GL_FALSE));
    milkyway->set_wrapping(GL_CLAMP_TO_EDGE);
    setMilkyWayMap(milkyway);
  }

  setSunElevation(0.6, SUN_MAX_ELEVATION, SUN_MIN_ELEVATION, 0.0);
  setEarth();
}

void DynamicSky::setMilkyWayMap(const ref_ptr<TextureCube> &milkyWayMap)
{
  if(milkyWayMap.get()!=NULL) {
    updateShader_->shader()->setTexture(&milkyWayMapChannel_, "milkyWayMap");
  } else {
    milkywayVisibility_->setVertex1f(0, 0.0f);
    updateShader_->shader()->setTexture(NULL, "milkyWayMap");
  }
  milkyWayMap_ = milkyWayMap;
}

void DynamicSky::setBrightStarMap(const ref_ptr<TextureCube> &starMap)
{
  if(starMap.get()!=NULL) {
    starVisibility_->setVertex1f(0, 1.0f);
    updateShader_->shader()->setTexture(&starMapChannel_, "brightStarMap");
  } else {
    starVisibility_->setVertex1f(0, 0.0f);
    updateShader_->shader()->setTexture(NULL, "brightStarMap");
  }
  brightStarMap_ = starMap;
}

void DynamicSky::setStarBrightness(GLfloat brightness)
{
  starBrightness_ = brightness;
}
void DynamicSky::setMilkyWayBrightness(GLfloat brightness)
{
  milkyWayBrightness_ = brightness;
}

void DynamicSky::setSunElevation(
    GLdouble time,
    GLdouble maxAngle,
    GLdouble minAngle,
    GLdouble orientation)
{
  maxElevationTime_ = time;
  maxElevationAngle_ = maxAngle*degToRad;
  minElevationAngle_ = minAngle*degToRad;
  maxElevationOrientation_ = orientation*degToRad;
}

void DynamicSky::set_updateInterval(GLdouble ms)
{
  updateInterval_ = ms;
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
ref_ptr<ShaderInput3f>& DynamicSky::skyColor()
{
  return skyAbsorbtion_;
}

void DynamicSky::setEarth()
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

void DynamicSky::setMars()
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

void DynamicSky::setUranus()
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

void DynamicSky::setVenus()
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

void DynamicSky::setAlien()
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
  dt_ = 0.0;

#if 0
  GLint minutes = (GLint)(dayTime_*24.0*60.0);
  GLint hours = minutes/60;
  minutes = minutes%60;
  DEBUG_LOG("TIME: " << hours << ":" << minutes);
#endif

  // compute current sun position
  {
    GLdouble timeDiff = maxElevationTime_-dayTime_;
    GLdouble timeDiffPi = timeDiff*M_PI;
    GLdouble sunAzimuth = maxElevationOrientation_ + timeDiffPi*2.0;
    GLdouble sunAltitude = minElevationAngle_ +
        (maxElevationAngle_ - minElevationAngle_)*cos(timeDiffPi);
    // sun rotation as seen from horizont space
    Mat4f sunRotation = xyzRotationMatrix(sunAltitude, sunAzimuth, 0.0);

    // rotate star and milky way map.
    // this might be not very realistic, but it is ok for now...
    GLdouble planetAltitude = 0.81015*(cos(timeDiffPi*2.0)*0.5);
    planetRotation_->setVertex16f(0, xyzRotationMatrix(planetAltitude, sunAzimuth, 0.0));

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
    // change diffuse color brightness depending on elevation.
    // +0.2 is because the sun does disappear below 0°
    GLdouble brigthness = (sunAltitude+0.2)/maxElevationAngle_;
    if(brigthness < 0.0) { brigthness = 0.0; }
    sun_->set_diffuse(color * brigthness);

    if(brightStarMap_.get()) {
      starVisibility_->setVertex1f(0, (1.0f-brigthness)*starBrightness_);
    }
    if(milkyWayMap_.get()) {
      milkywayVisibility_->setVertex1f(0, (1.0f-brigthness)*milkyWayBrightness_);
    }
  }

  updateSky();
}

void DynamicSky::updateSky()
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, cubeMap_->width(), cubeMap_->height());
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  if(brightStarMap_.get()!=NULL) {
    glActiveTexture(GL_TEXTURE0+starMapChannel_);
    brightStarMap_->bind();
  }
  if(milkyWayMap_.get()!=NULL) {
    glActiveTexture(GL_TEXTURE0+milkyWayMapChannel_);
    milkyWayMap_->bind();
  }

  updateState_->enable(&rs_);
  updateState_->disable(&rs_);

#ifdef USE_MIPMAPPING
  cubeMap_->setupMipmaps(GL_DONT_CARE);
#endif
}

///////////
///////////
///////////

static GLdouble randomNorm() {
  return (GLdouble)rand()/(GLdouble)RAND_MAX;
}

StarSkyMap::StarSkyMap(GLuint cubeMapSize)
: TextureCube(),
  numStars_(0),
  vertexData_(NULL),
  vertexSize_(0)
{
  bind();
  set_format(GL_RGBA);
  set_internalFormat(GL_RGBA);
  set_filter(GL_LINEAR, GL_LINEAR);
  set_wrapping(GL_CLAMP_TO_EDGE);
  set_size(cubeMapSize,cubeMapSize);
  texImage();

  // create render target
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, id(), 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  updateState_ = ref_ptr<State>::manage(new State);
  mvpMatrices_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("mvpMatrices",6));
  updateState_->joinShaderInput(ref_ptr<ShaderInput>::cast(mvpMatrices_));
  // use alpha blending when updating the stars.
  // note: alpha blending with float textures produces artifacts here
  updateState_->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  // disable culling. star sprites should never be culled
  updateState_->joinStates(ref_ptr<State>::manage(new CullDisableState));
  updateShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  updateState_->joinStates(ref_ptr<State>::cast(updateShader_));

  // create mvp matrix for each cube face
  mvpMatrices_->setVertexData(1,NULL);
  const Mat4f *views = getCubeLookAtMatrices();
  Mat4f proj = projectionMatrix(90.0, 1.0f, 0.1, 2.0);
  for(register GLuint i=0; i<6; ++i) {
    mvpMatrices_->setVertex16f(i, views[i] * proj);
  }

  // create the update shader
  ShaderConfig shaderCfg;
  updateState_->configureShader(&shaderCfg);
  shaderCfg.define("HAS_GEOMETRY_SHADER", "TRUE");
  shaderCfg.setVersion("400");
  updateShader_->createShader(shaderCfg, "sky.stars");
}

StarSkyMap::~StarSkyMap()
{
  if(vertexData_!=NULL) {
    delete []vertexData_;
  }
  glDeleteFramebuffers(1, &fbo_);
}

GLboolean StarSkyMap::readStarFile(const string &path, GLuint numStars)
{
  return readStarFile_short(path, numStars);
}

GLboolean StarSkyMap::readStarFile_short(const string &path, GLuint numStars)
{
  struct ShortStar {
    short x, y, z;
    short r, g, b;
  };
  const GLuint faceVertices = 1;

  ifstream starFile(path.c_str(), ios::in|ios::binary);
  if (!starFile.is_open()) {
    ERROR_LOG("failed to open file at '" << path << "'");
    return GL_FALSE;
  }

  starFile.seekg (0, ios::end);
  GLint actualSize = (GLint) starFile.tellg();
  starFile.seekg (0, ios::beg);

  GLint expectedSize = numStars * sizeof(ShortStar);
  if (actualSize != expectedSize) {
    ERROR_LOG("file size missmatch " << actualSize << " != " << expectedSize << ".");
    return GL_FALSE;
  }

  // randomize stars a bit
  srand(time(0));

  numStars_ = numStars;
  ShortStar stars[numStars_];
  starFile.read((char*)stars, actualSize);

  vertexSize_ = 0;
  vertexSize_ += sizeof(GLfloat)*3; // position
  vertexSize_ += sizeof(GLfloat)*4; // color
  GLuint valueCount = numStars_*faceVertices*vertexSize_/sizeof(GLfloat);

  if(vertexData_!=NULL) {
    delete []vertexData_;
  }
  vertexData_ = new GLfloat[valueCount];
  GLfloat *vertexPtr = vertexData_;

  // convert to format used in ogle
  for(GLuint i=0; i<numStars; ++i)
  {
    ShortStar &star = stars[i];
    // TODO: not sure about x/y/z. z is unused now...
    // is the star size encoded somehow ?
    // at least color looks right.

    Vec3f pos(
        cos(star.x) * sin(star.y),
        sin(star.x),
        cos(star.x) * -cos(star.y));
    // scale position by star size.
    normalize(pos);
    pos *= 0.0025 + randomNorm()*0.002;
    *((Vec3f*)vertexPtr) = pos;
    vertexPtr += 3;

    Vec4f color(
        (GLfloat)star.r / (GLfloat)SHRT_MAX,
        (GLfloat)star.g / (GLfloat)SHRT_MAX,
        (GLfloat)star.b / (GLfloat)SHRT_MAX,
        1.15); // 1.15 influences alpha gradient
    *((Vec4f*)vertexPtr) = color;
    vertexPtr += 4;
  }

  starFile.close();

  return GL_TRUE;
}

void StarSkyMap::uploadVertexData()
{
  const GLuint faceVertices = 1;
  glBufferData(GL_ARRAY_BUFFER,
      vertexSize_*numStars_*faceVertices, vertexData_, GL_STATIC_DRAW);
}
void StarSkyMap::enableVertexData()
{
  // TODO: use VertexAttribute type, this would allow
  //  having a star mesh type aswell
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT,
      GL_FALSE, vertexSize_, BUFFER_OFFSET(sizeof(GLfloat)*0));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT,
      GL_FALSE, vertexSize_, BUFFER_OFFSET(sizeof(GLfloat)*3));
}

void StarSkyMap::disableVertexData()
{
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

void StarSkyMap::update()
{
  // create temporary vbo.
  GLuint vbo_;
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  uploadVertexData();
  enableVertexData();

  // bind render target and clear to zero
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0,0,width_,height_);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glClearColor(0.0f,0.0f,0.0f,0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  updateState_->enable(&rs_);

  glDrawArrays(GL_POINTS, 0, numStars_);

  updateState_->disable(&rs_);

  disableVertexData();
  glDeleteBuffers(1, &vbo_);
}
