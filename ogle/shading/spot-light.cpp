/*
 * spot-light.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <ogle/states/atomic-states.h>
#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/cone.h>
#include <ogle/shading/spot-shadow-map.h>

#include "spot-light.h"
using namespace ogle;

DeferredSpotLight::DeferredSpotLight()
: DeferredLight()
{
  mesh_ = ref_ptr<MeshState>::cast( ConeClosed::getBaseCone() );
  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
  joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));
}

void DeferredSpotLight::createShader(const ShaderConfig &cfg)
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
  GLuint smChannel = rs->reserveTextureChannel();

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

  rs->releaseTextureChannel();
}
