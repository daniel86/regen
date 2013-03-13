/*
 * light-pass.cpp
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/cone.h>
#include <ogle/meshes/box.h>
#include <ogle/shading/spot-shadow-map.h>
#include <ogle/shading/point-shadow-map.h>
#include <ogle/shading/directional-shadow-map.h>

#include "light-pass.h"
using namespace ogle;

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

LightPass::LightPass(LightType type, const string &shaderKey)
: State(), lightType_(type), shaderKey_(shaderKey)
{
  switch(lightType_) {
  case DIRECTIONAL:
    mesh_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());
    break;
  case SPOT:
    mesh_ = ref_ptr<MeshState>::cast(ConeClosed::getBaseCone());
    joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));
    break;
  case POINT:
    mesh_ = ref_ptr<MeshState>::cast(Box::getUnitCube());
    joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));
    break;
  }
  shadowFiltering_ = ShadowMap::FILTERING_NONE;
  numShadowLayer_ = 1;

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
}

void LightPass::addLight(
    const ref_ptr<Light> &l,
    const ref_ptr<ShadowMap> &sm,
    const list< ref_ptr<ShaderInput> > &inputs)
{
  LightPassLight light;
  light.light = l;
  light.sm = sm;
  light.inputs = inputs;
  lights_.push_back(light);

  list<LightPassLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;

  if(sm.get() && ShadowMap::useShadowMoments(shadowFiltering_))
  { sm->setComputeMoments(); }
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
  switch(lightType_) {
  case DIRECTIONAL:
    for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
    {
      if(!it->sm.get()) continue;
      DirectionalShadowMap *sm = (DirectionalShadowMap*) it->sm.get();
      sm->set_numShadowLayer(numLayer);
    }
    break;
  case SPOT:
  case POINT:
    break;
  }
}

void LightPass::createShader(const ShaderState::Config &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  _cfg.define("NUM_SHADOW_LAYER", FORMAT_STRING(numShadowLayer_));
  shader_->createShader(_cfg.cfg(), shaderKey_);

#define __ADD_INPUT__(x,y) addInputLocation(*it, ref_ptr<ShaderInput>::cast(x), y);
  for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    LightPassLight &l = *it;
    // clear list of uniform loactions
    l.inputLocations.clear();

    // add user specified uniforms
    for(list< ref_ptr<ShaderInput> >::iterator jt=l.inputs.begin(); jt!=l.inputs.end(); ++jt)
    {
      ref_ptr<ShaderInput> &in = *jt;
      __ADD_INPUT__(in, in->name());
    }
  }
  // add light/shadow uniforms
  switch(lightType_) {
  case DIRECTIONAL:
    for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
    {
      DirectionalLight *l = (DirectionalLight*) it->light.get();
      __ADD_INPUT__(l->direction(), "lightDirection");
      __ADD_INPUT__(l->diffuse(), "lightDiffuse");
      __ADD_INPUT__(l->specular(), "lightSpecular");
      if(it->sm.get()) {
        DirectionalShadowMap *sm = (DirectionalShadowMap*) it->sm.get();
        __ADD_INPUT__(sm->shadowFarUniform(), "shadowFar");
        __ADD_INPUT__(sm->shadowMatUniform(), "shadowMatrices");
      }
    }
    break;
  case SPOT:
    for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
    {
      SpotLight *l = (SpotLight*) it->light.get();
      __ADD_INPUT__(l->position(), "lightPosition");
      __ADD_INPUT__(l->spotDirection(), "lightDirection");
      __ADD_INPUT__(l->radius(), "lightRadius");
      __ADD_INPUT__(l->coneAngle(), "lightConeAngles");
      __ADD_INPUT__(l->diffuse(), "lightDiffuse");
      __ADD_INPUT__(l->specular(), "lightSpecular");
      __ADD_INPUT__(l->coneMatrix(), "modelMatrix");
      if(it->sm.get()) {
        SpotShadowMap *sm = (SpotShadowMap*) it->sm.get();
        __ADD_INPUT__(sm->near(), "shadowNear");
        __ADD_INPUT__(sm->far(), "shadowFar");
        __ADD_INPUT__(sm->shadowMatUniform(), "shadowMatrix");
      }
    }
    break;
  case POINT:
    for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
    {
      PointLight *l = (PointLight*) it->light.get();
      __ADD_INPUT__(l->position(), "lightPosition");
      __ADD_INPUT__(l->radius(), "lightRadius");
      __ADD_INPUT__(l->diffuse(), "lightDiffuse");
      __ADD_INPUT__(l->specular(), "lightSpecular");
      if(it->sm.get()) {
        PointShadowMap *sm = (PointShadowMap*) it->sm.get();
        __ADD_INPUT__(sm->near(), "shadowNear");
        __ADD_INPUT__(sm->far(), "shadowFar");
      }
    }
    break;
  }
#undef __ADD_INPUT__
  shadowMapLoc_ = shader_->shader()->uniformLocation("shadowTexture");
}

void LightPass::addInputLocation(LightPassLight &l,
    const ref_ptr<ShaderInput> &in, const string &name)
{
  Shader *s = shader_->shader().get();
  ShaderInputLocation x(in, s->uniformLocation(name));
  if(x.location>0) {
    l.inputLocations.push_back(x);
  }
  else {
    WARN_LOG("'" << name << "' is not an active uniform for shader '" << shaderKey_ << "'");
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
