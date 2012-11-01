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
    glDepthFunc(depthFunc_);
  }
  virtual void disable(RenderState *state) {
    glDepthFunc(GL_LESS);
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
    glDepthRange(nearVal_, farVal_);
  }
  virtual void disable(RenderState *state) {
    glDepthRange(0.0, 1.0);
  }
  GLdouble nearVal_, farVal_;
};
class PolygonOffsetState : public State
{
public:
  PolygonOffsetState(GLfloat factor, GLfloat units)
  : State(), factor_(factor), units_(units)
  {
  }
  virtual void enable(RenderState *state) {
    glPolygonOffset(factor_, units_);
  }
  virtual void disable(RenderState *state) {
    glPolygonOffset(0.0f, 0.0f);
  }
  GLfloat factor_, units_;
};
class EnableDepthTestState : public State
{
public:
  EnableDepthTestState() : State() { }
  virtual void enable(RenderState *state) {
    glEnable(GL_DEPTH_TEST);
  }
  virtual void disable(RenderState *state) {
    glDisable(GL_DEPTH_TEST);
  }
};
class DisableDepthTestState : public State
{
public:
  DisableDepthTestState() : State() { }
  virtual void enable(RenderState *state) {
    glDisable(GL_DEPTH_TEST);
  }
  virtual void disable(RenderState *state) {
    glEnable(GL_DEPTH_TEST);
  }
};
class ToggleDepthWriteState : public State
{
public:
  ToggleDepthWriteState(GLboolean toggle) : State(), toggle_(toggle) { }
  virtual void enable(RenderState *state) {
    glDepthMask(toggle_);
  }
  virtual void disable(RenderState *state) {
    glDepthMask(!toggle_);
  }
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

void DepthState::set_polygonOffset(GLfloat factor, GLfloat units)
{
  if(polygonOffset_.get()) {
    disjoinStates(polygonOffset_);
  }
  if(!isApprox(factor,0.0) || !isApprox(units,1.0)) {
    polygonOffset_ = ref_ptr<State>::manage(new PolygonOffsetState(factor,units));
    joinStates(polygonOffset_);
  } else {
    polygonOffset_ = ref_ptr<State>();
  }
}
