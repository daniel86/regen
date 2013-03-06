/*
 * deferred-light.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include "deferred-light.h"
namespace ogle {

string glsl_shadowFilterMode(ShadowMap::FilterMode f) {
  switch(f) {
  case ShadowMap::FILTERING_NONE: return "Single";
  case ShadowMap::FILTERING_PCF_GAUSSIAN: return "Gaussian";
  case ShadowMap::FILTERING_VSM: return "VSM";
  }
  return "Single";
}
GLboolean glsl_useShadowMoments(ShadowMap::FilterMode f)
{
  switch(f) {
  case ShadowMap::FILTERING_VSM:
    return GL_TRUE;
  default:
    return GL_FALSE;
  }
}
GLboolean glsl_useShadowSampler(ShadowMap::FilterMode f)
{
  switch(f) {
  case ShadowMap::FILTERING_VSM:
    return GL_FALSE;
  default:
    return GL_TRUE;
  }
}

DeferredLight::DeferredLight()
: State()
{
  shadowFiltering_ = ShadowMap::FILTERING_NONE;
}

GLboolean DeferredLight::empty() const
{
  return lights_.empty();
}
GLboolean DeferredLight::hasLight(Light *l) const
{
  return lightIterators_.count(l)>0;
}

void DeferredLight::addLight(const ref_ptr<Light> &l, const ref_ptr<ShadowMap> &sm)
{
  lights_.push_back( DLight(l,sm) );

  list<DLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;

  if(sm.get() && glsl_useShadowMoments(shadowFiltering_)) {
    sm->setComputeMoments();
  }
}
void DeferredLight::removeLight(Light *l)
{
  lights_.erase( lightIterators_[l] );
}

void DeferredLight::setShadowFiltering(ShadowMap::FilterMode mode)
{
  GLboolean usedMoments = glsl_useShadowMoments(shadowFiltering_);

  shadowFiltering_ = mode;
  shaderDefine("USE_SHADOW_MAP", "TRUE");
  shaderDefine("USE_SHADOW_SAMPLER", glsl_useShadowSampler(mode) ? "TRUE" : "FALSE");
  shaderDefine("SHADOW_MAP_FILTER", glsl_shadowFilterMode(mode));

  if(usedMoments != glsl_useShadowMoments(shadowFiltering_))
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
    sm->shadowMoments()->activate(channel);
    break;
  default:
    sm->shadowDepth()->activate(channel);
    break;
  }
  glUniform1i(shadowMapLoc_, channel);
}

}
