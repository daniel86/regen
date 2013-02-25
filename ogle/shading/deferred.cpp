/*
 * deferred.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include "deferred.h"

#include <ogle/states/shader-configurer.h>

DeferredShading::DeferredShading()
: State(), hasAmbient_(GL_FALSE)
{
  // accumulate light using add blending
  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));

  ambientState_ = ref_ptr<DeferredAmbientLight>::manage(new DeferredAmbientLight);
  dirState_ = ref_ptr<DeferredDirLight>::manage(new DeferredDirLight);
  pointState_ = ref_ptr<DeferredPointLight>::manage(new DeferredPointLight);
  spotState_ = ref_ptr<DeferredSpotLight>::manage(new DeferredSpotLight());

  dirShadowState_ = ref_ptr<DeferredDirLight>::manage(new DeferredDirLight);
  dirShadowState_->shaderDefine("USE_SHADOW_MAP", "TRUE");
  dirShadowState_->shaderDefine("SHADOW_MAP_FILTER", "Single");

  pointShadowState_ = ref_ptr<DeferredPointLight>::manage(new DeferredPointLight);
  pointShadowState_->shaderDefine("USE_SHADOW_MAP", "TRUE");
  pointShadowState_->shaderDefine("SHADOW_MAP_FILTER", "Single");

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

ref_ptr<DeferredLight> DeferredShading::getLightState(
    const ref_ptr<Light> &light, const ref_ptr<ShadowMap> &shadowMap)
{
  if(dynamic_cast<DirectionalLight*>(light.get())) {
    return ref_ptr<DeferredLight>::cast(
        (shadowMap.get() ? dirShadowState_ : dirState_));
  }
  else if(dynamic_cast<PointLight*>(light.get())) {
    return ref_ptr<DeferredLight>::cast(
        (shadowMap.get() ? pointShadowState_ : pointState_));
  }
  else if(dynamic_cast<SpotLight*>(light.get())) {
    return ref_ptr<DeferredLight>::cast(
        (shadowMap.get() ? spotShadowState_ : spotState_));
  }
  else {
    return ref_ptr<DeferredLight>();
  }
}

void DeferredShading::setUseAmbientLight()
{
  if(!hasAmbient_) {
    lightSequence_->joinStates(ref_ptr<State>::cast(ambientState_));
    hasAmbient_ = GL_TRUE;
  }
}

void DeferredShading::addLight(
    const ref_ptr<Light> &light,
    const ref_ptr<ShadowMap> &shadowMap)
{
  ref_ptr<DeferredLight> lightState = getLightState(light,shadowMap);
  if(!lightState.get()) {
    WARN_LOG("Unknown light type.");
    return;
  }
  if(lightState->empty()) {
    lightSequence_->joinStates(ref_ptr<State>::cast(lightState));
  }
  lightState->addLight(light, shadowMap);
}
void DeferredShading::addLight(const ref_ptr<Light> &light)
{
  addLight(light, ref_ptr<ShadowMap>());
}

void DeferredShading::removeLight(Light *l,
    const ref_ptr<DeferredLight> &lightState)
{
  lightState->removeLight(l);
  if(lightState->empty()) {
    lightSequence_->disjoinStates(ref_ptr<State>::cast(lightState));
  }
}
void DeferredShading::removeLight(Light *l)
{
  if(dirState_->hasLight(l)) {
    removeLight(l, ref_ptr<DeferredLight>::cast(dirState_));
  }
  if(dirShadowState_->hasLight(l)) {
    removeLight(l, ref_ptr<DeferredLight>::cast(dirShadowState_));
  }
  if(pointState_->hasLight(l)) {
    removeLight(l, ref_ptr<DeferredLight>::cast(pointState_));
  }
  if(pointShadowState_->hasLight(l)) {
    removeLight(l, ref_ptr<DeferredLight>::cast(pointShadowState_));
  }
  if(spotState_->hasLight(l)) {
    removeLight(l, ref_ptr<DeferredLight>::cast(spotState_));
  }
  if(spotShadowState_->hasLight(l)) {
    removeLight(l, ref_ptr<DeferredLight>::cast(spotShadowState_));
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
