/*
 * ambient-light.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <ogle/meshes/rectangle.h>
#include <ogle/states/shader-configurer.h>

#include "ambient-light.h"
using namespace ogle;

DeferredAmbientLight::DeferredAmbientLight() : State()
{
  ambientLight_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("lightAmbient"));
  ambientLight_->setUniformData(Vec3f(0.1f));
  joinShaderInput(ref_ptr<ShaderInput>::cast(ambientLight_));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

void DeferredAmbientLight::createShader(ShaderState::Config &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  shader_->createShader(_cfg.cfg(), "shading.deferred.ambient");
}

const ref_ptr<ShaderInput3f>& DeferredAmbientLight::ambientLight() const
{
  return ambientLight_;
}
