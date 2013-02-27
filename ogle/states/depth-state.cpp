/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "depth-state.h"

#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>

class DepthFuncState : public State
{
public:
  DepthFuncState(GLenum depthFunc)
  : State(), depthFunc_(depthFunc)
  {
  }
  virtual void enable(RenderState *state) {
    state->pushDepthFunc(depthFunc_);
  }
  virtual void disable(RenderState *state) {
    state->popDepthFunc();
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
    state->pushDepthRange(DepthRange(nearVal_, farVal_));
  }
  virtual void disable(RenderState *state) {
    state->popDepthRange();
  }
  GLdouble nearVal_, farVal_;
};
class EnableDepthTestState : public State
{
public:
  EnableDepthTestState() : State() { }
  virtual void enable(RenderState *state) {
    state->pushToggle(RenderState::DEPTH_TEST, GL_TRUE);
  }
  virtual void disable(RenderState *state) {
    state->popToggle(RenderState::DEPTH_TEST);
  }
};
class DisableDepthTestState : public State
{
public:
  DisableDepthTestState() : State() { }
  virtual void enable(RenderState *state) {
    state->pushToggle(RenderState::DEPTH_TEST, GL_FALSE);
  }
  virtual void disable(RenderState *state) {
    state->popToggle(RenderState::DEPTH_TEST);
  }
};
class ToggleDepthWriteState : public State
{
public:
  ToggleDepthWriteState(GLboolean toggle) : State(), toggle_(toggle) { }
  virtual void enable(RenderState *state) {
    state->pushDepthMask(toggle_);
  }
  virtual void disable(RenderState *state) {
    state->popDepthMask();
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
    depthTestToggle_ = ref_ptr<State>::manage(new EnableDepthTestState);
  } else {
    depthTestToggle_ = ref_ptr<State>::manage(new DisableDepthTestState);
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
