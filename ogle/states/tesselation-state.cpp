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

class SetPatchVertices : public State
{
public:
  SetPatchVertices(Tesselation *cfg) : State(), cfg_(cfg) { }
  virtual void enable(RenderState *state) {
    glPatchParameteri(GL_PATCH_VERTICES, cfg_->numPatchVertices);
  }
  Tesselation *cfg_;
};
class SetTessLevel : public State
{
public:
  SetTessLevel(Tesselation *cfg) : State(), cfg_(cfg) { }
  virtual void enable(RenderState *state) {
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

  ref_ptr<State> tessPatchVerticesSetter =
      ref_ptr<State>::manage(new SetPatchVertices(&tessConfig_));
  joinStates( tessPatchVerticesSetter );
  if(!tessConfig_.isAdaptive) {
    ref_ptr<State> tessLevelSetter =
        ref_ptr<State>::manage(new SetTessLevel(&tessConfig_));
    joinStates( tessLevelSetter );
  }
}

void TesselationState::set_lodFactor(GLfloat factor)
{
  lodFactor_->setUniformData(factor);
}
const ref_ptr<ShaderInput1f>& TesselationState::lodFactor()
{
  return lodFactor_;
}

void TesselationState::enable(RenderState *rs)
{
  usedTess_ = rs->useTesselation();
  rs->set_useTesselation(GL_TRUE);
  State::enable(rs);
}
void TesselationState::disable(RenderState *rs)
{
  rs->set_useTesselation(usedTess_);
  State::disable(rs);
}

void TesselationState::configureShader(ShaderConfig *cfg)
{
  State::configureShader(cfg);
  cfg->setTesselationCfg(tessConfig_);
}
