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

// TODO: handle moons

#define SUN_MAX_ELEVATION  45.0
#define SUN_MIN_ELEVATION -65.0

DynamicSky::DynamicSky(
    ref_ptr<MeshState> orthoQuad,
    GLfloat far, GLuint cubeMapSize, GLboolean useFloatBuffer)
: SkyBox(far),
  Animation(),
  dayTime_(0.5),
  timeScale_(0.0001),
  updateInterval_(40.0),
  dt_(0.0)
{
  starBrightness_ = 0.0f;
  milkyWayBrightness_ = 0.0f;

  ref_ptr<TextureCube> cubeMap = ref_ptr<TextureCube>::manage(new TextureCube(1));
  cubeMap->bind();
  cubeMap->set_format(GL_RGB);
  if(useFloatBuffer) {
    cubeMap->set_internalFormat(GL_RGB16F);
  } else {
    cubeMap->set_internalFormat(GL_RGB);
  }
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

  setEarth(53.075813, 8.807357);
}
DynamicSky::~DynamicSky()
{
  glDeleteFramebuffers(1, &fbo_);
}

void DynamicSky::setMilkyWayMap(const ref_ptr<TextureCube> &milkyWayMap, GLfloat brightness)
{
  milkyWayBrightness_ = brightness;
  if(milkyWayMap.get()!=NULL) {
    updateShader_->shader()->setTexture(&milkyWayMapChannel_, "milkyWayMap");
  } else {
    milkywayVisibility_->setVertex1f(0, 0.0f);
    updateShader_->shader()->setTexture(NULL, "milkyWayMap");
  }
  milkyWayMap_ = milkyWayMap;
}

void DynamicSky::setBrightStarMap(const ref_ptr<TextureCube> &starMap, GLfloat brightness)
{
  starBrightness_ = brightness;
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

  sunDistance_ = p.sunDistance;
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

  GLdouble t = dayTime_ + timeOffset_;
  if(t>1.0) { t-=1.0; }
  else if(t<0.0) { t+=1.0; }

  // compute current sun position
  {
    Vec3f &lightDir = lightDir_->getVertex3f(0);
    Vec3f sunToPlanet(sunDistance_,0.0,0.0);
    // rotate planet using the tilt
    Quaternion planetRotation;
    planetRotation.setAxisAngle(planetAxis_, t*2.0*M_PI);
    Vec3f cameraY = planetRotation.rotate(yAxis_);
    Vec3f cameraZ = planetRotation.rotate(zAxis_);
    Vec3f locationToSun = 0.5*planetDiameter_*cameraY - sunToPlanet;
    Mat4f transformToSun = lookAtCameraInverse(
        getLookAtMatrix(locationToSun, cameraZ, cameraY) );
    lightDir = transformVec3(transformToSun, locationToSun);
    normalize(lightDir);
    sun_->set_direction(lightDir);

    GLdouble nightFade = lightDir.y;
    if(nightFade < 0.0) { nightFade = 0.0; }

    // linear interpolate between day and night colors
    Vec3f &dayColor = skyAbsorbtion_->getVertex3f(0);
    Vec3f color = Vec3f(1.0)-dayColor; // night color
    color = color*(1.0-nightFade) + dayColor*nightFade;
    sun_->set_diffuse(color * nightFade);

    GLdouble starFade = (1.0-nightFade);
    starFade *= starFade;
    if(brightStarMap_.get()) {
      starVisibility_->setVertex1f(0, starFade*starBrightness_);
    }
    if(milkyWayMap_.get()) {
      milkywayVisibility_->setVertex1f(0, starFade*milkyWayBrightness_);
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

StarSky::StarSky()
: numStars_(0),
  vertexData_(NULL),
  vertexSize_(0),
  starAlpha_(1.5f),
  starSize_(0.002),
  starSizeVariance_(0.5)
{
  vertexSize_ = 0;
  vertexSize_ += sizeof(GLfloat)*3; // position
  vertexSize_ += sizeof(GLfloat)*4; // color

  GLuint vertexOffset = 0;
  posAttribute_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("pos"));
  posAttribute_->set_offset(vertexOffset);
  posAttribute_->set_stride(vertexSize_);
  posAttribute_->set_numVertices(0);
  vertexOffset += sizeof(GLfloat)*3;

  colorAttribute_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("color"));
  colorAttribute_->set_offset(vertexOffset);
  colorAttribute_->set_stride(vertexSize_);
  colorAttribute_->set_numVertices(0);
  vertexOffset += sizeof(GLfloat)*4;
}

StarSky::~StarSky()
{
  if(vertexData_!=NULL) {
    delete []vertexData_;
  }
}

void StarSky::set_starAlphaScale(GLfloat alphaScale)
{
  starAlpha_ = alphaScale;
}

void StarSky::set_starSize(GLfloat size, GLfloat variance)
{
  starSize_ = size;
  starSizeVariance_ = variance;
}

GLboolean StarSky::readStarFile(const string &path, GLuint numStars)
{
  return readStarFile_short(path, numStars);
}

GLboolean StarSky::readStarFile_short(const string &path, GLuint numStars)
{
  struct ShortStar {
    short x, y, z;
    short r, g, b;
  };

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
  posAttribute_->set_numVertices(numStars_);
  posAttribute_->set_size(numStars_*sizeof(GLfloat)*3);
  colorAttribute_->set_numVertices(numStars_);
  colorAttribute_->set_size(numStars_*sizeof(GLfloat)*4);
  ShortStar stars[numStars_];
  starFile.read((char*)stars, actualSize);

  GLuint valueCount = numStars_*vertexSize_/sizeof(GLfloat);

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

    GLfloat x = 2.0*M_PI*(GLfloat)star.x / (GLfloat)SHRT_MAX;
    GLfloat y = 2.0*M_PI*(GLfloat)star.y / (GLfloat)SHRT_MAX;

    Vec3f pos(cos(x)*sin(y), sin(x), -cos(x)*cos(y)); normalize(pos);
    // scale position by star size.
    pos *= starSize_ + randomNorm()*starSize_*starSizeVariance_;
    *((Vec3f*)vertexPtr) = pos;
    vertexPtr += 3;

    Vec4f color(
        (GLfloat)star.r / (GLfloat)SHRT_MAX,
        (GLfloat)star.g / (GLfloat)SHRT_MAX,
        (GLfloat)star.b / (GLfloat)SHRT_MAX,
        starAlpha_); // influences alpha gradient
    *((Vec4f*)vertexPtr) = color;
    vertexPtr += 4;
  }

  starFile.close();

  starDataUpdated();

  return GL_TRUE;
}

