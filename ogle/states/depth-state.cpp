/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "depth-state.h"

#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>
#include <ogle/states/toggle-state.h>

class DepthFuncState : public State
{
public:
  DepthFuncState(GLenum depthFunc)
  : State(), depthFunc_(depthFunc)
  {
  }
  virtual void enable(RenderState *state) {
    state->depthFunc().push(depthFunc_);
  }
  virtual void disable(RenderState *state) {
    state->depthFunc().pop();
  }
  GLenum depthFunc_;
};
class DepthRangeState : public State
{
public:
  DepthRangeState(GLdouble nearVal, GLdouble farVal)
  : State(), nearVal_(nearVal), farVal_(farVal)
  {
  }
  virtual void enable(RenderState *state) {
    state->depthRange().push(DepthRange(nearVal_, farVal_));
  }
  virtual void disable(RenderState *state) {
    state->depthRange().pop();
  }
  GLdouble nearVal_, farVal_;
};
class ToggleDepthWriteState : public State
{
public:
  ToggleDepthWriteState(GLboolean toggle) : State(), toggle_(toggle) { }
  virtual void enable(RenderState *state) {
    state->depthMask().push(toggle_);
  }
  virtual void disable(RenderState *state) {
    state->depthMask().pop();
  }
protected:
  GLboolean toggle_;
};

DepthState::DepthState()
: State()
{
}

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
  if(depthFunc!=GL_LESS) {
    depthFunc_ = ref_ptr<State>::manage(new DepthFuncState(depthFunc));
    joinStates(depthFunc_);
  } else {
    depthFunc_ = ref_ptr<State>();
  }
}

void DepthState::set_depthRange(GLdouble nearVal, GLdouble farVal)
{
  if(depthRange_.get()) {
    disjoinStates(depthRange_);
  }
  if(!isApprox(nearVal,0.0) || !isApprox(farVal,1.0)) {
    depthRange_ = ref_ptr<State>::manage(new DepthRangeState(nearVal,farVal));
    joinStates(depthRange_);
  } else {
    depthRange_ = ref_ptr<State>();
  }
}
