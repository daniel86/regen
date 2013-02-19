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
  case ShadowMap::SINGLE: return "Single";
  case ShadowMap::PCF_4TAB: return "4Tab";
  case ShadowMap::PCF_8TAB_RAND: return "8Tab";
  case ShadowMap::PCF_GAUSSIAN: return "Gaussian";
  //case VSM: return "VSM";
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
: State()
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
  _cfg.define("NUM_SHADOW_MAP_SLICES",
      FORMAT_STRING(DirectionalShadowMap::numSplits()));
  shader_->createShader(_cfg.cfg(), "shading.deferred.directional");

  Shader *s = shader_->shader().get();
  dirLoc_ = s->uniformLocation("lightDirection");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  specularLoc_ = s->uniformLocation("lightSpecular");

  shadowMapSizeLoc_ = s->uniformLocation("shadowMapSize");
  shadowMapLoc_ = s->uniformLocation("shadowMap");
  shadowMatricesLoc_ = s->uniformLocation("shadowMatrices");
  shadowFarLoc_ = s->uniformLocation("shadowFar");

}
void DeferredDirLight::addLight(
    const ref_ptr<DirectionalLight> &l,
    const ref_ptr<DirectionalShadowMap> &sm)
{
  lights_.push_back( DeferredLight(l,sm) );

  list<DeferredLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;
}
void DeferredDirLight::removeLight(Light *l)
{
  lights_.erase(lightIterators_[l]);
}
void DeferredDirLight::enable(RenderState *rs) {
  State::enable(rs);
  GLuint smChannel = rs->nextTexChannel();

  for(list<DeferredLight>::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    it->l->direction()->enableUniform(dirLoc_);
    it->l->diffuse()->enableUniform(diffuseLoc_);
    it->l->specular()->enableUniform(specularLoc_);

    if(it->sm.get()) {
      it->sm->shadowMap()->texture()->activateBind(smChannel);
      glUniform1i(shadowMapLoc_, smChannel);
      it->sm->shadowMapSize()->enableUniform(shadowMapSizeLoc_);
      it->sm->shadowFarUniform()->enableUniform(shadowFarLoc_);
      it->sm->shadowMatUniform()->enableUniform(shadowMatricesLoc_);
    }

    mesh_->draw(1);
  }

  rs->releaseTexChannel();
}

//////////////////
//////////////////
//////////////////

DeferredEnvLight::DeferredEnvLight()
: State()
{
  mesh_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
}

void DeferredEnvLight::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  _cfg.define("NUM_SHADOW_MAP_SLICES",
      FORMAT_STRING(DirectionalShadowMap::numSplits()));
  _cfg.define("USE_SKY_COLOR", "TRUE");
  shader_->createShader(_cfg.cfg(), "shading.deferred.directional");

  Shader *s = shader_->shader().get();
  dirLoc_ = s->uniformLocation("lightDirection");
  specularLoc_ = s->uniformLocation("lightSpecular");
  skyMapLoc_ = s->uniformLocation("skyColorTexture");

  shadowMapSizeLoc_ = s->uniformLocation("shadowMapSize");
  shadowMapLoc_ = s->uniformLocation("shadowMap");
  shadowMatricesLoc_ = s->uniformLocation("shadowMatrices");
  shadowFarLoc_ = s->uniformLocation("shadowFar");

}
void DeferredEnvLight::addLight(
    const ref_ptr<DirectionalLight> &l,
    const ref_ptr<TextureCube> &skyMap,
    const ref_ptr<DirectionalShadowMap> &sm)
{
  lights_.push_back( DeferredLight(l,sm,skyMap) );

  list<DeferredLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;
}
void DeferredEnvLight::removeLight(Light *l)
{
  lights_.erase(lightIterators_[l]);
}
void DeferredEnvLight::enable(RenderState *rs) {
  State::enable(rs);
  GLuint smChannel = rs->nextTexChannel();
  GLuint skyChannel = rs->nextTexChannel();

  for(list<DeferredLight>::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    it->l->direction()->enableUniform(dirLoc_);
    it->l->specular()->enableUniform(specularLoc_);

    it->skyMap->activateBind(skyChannel);
    glUniform1i(skyMapLoc_, skyChannel);

    if(it->sm.get()) {
      it->sm->shadowMap()->texture()->activateBind(smChannel);
      glUniform1i(shadowMapLoc_, smChannel);
      it->sm->shadowMapSize()->enableUniform(shadowMapSizeLoc_);
      it->sm->shadowFarUniform()->enableUniform(shadowFarLoc_);
      it->sm->shadowMatUniform()->enableUniform(shadowMatricesLoc_);
    }

    mesh_->draw(1);
  }

  rs->releaseTexChannel();
  rs->releaseTexChannel();
}

/////////////
/////////////

DeferredPointLight::DeferredPointLight()
: State()
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
  shadowMapLoc_ = s->uniformLocation("shadowMap");
  shadowFarLoc_ = s->uniformLocation("shadowFar");
  shadowNearLoc_ = s->uniformLocation("shadowNear");
}
void DeferredPointLight::addLight(
    const ref_ptr<PointLight> &l,
    const ref_ptr<PointShadowMap> &sm)
{
  lights_.push_back( DeferredLight(l,sm) );

  list<DeferredLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;
}
void DeferredPointLight::removeLight(Light *l)
{
  lights_.erase( lightIterators_[l] );
}
void DeferredPointLight::enable(RenderState *rs)
{
  State::enable(rs);
  GLuint smChannel = rs->nextTexChannel();

  for(list<DeferredLight>::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    it->l->position()->enableUniform(posLoc_);
    it->l->radius()->enableUniform(radiusLoc_);
    it->l->diffuse()->enableUniform(diffuseLoc_);
    it->l->specular()->enableUniform(specularLoc_);

    if(it->sm.get()) {
      it->sm->shadowMap()->texture()->activateBind(smChannel);
      glUniform1i(shadowMapLoc_, smChannel);
      it->sm->shadowMapSize()->enableUniform(shadowMapSizeLoc_);
      it->sm->far()->enableUniform(shadowFarLoc_);
      it->sm->near()->enableUniform(shadowNearLoc_);
    }

    mesh_->draw(1);
  }

  rs->releaseTexChannel();
}