///////////

StarSkyMap::StarSkyMap(GLuint cubeMapSize)
: TextureCube(),
  StarSky()
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
  updateShader_->createShader(shaderCfg, "sky.starMap");
}

StarSkyMap::~StarSkyMap()
{
  glDeleteFramebuffers(1, &fbo_);
}

void StarSkyMap::update()
{
  // create temporary vbo.
  GLuint vbo_;
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, vertexSize_*numStars_, vertexData_, GL_STATIC_DRAW);
  posAttribute_->enable(0);
  colorAttribute_->enable(1);

  // bind render target and clear to zero
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0,0,width_,height_);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glClearColor(0.0f,0.0f,0.0f,0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  updateState_->enable(&rs_);

  glDrawArrays(GL_POINTS, 0, numStars_);

  updateState_->disable(&rs_);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDeleteBuffers(1, &vbo_);
}

///////////

StarSkyMesh::StarSkyMesh()
: MeshState(GL_POINTS),
  StarSky()
{
  vboState_ = ref_ptr<VBOState>::manage(
      new VBOState(0, VertexBufferObject::USAGE_STATIC));
  joinStates(ref_ptr<State>::cast(vboState_));
  // use alpha blending when updating the stars.
  //joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  // disable culling. star sprites should never be culled
  joinStates(ref_ptr<State>::manage(new CullDisableState));

  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE); // XXX
  depthState->set_useDepthWrite(GL_FALSE);
  //joinStates(ref_ptr<State>::cast(depthState));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->set_shading(Material::NO_SHADING);
  material->setConstantUniforms(GL_TRUE);
  joinStates(ref_ptr<State>::cast(material));

  GLuint vboID = vboState_->vbo()->id();
  setBuffer(vboID);
  posAttribute_->set_buffer(vboID);
  setInput(ref_ptr<ShaderInput>::cast(posAttribute_));
  colorAttribute_->set_buffer(vboID);
  setInput(ref_ptr<ShaderInput>::cast(colorAttribute_));

  set_starSize(0.001f, 0.0f);
}

void StarSkyMesh::starDataUpdated()
{
  vboState_->vbo()->bind(GL_ARRAY_BUFFER);
  glBufferData(GL_ARRAY_BUFFER,
      vertexSize_*numStars_, vertexData_, GL_STATIC_DRAW);
  numVertices_ = numStars_;
}

void StarSkyMesh::configureShader(ShaderConfig *cfg)
{
  cfg->setShaderInput(ref_ptr<ShaderInput>::cast(posAttribute_));
  cfg->setShaderInput(ref_ptr<ShaderInput>::cast(colorAttribute_));
  MeshState::configureShader(cfg);
  cfg->define("HAS_GEOMETRY_SHADER", "TRUE");
  cfg->setVersion("400");
}
