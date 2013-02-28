/*
 * tesselation-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "tesselation-state.h"
#include <ogle/exceptions/gl-exceptions.h>
#include <ogle/utility/gl-util.h>
#include <ogle/utility/string-util.h>

SetPatchVertices::SetPatchVertices(GLuint numPatchVertices)
: State(), numPatchVertices_(numPatchVertices)
{}
void SetPatchVertices::enable(RenderState *state)
{
  state->patchVertices().push(numPatchVertices_);
}
void SetPatchVertices::disable(RenderState *state)
{
  state->patchVertices().pop();
}

SetTessLevel::SetTessLevel(
    const ref_ptr<ShaderInput4f> &innerLevel,
    const ref_ptr<ShaderInput4f> &outerLevel)
: State(), innerLevel_(innerLevel), outerLevel_(outerLevel)
{
}
void SetTessLevel::enable(RenderState *state)
{
  state->patchLevel().push(PatchLevels(
      innerLevel_->getVertex4f(0),
      outerLevel_->getVertex4f(0)));
}
void SetTessLevel::disable(RenderState *state)
{
  state->patchLevel().pop();
}

TesselationState::TesselationState(GLuint numPatchVertices)
: State(),
  numPatchVertices_(numPatchVertices)
{
  shaderDefine("TESS_NUM_VERTICES", FORMAT_STRING(numPatchVertices));
  shaderDefine("HAS_TESSELATION", "TRUE");
  setShaderVersion(400);

  innerLevel_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("tessInnerLevel"));
  innerLevel_->setUniformData(Vec4f(8.0f));
  joinShaderInput(ref_ptr<ShaderInput>::cast(innerLevel_));

  outerLevel_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("tessOuterLevel"));
  outerLevel_->setUniformData(Vec4f(8.0f));
  joinShaderInput(ref_ptr<ShaderInput>::cast(outerLevel_));

  lodFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lodFactor"));
  lodFactor_->setUniformData(4.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(lodFactor_));

  joinStates(ref_ptr<State>::manage(new SetPatchVertices(numPatchVertices_)));
}

void TesselationState::set_lodMetric(LoDMetric metric)
{
  if(lodMetric_ == metric) { return; }
  lodMetric_ = metric;

  if(tessLevelSetter_.get()) {
    disjoinStates(tessLevelSetter_);
    tessLevelSetter_ = ref_ptr<State>();
  }

  shaderDefine("TESS_IS_ADAPTIVE",
      lodMetric_==FIXED_FUNCTION ? "FALSE" : "TRUE");
  switch(lodMetric_) {
  case FIXED_FUNCTION:
    shaderDefine("TESS_LOD", "FIXED_FUNCTION");
    tessLevelSetter_ = ref_ptr<State>::manage(new SetTessLevel(innerLevel_,outerLevel_));
    joinStates(tessLevelSetter_);
    break;
  case EDGE_SCREEN_DISTANCE:
    shaderDefine("TESS_LOD", "EDGE_SCREEN_DISTANCE");
    break;
  case EDGE_DEVICE_DISTANCE:
    shaderDefine("TESS_LOD", "EDGE_DEVICE_DISTANCE");
    break;
  case CAMERA_DISTANCE_INVERSE:
    shaderDefine("TESS_LOD", "CAMERA_DISTANCE_INVERSE");
    break;
  }
}
TesselationState::LoDMetric TesselationState::lodMetric() const
{
  return lodMetric_;
}

const ref_ptr<ShaderInput4f>& TesselationState::outerLevel() const
{
  return outerLevel_;
}
const ref_ptr<ShaderInput4f>& TesselationState::innerLevel() const
{
  return innerLevel_;
}
const ref_ptr<ShaderInput1f>& TesselationState::lodFactor() const
{
  return lodFactor_;
}
