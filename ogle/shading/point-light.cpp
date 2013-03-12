/*
 * point-light.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <ogle/states/atomic-states.h>
#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/box.h>
#include <ogle/shading/point-shadow-map.h>

#include "point-light.h"
using namespace ogle;

DeferredPointLight::DeferredPointLight()
: DeferredLight()
{
  mesh_ = ref_ptr<MeshState>::cast( Box::getUnitCube() );
  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));
  joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));
}

void DeferredPointLight::createShader(const ShaderState::Config &cfg) {
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  shader_->createShader(_cfg.cfg(), "shading.deferred.point");

  Shader *s = shader_->shader().get();
  posLoc_ = s->uniformLocation("lightPosition");
  radiusLoc_ = s->uniformLocation("lightRadius");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  specularLoc_ = s->uniformLocation("lightSpecular");
  shadowMapLoc_ = s->uniformLocation("shadowTexture");
  shadowFarLoc_ = s->uniformLocation("shadowFar");
  shadowNearLoc_ = s->uniformLocation("shadowNear");
}

void DeferredPointLight::enable(RenderState *rs)
{
  State::enable(rs);
  GLuint smChannel = rs->reserveTextureChannel();

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
      sm->far()->enableUniform(shadowFarLoc_);
      sm->near()->enableUniform(shadowNearLoc_);
    }

    mesh_->draw(1);
  }

  rs->releaseTextureChannel();
}