/////////////
/////////////

DeferredSpotLight::DeferredSpotLight()
: State()
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
  shadowMapLoc_ = s->uniformLocation("shadowMap");
  shadowMatLoc_ = s->uniformLocation("shadowMatrix");
}
void DeferredSpotLight::addLight(
    const ref_ptr<SpotLight> &l,
    const ref_ptr<SpotShadowMap> &sm)
{
  lights_.push_back( DeferredLight(l,sm) );

  list<DeferredLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;
}
void DeferredSpotLight::removeLight(Light *l)
{
  lights_.erase( lightIterators_[l] );
}
void DeferredSpotLight::enable(RenderState *rs)
{
  State::enable(rs);
  GLuint smChannel = rs->nextTexChannel();

  for(list<DeferredLight>::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    ref_ptr<SpotLight> l = it->l;
    // XXX
    if(it->dirStamp != l->spotDirection()->stamp()) {
      it->dirStamp = l->spotDirection()->stamp();
      l->updateConeMatrix();
    }

    l->spotDirection()->enableUniform(dirLoc_);
    l->coneAngle()->enableUniform(coneAnglesLoc_);
    l->position()->enableUniform(posLoc_);
    l->radius()->enableUniform(radiusLoc_);
    l->diffuse()->enableUniform(diffuseLoc_);
    l->specular()->enableUniform(specularLoc_);
    l->coneMatrix()->enableUniform(coneMatLoc_);

    if(it->sm.get()) {
      it->sm->shadowMap()->texture()->activateBind(smChannel);
      glUniform1i(shadowMapLoc_, smChannel);
      it->sm->shadowMapSize()->enableUniform(shadowMapSizeLoc_);
      it->sm->shadowMatUniform()->enableUniform(shadowMatLoc_);
    }

    mesh_->draw(1);
  }

  rs->releaseTexChannel();
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

  envState_ = ref_ptr<DeferredEnvLight>::manage(new DeferredEnvLight);
  envShadowState_ = ref_ptr<DeferredEnvLight>::manage(new DeferredEnvLight);
  envShadowState_->shaderDefine("USE_SHADOW_MAP", "TRUE");
  envShadowState_->shaderDefine("SHADOW_MAP_FILTER", "Single");

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
  envState_->createShader(_cfg.cfg());
  envShadowState_->createShader(_cfg.cfg());
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

void DeferredShading::addEnvironmentLight(
    const ref_ptr<DirectionalLight> &l,
    const ref_ptr<TextureCube> &skyMap)
{
  if(envState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(envState_));
  }
  envState_->addLight(l, skyMap, ref_ptr<DirectionalShadowMap>());
}
void DeferredShading::addEnvironmentLight(
    const ref_ptr<DirectionalLight> &l,
    const ref_ptr<TextureCube> &skyMap,
    const ref_ptr<DirectionalShadowMap> &sm)
{
  if(envShadowState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(envShadowState_));
  }
  envShadowState_->addLight(l,skyMap,sm);
}

void DeferredShading::addLight(
    const ref_ptr<DirectionalLight> &l,
    const ref_ptr<DirectionalShadowMap> &sm)
{
  if(dirShadowState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(dirShadowState_));
  }
  dirShadowState_->addLight(l,sm);
}
void DeferredShading::addLight(const ref_ptr<DirectionalLight> &l)
{
  if(dirState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(dirState_));
  }
  dirState_->addLight(l, ref_ptr<DirectionalShadowMap>());
}

void DeferredShading::addLight(
    const ref_ptr<PointLight> &l,
    const ref_ptr<PointShadowMap> &sm)
{
  if(pointShadowState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(pointShadowState_));
  }
  pointShadowState_->addLight(l,sm);
}
void DeferredShading::addLight(
    const ref_ptr<SpotLight> &l,
    const ref_ptr<SpotShadowMap> &sm)
{
  if(spotShadowState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(spotShadowState_));
  }
  spotShadowState_->addLight(l,sm);
}
void DeferredShading::addLight(const ref_ptr<PointLight> &l)
{
  if(pointState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(pointState_));
  }
  pointState_->addLight(l, ref_ptr<PointShadowMap>());
}
void DeferredShading::addLight(const ref_ptr<SpotLight> &l)
{
  if(spotState_->lights_.empty()) {
    deferredShadingSequence_->joinStates(ref_ptr<State>::cast(spotState_));
  }
  spotState_->addLight(l, ref_ptr<SpotShadowMap>());
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
  dirShadowState_->shaderDefine("SHADOW_MAP_FILTER", shadowFilterMode(mode));
  envShadowState_->shaderDefine("SHADOW_MAP_FILTER", shadowFilterMode(mode));
}
void DeferredShading::setPointFiltering(ShadowMap::FilterMode mode)
{
  pointShadowState_->shaderDefine("SHADOW_MAP_FILTER", shadowFilterMode(mode));
}
void DeferredShading::setSpotFiltering(ShadowMap::FilterMode mode)
{
  spotShadowState_->shaderDefine("SHADOW_MAP_FILTER", shadowFilterMode(mode));
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
  // XXX: remove
  disjoinStates(ref_ptr<State>::cast(l));
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

