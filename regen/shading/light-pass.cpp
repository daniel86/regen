/*
 * light-pass.cpp
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#include <regen/states/shader-configurer.h>
#include <regen/meshes/cone.h>
#include <regen/meshes/box.h>
#include <regen/shading/shadow-map.h>

#include "light-pass.h"
using namespace regen;

LightPass::LightPass(Light::Type type, const string &shaderKey)
: State(), lightType_(type), shaderKey_(shaderKey)
{
  switch(lightType_) {
  case Light::DIRECTIONAL:
    mesh_ = ref_ptr<Mesh>::cast(Rectangle::getUnitQuad());
    break;
  case Light::SPOT:
    mesh_ = ref_ptr<Mesh>::cast(ConeClosed::getBaseCone());
    joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));
    break;
  case Light::POINT:
    mesh_ = ref_ptr<Mesh>::cast(Box::getUnitCube());
    joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));
    break;
  }
  shadowFiltering_ = ShadowMap::FILTERING_NONE;
  numShadowLayer_ = 1;

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
}

void LightPass::setShadowFiltering(ShadowMap::FilterMode mode)
{
  GLboolean usedMoments = ShadowMap::useShadowMoments(shadowFiltering_);

  shadowFiltering_ = mode;
  shaderDefine("USE_SHADOW_MAP", "TRUE");
  shaderDefine("USE_SHADOW_SAMPLER", ShadowMap::useShadowSampler(mode) ? "TRUE" : "FALSE");
  shaderDefine("SHADOW_MAP_FILTER", ShadowMap::shadowFilterMode(mode));

  if(usedMoments != ShadowMap::useShadowMoments(shadowFiltering_))
  {
    for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
    {
      if(!it->sm.get()) continue;
      it->sm->setComputeMoments();
    }
  }
}

void LightPass::setShadowLayer(GLuint numLayer)
{
  numShadowLayer_ = numLayer;
  // change number of layers for added lights
  for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    if(!it->sm.get()) continue;
    it->sm->set_shadowLayer(numLayer);
  }
}

void LightPass::addLight(
    const ref_ptr<Light> &l,
    const ref_ptr<ShadowMap> &sm,
    const list< ref_ptr<ShaderInput> > &inputs)
{
  LightPassLight light;
  lights_.push_back(light);

  list<LightPassLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;

  it->light = l;
  it->sm = sm;
  it->inputs = inputs;

  if(sm.get() && ShadowMap::useShadowMoments(shadowFiltering_))
  { sm->setComputeMoments(); }
  if(shader_->shader().get())
  { addLightInput(*it); }
}
void LightPass::removeLight(Light *l)
{
  lights_.erase( lightIterators_[l] );
}
GLboolean LightPass::empty() const
{
  return lights_.empty();
}
GLboolean LightPass::hasLight(Light *l) const
{
  return lightIterators_.count(l)>0;
}

void LightPass::createShader(const ShaderState::Config &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  _cfg.define("NUM_SHADOW_LAYER", FORMAT_STRING(numShadowLayer_));
  shader_->createShader(_cfg.cfg(), shaderKey_);

  for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  { addLightInput(*it); }
  shadowMapLoc_ = shader_->shader()->uniformLocation("shadowTexture");
}

void LightPass::addLightInput(LightPassLight &light)
{
#define __ADD_INPUT__(x,y) addInputLocation(light, ref_ptr<ShaderInput>::cast(x), y);
  // clear list of uniform loactions
  light.inputLocations.clear();
  // add user specified uniforms
  for(list< ref_ptr<ShaderInput> >::iterator jt=light.inputs.begin(); jt!=light.inputs.end(); ++jt)
  {
    ref_ptr<ShaderInput> &in = *jt;
    __ADD_INPUT__(in, in->name());
  }

  // add shadow uniforms
  if(light.sm.get()) {
    __ADD_INPUT__(light.sm->shadowFar(), "shadowFar");
    __ADD_INPUT__(light.sm->shadowNear(), "shadowNear");
    __ADD_INPUT__(light.sm->shadowMat(), "shadowMatrix");
  }

  // add light uniforms
  switch(lightType_) {
  case Light::DIRECTIONAL: {
    __ADD_INPUT__(light.light->direction(), "lightDirection");
    __ADD_INPUT__(light.light->diffuse(), "lightDiffuse");
    __ADD_INPUT__(light.light->specular(), "lightSpecular");
  }
  break;
  case Light::SPOT: {
    __ADD_INPUT__(light.light->position(), "lightPosition");
    __ADD_INPUT__(light.light->direction(), "lightDirection");
    __ADD_INPUT__(light.light->radius(), "lightRadius");
    __ADD_INPUT__(light.light->coneAngle(), "lightConeAngles");
    __ADD_INPUT__(light.light->diffuse(), "lightDiffuse");
    __ADD_INPUT__(light.light->specular(), "lightSpecular");
    __ADD_INPUT__(light.light->coneMatrix(), "modelMatrix");
  }
  break;
  case Light::POINT: {
    __ADD_INPUT__(light.light->position(), "lightPosition");
    __ADD_INPUT__(light.light->radius(), "lightRadius");
    __ADD_INPUT__(light.light->diffuse(), "lightDiffuse");
    __ADD_INPUT__(light.light->specular(), "lightSpecular");
  }
  break;
  }
#undef __ADD_INPUT__
}

void LightPass::addInputLocation(LightPassLight &l,
    const ref_ptr<ShaderInput> &in, const string &name)
{
  Shader *s = shader_->shader().get();
  GLint loc = s->uniformLocation(name);
  if(loc>0) {
    l.inputLocations.push_back( ShaderInputLocation(in, loc) );
  }
}

void LightPass::enable(RenderState *rs)
{
  State::enable(rs);
  GLuint smChannel = rs->reserveTextureChannel();

  for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    LightPassLight &l = *it;

    // activate shadow map if specified
    if(l.sm.get()) {
      switch(shadowFiltering_) {
      case ShadowMap::FILTERING_VSM:
        l.sm->shadowMoments()->activate(smChannel);
        break;
      default:
        l.sm->shadowDepth()->activate(smChannel);
        break;
      }
      glUniform1i(shadowMapLoc_, smChannel);
    }
    // enable light pass uniforms
    for(list<ShaderInputLocation>::iterator
        jt=l.inputLocations.begin(); jt!=l.inputLocations.end(); ++jt)
    {
      ShaderInputLocation &in = *jt;
      in.input->enableUniform(in.location);
    }

    mesh_->draw(1);
  }

  rs->releaseTextureChannel();
}
