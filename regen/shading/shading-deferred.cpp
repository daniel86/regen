/*
 * deferred.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <regen/states/state-configurer.h>
#include <regen/shading/ambient-occlusion.h>

#include "shading-deferred.h"
using namespace regen;

DeferredShading::DeferredShading()
: State(), hasShaderConfig_(GL_FALSE), hasAmbient_(GL_FALSE), hasAO_(GL_FALSE)
{
  // accumulate light using add blending
  joinStates(ref_ptr<BlendState>::alloc(BLEND_MODE_ADD));

  aoState_ = ref_ptr<FullscreenPass>::alloc("regen.shading.ssao.sample");
  aoState_->joinStatesFront(ref_ptr<BlendState>::alloc(BLEND_MODE_MULTIPLY));

  ambientState_ = ref_ptr<FullscreenPass>::alloc("regen.shading.deferred.ambient");
  ambientLight_ = ref_ptr<ShaderInput3f>::alloc("lightAmbient");
  ambientLight_->setUniformData(Vec3f(0.1f));
  ambientState_->joinShaderInput(ambientLight_);

  dirState_ = ref_ptr<LightPass>::alloc(
      Light::DIRECTIONAL, "regen.shading.deferred.directional");
  dirShadowState_ = ref_ptr<LightPass>::alloc(
      Light::DIRECTIONAL, "regen.shading.deferred.directional");
  dirShadowState_->setShadowFiltering(ShadowMap::FILTERING_NONE);

  pointState_ = ref_ptr<LightPass>::alloc(
      Light::POINT, "regen.shading.deferred.point");
  pointShadowState_ = ref_ptr<LightPass>::alloc(
      Light::POINT, "regen.shading.deferred.point");
  pointShadowState_->setShadowFiltering(ShadowMap::FILTERING_NONE);

  spotState_ = ref_ptr<LightPass>::alloc(
      Light::SPOT, "regen.shading.deferred.spot");
  spotShadowState_ = ref_ptr<LightPass>::alloc(
      Light::SPOT, "regen.shading.deferred.spot");
  spotShadowState_->setShadowFiltering(ShadowMap::FILTERING_NONE);

  lightSequence_ = ref_ptr<StateSequence>::alloc();
  joinStates(lightSequence_);
}

void DeferredShading::setUseAmbientOcclusion()
{
  if(hasAO_) return;
  hasAO_ = GL_TRUE;

  // update ao texture
  updateAOState_ = ref_ptr<AmbientOcclusion>::alloc(gNorWorldTexture_->texture(), 0.5);
  updateAOState_->joinStatesFront(ref_ptr<BlendState>::alloc(BLEND_MODE_SRC));
  joinStates(updateAOState_);
  // combine with deferred shading result
  ref_ptr<TextureState> tex = ref_ptr<TextureState>::alloc(updateAOState_->output(), "aoTexture");
  aoState_->joinStatesFront(tex);
  joinStates(aoState_);

  if(hasShaderConfig_) {
    {
      StateConfigurer _cfg(shaderCfg_);
      _cfg.addState(updateAOState_.get());
      updateAOState_->createShader(_cfg.cfg());
    }
    {
      StateConfigurer _cfg(shaderCfg_);
      _cfg.addState(aoState_.get());
      aoState_->createShader(_cfg.cfg());
    }
  }
}

void DeferredShading::setUseAmbientLight()
{
  if(!hasAmbient_) {
    lightSequence_->joinStates(ambientState_);
    hasAmbient_ = GL_TRUE;
  }
  if(hasShaderConfig_) {
    StateConfigurer _cfg(shaderCfg_);
    _cfg.addState(ambientState_.get());
    ambientState_->createShader(_cfg.cfg());
  }
}

void DeferredShading::createShader(StateConfig &cfg)
{
  {
    StateConfigurer _cfg(cfg);
    _cfg.addState(this);
    shaderCfg_ = _cfg.cfg();
    hasShaderConfig_ = GL_TRUE;
  }

  if(!dirState_->empty())
  { dirState_->createShader(shaderCfg_); }
  if(!pointState_->empty())
  { pointState_->createShader(shaderCfg_); }
  if(!spotState_->empty())
  { spotState_->createShader(shaderCfg_); }
  if(!dirShadowState_->empty())
  { dirShadowState_->createShader(shaderCfg_); }
  if(!pointShadowState_->empty())
  { pointShadowState_->createShader(shaderCfg_); }
  if(!spotShadowState_->empty())
  { spotShadowState_->createShader(shaderCfg_); }

  if(hasAmbient_) {
    StateConfigurer _cfg(shaderCfg_);
    _cfg.addState(ambientState_.get());
    ambientState_->createShader(_cfg.cfg());
  }

  if(hasAO_) {
    {
      StateConfigurer _cfg(shaderCfg_);
      _cfg.addState(updateAOState_.get());
      updateAOState_->createShader(_cfg.cfg());
    }
    {
      StateConfigurer _cfg(shaderCfg_);
      _cfg.addState(aoState_.get());
      aoState_->createShader(_cfg.cfg());
    }
  }
}

void DeferredShading::set_gBuffer(
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &norWorldTexture,
    const ref_ptr<Texture> &diffuseTexture,
    const ref_ptr<Texture> &specularTexture)
{
  if(gDepthTexture_.get()) {
    disjoinStates(gDepthTexture_);
    disjoinStates(gDiffuseTexture_);
    disjoinStates(gSpecularTexture_);
    disjoinStates(gNorWorldTexture_);
  }

  gDepthTexture_ = ref_ptr<TextureState>::alloc(depthTexture, "gDepthTexture");
  joinStatesFront(gDepthTexture_);

  gNorWorldTexture_ = ref_ptr<TextureState>::alloc(norWorldTexture, "gNorWorldTexture");
  joinStatesFront(gNorWorldTexture_);

  gDiffuseTexture_ = ref_ptr<TextureState>::alloc(diffuseTexture, "gDiffuseTexture");
  joinStatesFront(gDiffuseTexture_);

  gSpecularTexture_ = ref_ptr<TextureState>::alloc(specularTexture, "gSpecularTexture");
  joinStatesFront(gSpecularTexture_);
}

ref_ptr<LightPass> DeferredShading::getLightState(
    const ref_ptr<Light> &light, const ref_ptr<ShadowMap> &shadowMap)
{
  switch(light->lightType()) {
  case Light::DIRECTIONAL:
    return (shadowMap.get() ? dirShadowState_ : dirState_);
  case Light::POINT:
    return (shadowMap.get() ? pointShadowState_ : pointState_);
  case Light::SPOT:
    return (shadowMap.get() ? spotShadowState_ : spotState_);
  }
  return ref_ptr<LightPass>();
}

void DeferredShading::addLight(
    const ref_ptr<Light> &light,
    const ref_ptr<ShadowMap> &shadowMap)
{
  ref_ptr<LightPass> lightState = getLightState(light,shadowMap);
  if(!lightState.get()) {
    REGEN_WARN("Unknown light type.");
    return;
  }
  if(lightState->empty()) {
    lightSequence_->joinStates(lightState);
    if(hasShaderConfig_) {
      lightState->createShader(shaderCfg_);
    }
  }
  list< ref_ptr<ShaderInput> > inputs; // no additional input needed
  lightState->addLight(light, shadowMap, inputs);
}
void DeferredShading::addLight(const ref_ptr<Light> &light)
{
  addLight(light, ref_ptr<ShadowMap>());
}

void DeferredShading::removeLight(Light *l,
    const ref_ptr<LightPass> &lightState)
{
  lightState->removeLight(l);
  if(lightState->empty()) {
    lightSequence_->disjoinStates(lightState);
  }
}
void DeferredShading::removeLight(Light *l)
{
  if(dirState_->hasLight(l))
  { removeLight(l, dirState_); }
  if(dirShadowState_->hasLight(l))
  { removeLight(l, dirShadowState_); }
  if(pointState_->hasLight(l))
  { removeLight(l, pointState_); }
  if(pointShadowState_->hasLight(l))
  { removeLight(l, pointShadowState_); }
  if(spotState_->hasLight(l))
  { removeLight(l, spotState_); }
  if(spotShadowState_->hasLight(l))
  { removeLight(l, spotShadowState_); }
}

const ref_ptr<LightPass>& DeferredShading::dirState() const
{ return dirState_; }
const ref_ptr<LightPass>& DeferredShading::dirShadowState() const
{ return dirShadowState_; }
const ref_ptr<LightPass>& DeferredShading::pointState() const
{ return pointState_; }
const ref_ptr<LightPass>& DeferredShading::pointShadowState() const
{ return pointShadowState_; }
const ref_ptr<LightPass>& DeferredShading::spotState() const
{ return spotState_; }
const ref_ptr<LightPass>& DeferredShading::spotShadowState() const
{ return spotShadowState_; }
const ref_ptr<FullscreenPass>& DeferredShading::ambientState() const
{ return ambientState_; }
const ref_ptr<ShaderInput3f>& DeferredShading::ambientLight() const
{ return ambientLight_; }
const ref_ptr<AmbientOcclusion>& DeferredShading::ambientOcclusion() const
{ return updateAOState_; }
