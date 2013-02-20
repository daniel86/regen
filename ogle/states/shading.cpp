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

string shadowFilterMode(ShadowMap::FilterMode f) {
  switch(f) {
  case ShadowMap::FILTERING_NONE: return "Single";
  case ShadowMap::FILTERING_PCF_4TAB: return "4Tab";
  case ShadowMap::FILTERING_PCF_8TAB_RAND: return "8Tab";
  case ShadowMap::FILTERING_PCF_GAUSSIAN: return "Gaussian";
  case ShadowMap::FILTERING_VSM: return "VSM";
  }
  return "Single";
}

/////////////
/////////////

DeferredAmbientLight::DeferredAmbientLight()
: State()
{
  ambientLight_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f("lightAmbient"));
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
: DeferredLight()
{
  mesh_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());
  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
}
void DeferredDirLight::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  _cfg.define("NUM_SHADOW_MAP_SLICES", FORMAT_STRING(DirectionalShadowMap::numSplits()));
  shader_->createShader(_cfg.cfg(), "shading.deferred.directional");

  Shader *s = shader_->shader().get();
  // find uniform locations
  dirLoc_ = s->uniformLocation("lightDirection");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  specularLoc_ = s->uniformLocation("lightSpecular");
  shadowMapSizeLoc_ = s->uniformLocation("shadowMapSize");
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
      sm->shadowMapSize()->enableUniform(shadowMapSizeLoc_);
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
  shadowMapSizeLoc_ = s->uniformLocation("shadowMapSize");
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
      sm->shadowMapSize()->enableUniform(shadowMapSizeLoc_);
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
  shadowMapSizeLoc_ = s->uniformLocation("shadowMapSize");
  shadowMapLoc_ = s->uniformLocation("shadowTexture");
  shadowMatLoc_ = s->uniformLocation("shadowMatrix");
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
      sm->shadowMapSize()->enableUniform(shadowMapSizeLoc_);
      sm->shadowMatUniform()->enableUniform(shadowMatLoc_);
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
    sm->set_computeMoments();
    // XXX config
    it->sm->set_useMomentBlurFilter();
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
      it->sm->set_computeMoments();
      // XXX config
      it->sm->set_useMomentBlurFilter();
    }
  }
}

void DeferredLight::activateShadowMap(ShadowMap *sm, GLuint channel)
{
  switch(shadowFiltering_) {
  case ShadowMap::FILTERING_VSM:
    sm->shadowMoments()->texture()->activateBind(channel);
    break;
  default:
    sm->shadowDepth()->texture()->activateBind(channel);
    break;
  }
  glUniform1i(shadowMapLoc_, channel);
}

/////////////
/////////////

DeferredShading::DeferredShading()
: State()
{
  // accumulate using add blending
  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));

  ambientState_ = ref_ptr<DeferredAmbientLight>::manage(new DeferredAmbientLight);
  hasAmbient_ = GL_FALSE;

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

  deferredShadingSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(ref_ptr<State>::cast(deferredShadingSequence_));
}

void DeferredShading::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  ambientState_->createShader(_cfg.cfg());
  dirState_->createShader(_cfg.cfg());
  pointState_->createShader(_cfg.cfg());
  spotState_->createShader(_cfg.cfg());
  dirShadowState_->createShader(_cfg.cfg());
  pointShadowState_->createShader(_cfg.cfg());
  spotShadowState_->createShader(_cfg.cfg());
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

void DeferredShading::addLight(
    const ref_ptr<DirectionalLight> &l,
    const ref_ptr<DirectionalShadowMap> &sm)
{
  if(dirShadowState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(dirShadowState_));
  }
  dirShadowState_->addLight(
      ref_ptr<Light>::cast(l),
      ref_ptr<ShadowMap>::cast(sm));
}
void DeferredShading::addLight(const ref_ptr<DirectionalLight> &l)
{
  if(dirState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(dirState_));
  }
  dirState_->addLight(
      ref_ptr<Light>::cast(l),
      ref_ptr<ShadowMap>());
}

