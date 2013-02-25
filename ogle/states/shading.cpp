/*
 * shading.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include "shading.h"

#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/attribute-less-mesh.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/cull-state.h>
#include <ogle/textures/texture-loader.h>

string shadowFilterMode(ShadowMap::FilterMode f) {
  switch(f) {
  case ShadowMap::FILTERING_NONE: return "Single";
  case ShadowMap::FILTERING_PCF_GAUSSIAN: return "Gaussian";
  case ShadowMap::FILTERING_VSM: return "VSM";
  }
  return "Single";
}

/////////////
/////////////

AmbientOcclusion::AmbientOcclusion(GLfloat sizeScale)
: State(), sizeScale_(sizeScale)
{
  blurSigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  blurSigma_->setUniformData(2.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(blurSigma_));

  blurNumPixels_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("numBlurPixels"));
  blurNumPixels_->setUniformData(4.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(blurNumPixels_));

  aoSamplingRadius_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoSamplingRadius"));
  aoSamplingRadius_->setUniformData(30.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoSamplingRadius_));

  aoBias_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoBias"));
  aoBias_->setUniformData(0.05);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoBias_));

  aoAttenuation_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("aoAttenuation"));
  aoAttenuation_->setUniformData( Vec2f(0.5,1.0) );
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoAttenuation_));

  ref_ptr<Texture> noise = TextureLoader::load("res/textures/random_normals.png");
  joinStates(ref_ptr<State>::manage(new TextureState(noise, "aoNoiseTexture")));
}
void AmbientOcclusion::createFilter(const ref_ptr<Texture> &input)
{
  if(filter_.get()) {
    disjoinStates(ref_ptr<State>::cast(filter_));
  }
  filter_ = ref_ptr<FilterSequence>::manage(new FilterSequence(input, GL_FALSE));
  filter_->setClearColor(Vec4f(0.0));
  filter_->set_format(GL_RGBA);
  filter_->set_internalFormat(GL_INTENSITY);
  filter_->set_pixelType(GL_BYTE);
  filter_->addFilter(ref_ptr<Filter>::manage(new Filter("shading.ssao", sizeScale_)));
  filter_->addFilter(ref_ptr<Filter>::manage(new Filter("blur.horizontal")));
  filter_->addFilter(ref_ptr<Filter>::manage(new Filter("blur.vertical")));
  joinStates(ref_ptr<State>::cast(filter_));
}
void AmbientOcclusion::createShader(ShaderConfig &cfg)
{
  if(filter_.get()) {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(this);
    filter_->createShader(_cfg.cfg());
  }
}
void AmbientOcclusion::resize()
{
  filter_->resize();
}

const ref_ptr<Texture>& AmbientOcclusion::aoTexture() const
{
  return filter_->output();
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::aoSamplingRadius() const
{
  return aoSamplingRadius_;
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::aoBias() const
{
  return aoBias_;
}
const ref_ptr<ShaderInput2f>& AmbientOcclusion::aoAttenuation() const
{
  return aoAttenuation_;
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::blurSigma() const
{
  return blurSigma_;
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::blurNumPixels() const
{
  return blurNumPixels_;
}

/////////////
/////////////

DeferredAmbientLight::DeferredAmbientLight() : State()
{
  ambientLight_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightAmbient"));
  ambientLight_->setUniformData(Vec3f(0.1f));
  joinShaderInput(ref_ptr<ShaderInput>::cast(ambientLight_));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}
void DeferredAmbientLight::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  shader_->createShader(_cfg.cfg(), "shading.deferred.ambient");
}
const ref_ptr<ShaderInput3f>& DeferredAmbientLight::ambientLight() const
{
  return ambientLight_;
}

/////////////
/////////////

DeferredDirLight::DeferredDirLight()
: DeferredLight(), numShadowLayer_(3)
{
  mesh_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());
  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
}
GLuint DeferredDirLight::numShadowLayer() const
{
  return numShadowLayer_;
}
void DeferredDirLight::set_numShadowLayer(GLuint numLayer)
{
  numShadowLayer_ = numLayer;
  // change number of layers for added lights
  for(list<DLight>::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    if(it->sm.get()) {
      DirectionalShadowMap *sm = (DirectionalShadowMap*) it->sm.get();
      sm->set_numShadowLayer(numLayer);
    }
  }
}
void DeferredDirLight::addLight(const ref_ptr<Light> &l, const ref_ptr<ShadowMap> &sm)
{
  DeferredLight::addLight(l, sm);
  if(sm.get()) {
    DirectionalShadowMap *sm_ = (DirectionalShadowMap*) sm.get();
    sm_->set_numShadowLayer(numShadowLayer_);
  }
}
void DeferredDirLight::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  _cfg.define("NUM_SHADOW_LAYER", FORMAT_STRING(numShadowLayer_));
  shader_->createShader(_cfg.cfg(), "shading.deferred.directional");

  Shader *s = shader_->shader().get();
  // find uniform locations
  dirLoc_ = s->uniformLocation("lightDirection");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  specularLoc_ = s->uniformLocation("lightSpecular");
  shadowMapLoc_ = s->uniformLocation("shadowTexture");
  shadowMatricesLoc_ = s->uniformLocation("shadowMatrices");
  shadowFarLoc_ = s->uniformLocation("shadowFar");
}
void DeferredDirLight::enable(RenderState *rs) {
  State::enable(rs);
  GLuint smChannel = rs->nextTexChannel();

  for(list<DLight>::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    DirectionalLight *l = (DirectionalLight*) it->l.get();
    l->direction()->enableUniform(dirLoc_);
    l->diffuse()->enableUniform(diffuseLoc_);
    l->specular()->enableUniform(specularLoc_);

    if(it->sm.get()) {
      DirectionalShadowMap *sm = (DirectionalShadowMap*) it->sm.get();
      activateShadowMap(sm, smChannel);
      sm->shadowFarUniform()->enableUniform(shadowFarLoc_);
      sm->shadowMatUniform()->enableUniform(shadowMatricesLoc_);
    }

    mesh_->draw(1);
  }

  rs->releaseTexChannel();
}

/////////////
/////////////

DeferredPointLight::DeferredPointLight()
: DeferredLight()
{
  mesh_ = ref_ptr<MeshState>::cast( Box::getUnitCube() );
  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
  joinStates(ref_ptr<State>::manage(new CullFrontFaceState));
}
void DeferredPointLight::createShader(ShaderConfig &cfg) {
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  shader_->createShader(_cfg.cfg(), "shading.deferred.point");

  Shader *s = shader_->shader().get();
  posLoc_ = s->uniformLocation("lightPosition");
  radiusLoc_ = s->uniformLocation("lightRadius");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  specularLoc_ = s->uniformLocation("lightSpecular");
  shadowMapLoc_ = s->uniformLocation("shadowTexture");
  shadowFarLoc_ = s->uniformLocation("shadowFar");
  shadowNearLoc_ = s->uniformLocation("shadowNear");
}
void DeferredPointLight::enable(RenderState *rs)
{
  State::enable(rs);
  GLuint smChannel = rs->nextTexChannel();

  for(list<DLight>::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    PointLight *l = (PointLight*) it->l.get();
    l->position()->enableUniform(posLoc_);
    l->radius()->enableUniform(radiusLoc_);
    l->diffuse()->enableUniform(diffuseLoc_);
    l->specular()->enableUniform(specularLoc_);

    if(it->sm.get()) {
      PointShadowMap *sm = (PointShadowMap*) it->sm.get();
      activateShadowMap(sm, smChannel);
      sm->far()->enableUniform(shadowFarLoc_);
      sm->near()->enableUniform(shadowNearLoc_);
    }

    mesh_->draw(1);
  }

  rs->releaseTexChannel();
}

/////////////
/////////////

DeferredSpotLight::DeferredSpotLight()
: DeferredLight()
{
  mesh_ = ref_ptr<MeshState>::cast( ClosedCone::getBaseCone() );
  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
  joinStates(ref_ptr<State>::manage(new CullFrontFaceState));
}
void DeferredSpotLight::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  shader_->createShader(_cfg.cfg(), "shading.deferred.spot");

  Shader *s = shader_->shader().get();
  dirLoc_ = s->uniformLocation("lightDirection");
  posLoc_ = s->uniformLocation("lightPosition");
  radiusLoc_ = s->uniformLocation("lightRadius");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  specularLoc_ = s->uniformLocation("lightSpecular");
  coneAnglesLoc_ = s->uniformLocation("lightConeAngles");
  coneMatLoc_ = s->uniformLocation("modelMatrix");
  shadowMapLoc_ = s->uniformLocation("shadowTexture");
  shadowMatLoc_ = s->uniformLocation("shadowMatrix");
  shadowFarLoc_ = s->uniformLocation("shadowFar");
  shadowNearLoc_ = s->uniformLocation("shadowNear");
}
void DeferredSpotLight::enable(RenderState *rs)
{
  State::enable(rs);
  GLuint smChannel = rs->nextTexChannel();

  for(list<DLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    SpotLight *l = (SpotLight*) it->l.get();
    l->spotDirection()->enableUniform(dirLoc_);
    l->coneAngle()->enableUniform(coneAnglesLoc_);
    l->position()->enableUniform(posLoc_);
    l->radius()->enableUniform(radiusLoc_);
    l->diffuse()->enableUniform(diffuseLoc_);
    l->specular()->enableUniform(specularLoc_);
    l->coneMatrix()->enableUniform(coneMatLoc_);

    if(it->sm.get()) {
      SpotShadowMap *sm = (SpotShadowMap*) it->sm.get();
      activateShadowMap(sm, smChannel);
      sm->shadowMatUniform()->enableUniform(shadowMatLoc_);
      sm->far()->enableUniform(shadowFarLoc_);
      sm->near()->enableUniform(shadowNearLoc_);
    }

    mesh_->draw(1);
  }

  rs->releaseTexChannel();
}

/////////////
/////////////

DeferredLight::DeferredLight()
: State()
{
  shadowFiltering_ = ShadowMap::FILTERING_NONE;
  setShadowFiltering(shadowFiltering_);
}

GLboolean DeferredLight::useShadowMoments()
{
  switch(shadowFiltering_) {
  case ShadowMap::FILTERING_VSM:
    return GL_TRUE;
  default:
    return GL_FALSE;
  }
}
GLboolean DeferredLight::useShadowSampler()
{
  switch(shadowFiltering_) {
  case ShadowMap::FILTERING_VSM:
    return GL_FALSE;
  default:
    return GL_TRUE;
  }
}

void DeferredLight::addLight(const ref_ptr<Light> &l, const ref_ptr<ShadowMap> &sm)
{
  lights_.push_back( DLight(l,sm) );

  list<DLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;

  if(sm.get() && useShadowMoments()) {
    sm->setComputeMoments();
  }
}
void DeferredLight::removeLight(Light *l)
{
  lights_.erase( lightIterators_[l] );
}

void DeferredLight::setShadowFiltering(ShadowMap::FilterMode mode)
{
  GLboolean usedMoments = useShadowMoments();

  shadowFiltering_ = mode;
  shaderDefine("USE_SHADOW_SAMPLER", useShadowSampler() ? "TRUE" : "FALSE");
  shaderDefine("SHADOW_MAP_FILTER", shadowFilterMode(mode));

  if(usedMoments != useShadowMoments())
  {
    for(list<DLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
    {
      if(!it->sm.get()) continue;
      it->sm->setComputeMoments();
    }
  }
}

void DeferredLight::activateShadowMap(ShadowMap *sm, GLuint channel)
{
  switch(shadowFiltering_) {
  case ShadowMap::FILTERING_VSM:
    sm->shadowMoments()->activateBind(channel);
    break;
  default:
    sm->shadowDepth()->activateBind(channel);
    break;
  }
  glUniform1i(shadowMapLoc_, channel);
}

/////////////
/////////////

// TODO: split into multiple files...

DeferredShading::DeferredShading()
: State(), hasAmbient_(GL_FALSE)
{
  // accumulate light using add blending
  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));

  ambientState_ = ref_ptr<DeferredAmbientLight>::manage(new DeferredAmbientLight);

  dirState_ = ref_ptr<DeferredDirLight>::manage(new DeferredDirLight);
  dirShadowState_ = ref_ptr<DeferredDirLight>::manage(new DeferredDirLight);
  dirShadowState_->shaderDefine("USE_SHADOW_MAP", "TRUE");
  dirShadowState_->shaderDefine("SHADOW_MAP_FILTER", "Single");

  pointState_ = ref_ptr<DeferredPointLight>::manage(new DeferredPointLight);
  pointShadowState_ = ref_ptr<DeferredPointLight>::manage(new DeferredPointLight);
  pointShadowState_->shaderDefine("USE_SHADOW_MAP", "TRUE");
  pointShadowState_->shaderDefine("SHADOW_MAP_FILTER", "Single");

  spotState_ = ref_ptr<DeferredSpotLight>::manage(new DeferredSpotLight());
  spotShadowState_ = ref_ptr<DeferredSpotLight>::manage(new DeferredSpotLight());
  spotShadowState_->shaderDefine("USE_SHADOW_MAP", "TRUE");
  spotShadowState_->shaderDefine("SHADOW_MAP_FILTER", "Single");

  lightSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(ref_ptr<State>::cast(lightSequence_));
}

void DeferredShading::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  // TODO: do not create all shader
  dirState_->createShader(_cfg.cfg());
  pointState_->createShader(_cfg.cfg());
  spotState_->createShader(_cfg.cfg());
  dirShadowState_->createShader(_cfg.cfg());
  pointShadowState_->createShader(_cfg.cfg());
  spotShadowState_->createShader(_cfg.cfg());
  if(hasAmbient_) {
    ambientState_->createShader(_cfg.cfg());
  }
}

void DeferredShading::set_gBuffer(
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &norWorldTexture,
    const ref_ptr<Texture> &diffuseTexture,
    const ref_ptr<Texture> &specularTexture)
{
  if(gDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(gDiffuseTexture_));
    disjoinStates(ref_ptr<State>::cast(gSpecularTexture_));
    disjoinStates(ref_ptr<State>::cast(gNorWorldTexture_));
  }

  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depthTexture));
  gDepthTexture_->set_name("gDepthTexture");
  joinStatesFront(ref_ptr<State>::cast(gDepthTexture_));

  gNorWorldTexture_ = ref_ptr<TextureState>::manage(new TextureState(norWorldTexture));
  gNorWorldTexture_->set_name("gNorWorldTexture");
  joinStatesFront(ref_ptr<State>::cast(gNorWorldTexture_));

  gDiffuseTexture_ = ref_ptr<TextureState>::manage(new TextureState(diffuseTexture));
  gDiffuseTexture_->set_name("gDiffuseTexture");
  joinStatesFront(ref_ptr<State>::cast(gDiffuseTexture_));

  gSpecularTexture_ = ref_ptr<TextureState>::manage(new TextureState(specularTexture));
  gSpecularTexture_->set_name("gSpecularTexture");
  joinStatesFront(ref_ptr<State>::cast(gSpecularTexture_));
}

void DeferredShading::setUseAmbientLight()
{
  if(!hasAmbient_) {
    lightSequence_->joinStates(ref_ptr<State>::cast(ambientState_));
    hasAmbient_ = GL_TRUE;
  }
}

void DeferredShading::addLight(const ref_ptr<DirectionalLight> &l, const ref_ptr<DirectionalShadowMap> &sm)
{
  if(dirShadowState_->lights_.empty()) {
    lightSequence_->joinStates(ref_ptr<State>::cast(dirShadowState_));
  }
  dirShadowState_->addLight(ref_ptr<Light>::cast(l), ref_ptr<ShadowMap>::cast(sm));
}
void DeferredShading::addLight(const ref_ptr<DirectionalLight> &l)
{
  if(dirState_->lights_.empty()) {
    lightSequence_->joinStates(ref_ptr<State>::cast(dirState_));
  }
  dirState_->addLight(ref_ptr<Light>::cast(l), ref_ptr<ShadowMap>());
}
void DeferredShading::removeLight(DirectionalLight *l)
{
  dirState_->removeLight(l);
  if(dirState_->lights_.empty()) {
    lightSequence_->disjoinStates(ref_ptr<State>::cast(dirState_));
  }
}

void DeferredShading::addLight(const ref_ptr<PointLight> &l, const ref_ptr<PointShadowMap> &sm)
{
  if(pointShadowState_->lights_.empty()) {
    lightSequence_->joinStates(ref_ptr<State>::cast(pointShadowState_));
  }
  pointShadowState_->addLight(ref_ptr<Light>::cast(l), ref_ptr<ShadowMap>::cast(sm));
}
void DeferredShading::addLight(const ref_ptr<PointLight> &l)
{
  if(pointState_->lights_.empty()) {
    lightSequence_->joinStates(ref_ptr<State>::cast(pointState_));
  }
  pointState_->addLight(ref_ptr<Light>::cast(l), ref_ptr<ShadowMap>());
}
void DeferredShading::removeLight(PointLight *l)
{
  pointState_->removeLight(l);
  if(pointState_->lights_.empty()) {
    lightSequence_->disjoinStates(ref_ptr<State>::cast(pointState_));
  }
}

void DeferredShading::addLight(const ref_ptr<SpotLight> &l, const ref_ptr<SpotShadowMap> &sm)
{
  if(spotShadowState_->lights_.empty()) {
    lightSequence_->joinStates(ref_ptr<State>::cast(spotShadowState_));
  }
  spotShadowState_->addLight(ref_ptr<Light>::cast(l), ref_ptr<ShadowMap>::cast(sm));
}
void DeferredShading::addLight(const ref_ptr<SpotLight> &l)
{
  if(spotState_->lights_.empty()) {
    lightSequence_->joinStates(ref_ptr<State>::cast(spotState_));
  }
  spotState_->addLight(ref_ptr<Light>::cast(l), ref_ptr<ShadowMap>());
}
void DeferredShading::removeLight(SpotLight *l)
{
  spotState_->removeLight(l);
  if(spotState_->lights_.empty()) {
    lightSequence_->disjoinStates(ref_ptr<State>::cast(spotState_));
  }
}

const ref_ptr<DeferredDirLight>& DeferredShading::dirState() const
{
  return dirState_;
}
const ref_ptr<DeferredDirLight>& DeferredShading::dirShadowState() const
{
  return dirShadowState_;
}
const ref_ptr<DeferredPointLight>& DeferredShading::pointState() const
{
  return pointState_;
}
const ref_ptr<DeferredPointLight>& DeferredShading::pointShadowState() const
{
  return pointShadowState_;
}
const ref_ptr<DeferredSpotLight>& DeferredShading::spotState() const
{
  return spotState_;
}
const ref_ptr<DeferredSpotLight>& DeferredShading::spotShadowState() const
{
  return spotShadowState_;
}
const ref_ptr<DeferredAmbientLight>& DeferredShading::ambientState() const
{
  return ambientState_;
}

//////////////////
//////////////////
//////////////////

DirectShading::DirectShading() : State()
{
  shaderDefine("NUM_LIGHTS", "0");
}

void DirectShading::addLight(const ref_ptr<Light> &l)
{
  GLuint numLights = lights_.size();
  // map for loop index to light id
  shaderDefine(
      FORMAT_STRING("LIGHT" << numLights << "_ID"),
      FORMAT_STRING(l->id()));
  // remember the number of lights used
  shaderDefine("NUM_LIGHTS", FORMAT_STRING(numLights+1));

  joinStatesFront(ref_ptr<State>::cast(l));
  lights_.push_back(l);
}
void DirectShading::removeLight(const ref_ptr<Light> &l)
{
  for(list< ref_ptr<Light> >::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    ref_ptr<Light> &x = *it;
    if(x.get()==l.get()) {
      lights_.erase(it);
      break;
    }
  }
  disjoinStates(ref_ptr<State>::cast(l));

  GLuint numLights = lights_.size(), lightIndex=0;
  // update shader defines
  shaderDefine("NUM_LIGHTS", FORMAT_STRING(numLights));
  for(list< ref_ptr<Light> >::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    ref_ptr<Light> &x = *it;
    shaderDefine(
        FORMAT_STRING("LIGHT" << lightIndex << "_ID"),
        FORMAT_STRING(x->id()));
    ++lightIndex;
  }
}

//////////////////
//////////////////
//////////////////

ShadingPostProcessing::ShadingPostProcessing()
: State()
{
  stateSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(ref_ptr<State>::cast(stateSequence_));

  updateAOState_ = ref_ptr<AmbientOcclusion>::manage(new AmbientOcclusion(0.5));

  ref_ptr<State> drawState = ref_ptr<State>::manage(new State);
  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  drawState->joinStates(ref_ptr<State>::cast(shader_));
  drawState->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
  stateSequence_->joinStates(drawState);
}
void ShadingPostProcessing::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  shader_->createShader(_cfg.cfg(), "shading.postProcessing");
  if(hasAO_) {
    updateAOState_->createShader(_cfg.cfg());
  }
}
void ShadingPostProcessing::resize()
{
  updateAOState_->resize();
}

const ref_ptr<AmbientOcclusion>& ShadingPostProcessing::ambientOcclusionState() const
{
  return updateAOState_;
}

void ShadingPostProcessing::setUseAmbientOcclusion()
{
  if(!hasAO_) {
    stateSequence_->joinStatesFront(ref_ptr<State>::cast(updateAOState_));
    hasAO_ = GL_TRUE;
    if(gNorWorldTexture_.get()) {
      updateAOState_->createFilter(gNorWorldTexture_->texture());
      set_aoBuffer(updateAOState_->aoTexture());
    }
  }
}

void ShadingPostProcessing::set_gBuffer(
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &norWorldTexture,
    const ref_ptr<Texture> &diffuseTexture)
{
  if(gDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(gDiffuseTexture_));
    disjoinStates(ref_ptr<State>::cast(gNorWorldTexture_));
  }

  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depthTexture));
  gDepthTexture_->set_name("gDepthTexture");
  joinStatesFront(ref_ptr<State>::cast(gDepthTexture_));

  gNorWorldTexture_ = ref_ptr<TextureState>::manage(new TextureState(norWorldTexture));
  gNorWorldTexture_->set_name("gNorWorldTexture");
  joinStatesFront(ref_ptr<State>::cast(gNorWorldTexture_));

  gDiffuseTexture_ = ref_ptr<TextureState>::manage(new TextureState(diffuseTexture));
  gDiffuseTexture_->set_name("gDiffuseTexture");
  joinStatesFront(ref_ptr<State>::cast(gDiffuseTexture_));

  if(hasAO_) {
    updateAOState_->createFilter(gNorWorldTexture_->texture());
    set_aoBuffer(updateAOState_->aoTexture());
  }
}

void ShadingPostProcessing::set_tBuffer(const ref_ptr<Texture> &t)
{
  shaderDefine("USE_TRANSPARENCY","TRUE");
  if(tColorTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(tColorTexture_));
  }
  tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  tColorTexture_->set_name("tColorTexture");
  joinStatesFront(ref_ptr<State>::cast(tColorTexture_));
}

void ShadingPostProcessing::set_aoBuffer(const ref_ptr<Texture> &t)
{
  shaderDefine("USE_AMBIENT_OCCLUSION","TRUE");
  if(aoTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(aoTexture_));
  }
  aoTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  aoTexture_->set_name("aoTexture");
  joinStatesFront(ref_ptr<State>::cast(aoTexture_));
}

