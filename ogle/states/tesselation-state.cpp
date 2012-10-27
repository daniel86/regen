/*
 * tesselation-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "tesselation-state.h"
#include <ogle/exceptions/gl-exceptions.h>
#include <ogle/utility/gl-error.h>
#include <ogle/states/render-state.h>

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
    // the default outer or inner tessellation levels, respectively,
    // to be used when no tessellation control shader is present.
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL,
        &cfg_->defaultOuterLevel.x);
    glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL,
        &cfg_->defaultInnerLevel.x);
  }
  Tesselation *cfg_;
};

TesselationState::TesselationState(const Tesselation &cfg)
: State(),
  tessConfig_(cfg)
{
  lodFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lodFactor"));
  lodFactor_->setUniformData(4.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(lodFactor_));

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

void TesselationState::set_lodFactor(GLfloat factor)
{
  lodFactor_->setUniformData(factor);
}
ref_ptr<ShaderInput1f>& TesselationState::lodFactor()
{
  return lodFactor_;
}

void TesselationState::configureShader(ShaderConfig *cfg)
{
  State::configureShader(cfg);
  cfg->setTesselationCfg(tessConfig_);
}
