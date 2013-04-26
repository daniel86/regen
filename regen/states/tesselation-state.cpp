/*
 * tesselation-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <regen/gl-types/gl-util.h>
#include <regen/utility/string-util.h>
#include <regen/states/atomic-states.h>

#include "tesselation-state.h"
using namespace regen;

TesselationState::TesselationState(GLuint numPatchVertices)
: State(),
  numPatchVertices_(numPatchVertices)
{
#ifdef GL_VERSION_4_0
  shaderDefine("TESS_NUM_VERTICES", FORMAT_STRING(numPatchVertices));
  shaderDefine("HAS_TESSELATION", "TRUE");
  setShaderVersion(400);
#else
  WARN_LOG("GL_ARB_tessellation_shader not supported.");
#endif

  innerLevel_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("tessInnerLevel"));
  innerLevel_->setUniformData(Vec4f(8.0f));
  joinShaderInput(innerLevel_);

  outerLevel_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("tessOuterLevel"));
  outerLevel_->setUniformData(Vec4f(8.0f));
  joinShaderInput(outerLevel_);

  lodFactor_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lodFactor"));
  lodFactor_->setUniformData(4.0f);
  joinShaderInput(lodFactor_);

  joinStates(ref_ptr<State>::manage(new PatchVerticesState(numPatchVertices_)));
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
    tessLevelSetter_ = ref_ptr<State>::manage(new PatchLevelState(innerLevel_,outerLevel_));
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
{ return lodMetric_; }

const ref_ptr<ShaderInput4f>& TesselationState::outerLevel() const
{ return outerLevel_; }
const ref_ptr<ShaderInput4f>& TesselationState::innerLevel() const
{ return innerLevel_; }
const ref_ptr<ShaderInput1f>& TesselationState::lodFactor() const
{ return lodFactor_; }
