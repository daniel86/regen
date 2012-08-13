/*
 * tesselation-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "tesselation-state.h"
#include <ogle/exceptions/gl-exceptions.h>
#include <ogle/utility/gl-error.h>

class SetPatchVertices : public Callable
{
  public: SetPatchVertices(Tesselation *cfg)
  : Callable(),
    cfg_(cfg)
  {
  }
  virtual void call()
  {
    handleGLError("before SetPatchVertices::call");
    glPatchParameteri(GL_PATCH_VERTICES, cfg_->numPatchVertices);
    handleGLError("after SetPatchVertices::call");
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
    handleGLError("before SetTessLevel::call");
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL,
        &cfg_->defaultOuterLevel.x);
    glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL,
        &cfg_->defaultInnerLevel.x);
    handleGLError("after SetTessLevel::call");
  }
  Tesselation *cfg_;
};

TesselationState::TesselationState(const Tesselation &cfg)
: State(),
  tessConfig_(TESS_PRIMITVE_TRIANGLES, 3)
{
  lodFactor_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("lodFactor", 1, 4.0f));
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

string TesselationState::name()
{
  return "TesselationState";
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
  handleGLError("before TesselationState::enable");
  State::enable(state);
  handleGLError("after TesselationState::enable");
}

void TesselationState::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setTesselationCfg(tessConfig_);
}
