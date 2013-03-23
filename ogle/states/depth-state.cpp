/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/atomic-states.h>

#include "depth-state.h"
using namespace ogle;

void DepthState::set_useDepthWrite(GLboolean useDepthWrite)
{
  if(depthWriteToggle_.get()) {
    disjoinStates(depthWriteToggle_);
  }
  depthWriteToggle_ = ref_ptr<State>::manage(
      new ToggleDepthWriteState(useDepthWrite));
  joinStates(depthWriteToggle_);
}

void DepthState::set_useDepthTest(GLboolean useDepthTest)
{
  if(depthTestToggle_.get()) {
    disjoinStates(depthTestToggle_);
  }
  if(useDepthTest) {
    depthTestToggle_ = ref_ptr<State>::manage(
        new ToggleState(RenderState::DEPTH_TEST, GL_TRUE));
  } else {
    depthTestToggle_ = ref_ptr<State>::manage(
        new ToggleState(RenderState::DEPTH_TEST, GL_FALSE));
  }
  joinStates(depthTestToggle_);
}

void DepthState::set_depthFunc(GLenum depthFunc)
{
  if(depthFunc_.get()) {
    disjoinStates(depthFunc_);
  }
  depthFunc_ = ref_ptr<State>::manage(new DepthFuncState(depthFunc));
  joinStates(depthFunc_);
}

void DepthState::set_depthRange(GLdouble nearVal, GLdouble farVal)
{
  if(depthRange_.get()) {
    disjoinStates(depthRange_);
  }
  depthRange_ = ref_ptr<State>::manage(new DepthRangeState(nearVal,farVal));
  joinStates(depthRange_);
}
