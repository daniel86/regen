/*
 * tesselation-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "tesselation-state.h"
#include <ogle/exceptions/gl-exceptions.h>

class SetPatchVertices : public Callable
{
  public: SetPatchVertices(Tesselation *cfg)
  : Callable(),
    cfg_(cfg)
  {
  }
  virtual void call()
  {
    glPatchParameteri(GL_PATCH_VERTICES, cfg_->numPatchVertices);
  }
  Tesselation *cfg_;
};
class SetTessLevel : public Callable
{
  public: SetTessLevel(Tesselation *cfg)
  : Callable(),
    cfg_(cfg)
  {
  }
  virtual void call() {
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL,
        &cfg_->defaultOuterLevel.x);
    glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL,
        &cfg_->defaultInnerLevel.x);
  }
  Tesselation *cfg_;
};

TesselationState::TesselationState(const Tesselation &cfg)
: State(),
  tessConfig_(TESS_PRIMITVE_TRIANGLES, 3)
{
  if(!glewIsSupported("GL_ARB_tessellation_shader")) {
    throw ExtensionUnsupported("GL_ARB_tessellation_shader");
  }
  lodFactor_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("lodFactor", 4.0f));
  joinUniform(ref_ptr<Uniform>::cast(lodFactor_));

  ref_ptr<Callable> tessPatchVerticesSetter =
      ref_ptr<Callable>::manage(new SetPatchVertices(&tessConfig_));
  addEnabler( tessPatchVerticesSetter );
  if(!tessConfig_.isAdaptive) {
    ref_ptr<Callable> tessLevelSetter =
        ref_ptr<Callable>::manage(new SetTessLevel(&tessConfig_));
    addEnabler( tessLevelSetter );
  }
}

void TesselationState::set_tessConfig(const Tesselation &cfg)
{
  tessConfig_ = cfg;
}

void TesselationState::set_lodFactor(float factor)
{
  lodFactor_->set_value(factor);
}
float TesselationState::lodFactor() const
{
  return lodFactor_->value();
}

void TesselationState::enable(RenderState *state)
{
  glPatchParameteri(GL_PATCH_VERTICES,
      tessConfig_.numPatchVertices);
  glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL,
      &tessConfig_.defaultOuterLevel.x);
  glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL,
      &tessConfig_.defaultInnerLevel.x);
  State::enable(state);
}

void TesselationState::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setTesselationCfg(tessConfig_);
}