void DeferredShading::addLight(
    const ref_ptr<PointLight> &l,
    const ref_ptr<PointShadowMap> &sm)
{
  if(pointShadowState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(pointShadowState_));
  }
  pointShadowState_->addLight(
      ref_ptr<Light>::cast(l),
      ref_ptr<ShadowMap>::cast(sm));
}
void DeferredShading::addLight(
    const ref_ptr<SpotLight> &l,
    const ref_ptr<SpotShadowMap> &sm)
{
  if(spotShadowState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(spotShadowState_));
  }
  spotShadowState_->addLight(
      ref_ptr<Light>::cast(l),
      ref_ptr<ShadowMap>::cast(sm));
}
void DeferredShading::addLight(const ref_ptr<PointLight> &l)
{
  if(pointState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(pointState_));
  }
  pointState_->addLight(
      ref_ptr<Light>::cast(l),
      ref_ptr<ShadowMap>());
}
void DeferredShading::addLight(const ref_ptr<SpotLight> &l)
{
  if(spotState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(spotState_));
  }
  spotState_->addLight(
      ref_ptr<Light>::cast(l),
      ref_ptr<ShadowMap>());
}

void DeferredShading::removeLight(DirectionalLight *l)
{
  dirState_->removeLight(l);
  if(dirState_->lights_.empty()) {
    deferredShadingSequence_->disjoinStates(ref_ptr<State>::cast(dirState_));
  }
}
void DeferredShading::removeLight(PointLight *l)
{
  pointState_->removeLight(l);
  if(pointState_->lights_.empty()) {
    deferredShadingSequence_->disjoinStates(ref_ptr<State>::cast(pointState_));
  }
}
void DeferredShading::removeLight(SpotLight *l)
{
  spotState_->removeLight(l);
  if(spotState_->lights_.empty()) {
    deferredShadingSequence_->disjoinStates(ref_ptr<State>::cast(spotState_));
  }
}

void DeferredShading::setDirFiltering(ShadowMap::FilterMode mode)
{
  dirShadowState_->setShadowFiltering(mode);
}
void DeferredShading::setPointFiltering(ShadowMap::FilterMode mode)
{
  pointShadowState_->setShadowFiltering(mode);
}
void DeferredShading::setSpotFiltering(ShadowMap::FilterMode mode)
{
  spotShadowState_->setShadowFiltering(mode);
}

void DeferredShading::setAmbientLight(const Vec3f &v)
{
  if(!hasAmbient_) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(ambientState_));
    hasAmbient_ = GL_TRUE;
  }
  ambientState_->ambientLight()->setVertex3f(0,v);
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

CombineShading::CombineShading()
: State()
{
  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));

  combineShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(combineShader_));
}

void CombineShading::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  combineShader_->createShader(_cfg.cfg(), "shading.combine");
}

void CombineShading::set_gBuffer(const ref_ptr<Texture> &t)
{
  if(gColorTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gColorTexture_));
  }
  gColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  gColorTexture_->set_name("gColorTexture");
  joinStates(ref_ptr<State>::cast(gColorTexture_));
}
void CombineShading::set_tBuffer(const ref_ptr<Texture> &t)
{
  shaderDefine("USE_TRANSPARENCY","TRUE");
  if(tColorTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(tColorTexture_));
  }
  tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  tColorTexture_->set_name("tColorTexture");
  joinStates(ref_ptr<State>::cast(tColorTexture_));
}
void CombineShading::set_aoBuffer(const ref_ptr<Texture> &t)
{
  shaderDefine("USE_AMBIENT_OCCLUSION","TRUE");
  if(aoTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(aoTexture_));
  }
  aoTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  aoTexture_->set_name("aoTexture");
  joinStates(ref_ptr<State>::cast(aoTexture_));
}
void CombineShading::set_taoBuffer(const ref_ptr<Texture> &t)
{
  shaderDefine("USE_TRANSPARENCY_AMBIENT_OCCLUSION","TRUE");
  if(taoTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(taoTexture_));
  }
  taoTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  taoTexture_->set_name("taoTexture");
  joinStates(ref_ptr<State>::cast(taoTexture_));
}

