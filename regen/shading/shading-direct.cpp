/*
 * direct.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <regen/utility/string-util.h>

#include "shading-direct.h"
using namespace regen;

#define __NAME__(x,id) REGEN_STRING(x << id)

DirectShading::DirectShading() : State(), idCounter_(0)
{
  shaderDefine("NUM_LIGHTS", "0");

  ambientLight_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("ambientLight"));
  ambientLight_->setUniformData(Vec3f(0.2f));
  joinShaderInput(ambientLight_);
}

void DirectShading::updateDefine(DirectLight &l, GLuint lightIndex)
{
  shaderDefine(
      REGEN_STRING("LIGHT" << lightIndex << "_ID"),
      REGEN_STRING(l.id_));
  shaderDefine(
      __NAME__("LIGHT_IS_ATTENUATED",l.id_),
      l.light_->isAttenuated() ? "TRUE" : "FALSE");

  string lightType = "UNKNOWN";
  switch(l.light_->lightType()) {
  case Light::DIRECTIONAL:
    lightType = "DIRECTIONAL"; break;
  case Light::POINT:
    lightType = "POINT"; break;
  case Light::SPOT:
    lightType = "SPOT"; break;
  }
  shaderDefine(__NAME__("LIGHT_TYPE",l.id_), lightType);

  // handle shadow map defined
  if(l.sm_.get()) {
    shaderDefine(__NAME__("USE_SHADOW_MAP",l.id_), "TRUE");
    shaderDefine(__NAME__("SHADOW_MAP_FILTER",l.id_),
        ShadowMap::shadowFilterMode(l.shadowFilter_));
    shaderDefine(__NAME__("USE_SHADOW_SAMPLER",l.id_),
        ShadowMap::useShadowSampler(l.shadowFilter_) ? "TRUE" : "FALSE");

    const ref_ptr<Texture> &shadowMap = (
        ShadowMap::useShadowMoments(l.shadowFilter_) ? l.sm_->shadowMoments() : l.sm_->shadowDepth());
    if(dynamic_cast<Texture3D*>(shadowMap.get())) {
      Texture3D *tex3d = dynamic_cast<Texture3D*>(shadowMap.get());
      shaderDefine(__NAME__("NUM_SHADOW_LAYER",l.id_), REGEN_STRING(tex3d->depth()));
    }
  }
  else {
    shaderDefine(__NAME__("USE_SHADOW_MAP",l.id_), "FALSE");
  }
}

void DirectShading::addLight(const ref_ptr<Light> &l)
{
  addLight(l, ref_ptr<ShadowMap>(), ShadowMap::FILTERING_NONE);
}
void DirectShading::addLight(
    const ref_ptr<Light> &l,
    const ref_ptr<ShadowMap> &sm,
    ShadowMap::FilterMode shadowFilter)
{
  GLuint lightID = ++idCounter_;
  GLuint lightIndex = lights_.size();

  {
    DirectLight dl;
    dl.id_ = lightID;
    dl.light_ = l;
    dl.sm_ = sm;
    dl.shadowFilter_ = shadowFilter;
    lights_.push_back(dl);
  }
  DirectLight &directLight = *lights_.rbegin();
  // remember the number of lights used
  shaderDefine("NUM_LIGHTS", REGEN_STRING(lightIndex+1));
  updateDefine(directLight, lightIndex);

  // join light shader inputs using a name override
  {
    const ShaderInputState::InputContainer &in = l->inputs();
    for(ShaderInputState::InputItConst it=in.begin(); it!=in.end(); ++it)
    { joinShaderInput(it->in_, __NAME__(it->in_->name(), lightID)); }
  }
  if(sm.get()) {
    const ShaderInputState::InputContainer &in = sm->inputs();
    for(ShaderInputState::InputItConst it=in.begin(); it!=in.end(); ++it)
    { joinShaderInput(it->in_, __NAME__(it->in_->name(), lightID)); }
    // we have to explicitly join the shadow map
    const ref_ptr<Texture> &shadowMap = (ShadowMap::useShadowMoments(shadowFilter) ?
        sm->shadowMoments() : sm->shadowDepth());
    directLight.shadowMap_ = ref_ptr<TextureState>::manage(
        new TextureState(shadowMap, __NAME__("shadowTexture",lightID)));
    joinStates(directLight.shadowMap_);
  }
}

void DirectShading::removeLight(const ref_ptr<Light> &l)
{
  list<DirectLight>::iterator it;
  for(it=lights_.begin(); it!=lights_.end(); ++it)
  {
    ref_ptr<Light> &x = it->light_;
    if(x.get()==l.get()) {
      break;
    }
  }
  if(it == lights_.end()) { return; }

  DirectLight &directLight = *it;
  {
    const ShaderInputState::InputContainer &in = l->inputs();
    for(ShaderInputState::InputItConst it=in.begin(); it!=in.end(); ++it)
    { disjoinShaderInput(it->in_); }
  }
  if(directLight.sm_.get()) {
    const ShaderInputState::InputContainer &in = directLight.sm_->inputs();
    for(ShaderInputState::InputItConst it=in.begin(); it!=in.end(); ++it)
    { disjoinShaderInput(it->in_); }
    disjoinStates(directLight.shadowMap_);
  }
  lights_.erase(it);

  GLuint numLights = lights_.size(), lightIndex=0;
  // update shader defines
  shaderDefine("NUM_LIGHTS", REGEN_STRING(numLights));
  for(list<DirectLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    updateDefine(*it, lightIndex);
    ++lightIndex;
  }
}

const ref_ptr<ShaderInput3f>& DirectShading::ambientLight() const
{ return ambientLight_; }
