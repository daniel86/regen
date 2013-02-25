/*
 * directional-light.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include "directional-light.h"

#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/shading/directional-shadow-map.h>
#include <ogle/utility/string-util.h>

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
