/*
 * tesselation-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "tesselation-state.h"
#include <ogle/exceptions/gl-exceptions.h>
#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
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

// TODO: re-think tess
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

  shaderDefine("HAS_TESSELATION", "TRUE");
  shaderDefine("TESS_IS_ADAPTIVE", cfg.isAdaptive ? "TRUE" : "FALSE");
  shaderDefine("GLSL_VERSION", "400");
  shaderDefine("TESS_NUM_VERTICES", FORMAT_STRING(cfg.numPatchVertices));
  switch(cfg.lodMetric) {
  case TESS_LOD_EDGE_SCREEN_DISTANCE:
    shaderDefine("TESS_LOD", "EDGE_SCREEN_DISTANCE");
    break;
  case TESS_LOD_EDGE_DEVICE_DISTANCE:
    shaderDefine("TESS_LOD", "EDGE_DEVICE_DISTANCE");
    break;
  default:
    shaderDefine("TESS_LOD", "CAMERA_DISTANCE_INVERSE");
    break;
  }
  switch(cfg.spacing) {
  case TESS_SPACING_EQUAL:
    shaderDefine("TESS_SPACING", "equal_spacing");
    break;
  case TESS_SPACING_FRACTIONAL_EVEN:
    shaderDefine("TESS_SPACING", "fractional_even_spacing");
    break;
  case TESS_SPACING_FRACTIONAL_ODD:
    shaderDefine("TESS_SPACING", "fractional_odd_spacing");
    break;
  }
  switch(cfg.ordering) {
  case TESS_ORDERING_CCW:
    shaderDefine("TESS_ORDERING", "ccw");
    break;
  case TESS_ORDERING_CW:
    shaderDefine("TESS_ORDERING", "cw");
    break;
  case TESS_ORDERING_POINT_MODE:
    shaderDefine("TESS_ORDERING", "point_mode");
    break;
  }
  switch(cfg.primitive) {
  case TESS_PRIMITVE_TRIANGLES:
    shaderDefine("TESS_PRIMITVE", "triangles");
    break;
  case TESS_PRIMITVE_QUADS:
    shaderDefine("TESS_PRIMITVE", "quads");
    break;
  case TESS_PRIMITVE_ISOLINES:
    shaderDefine("TESS_PRIMITVE", "isolines");
    break;
  }
}

void TesselationState::set_lodFactor(GLfloat factor)
{
  lodFactor_->setUniformData(factor);
}
const ref_ptr<ShaderInput1f>& TesselationState::lodFactor() const
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
