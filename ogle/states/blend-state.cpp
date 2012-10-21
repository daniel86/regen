/*
 * blend-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blend-state.h"

class BlendColorState : public State
{
public:
  BlendColorState(const Vec4f &col)
  : State(),
    col_(col)
  {
  }
  virtual void enable(RenderState *state)
  {
    glBlendColor(col_.x, col_.y, col_.z, col_.w);
  }
  virtual void disable(RenderState *state)
  {
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);
  }
  Vec4f col_;
};
class BlendEquationState : public State
{
public:
  BlendEquationState(GLenum equation)
  : State(),
    equation_(equation)
  {
  }
  virtual void enable(RenderState *state)
  {
    glBlendEquation(equation_);
  }
  virtual void disable(RenderState *state)
  {
    glBlendEquation(GL_FUNC_ADD);
  }
  GLenum equation_;
};
class BlendFuncSeparateState : public State
{
public:
  BlendFuncSeparateState(
      GLenum srcRGB,
      GLenum dstRGB,
      GLenum srcAlpha,
      GLenum dstAlpha)
  : State(),
    srcRGB_(srcRGB),
    dstRGB_(dstRGB),
    srcAlpha_(srcAlpha),
    dstAlpha_(dstAlpha)
  {
  }
  virtual void enable(RenderState *state)
  {
    glBlendFuncSeparate(srcRGB_, dstRGB_, srcAlpha_, dstAlpha_);
  }
  GLenum srcRGB_;
  GLenum dstRGB_;
  GLenum srcAlpha_;
  GLenum dstAlpha_;
};
class BlendFuncState : public State
{
public:
  BlendFuncState(
      GLenum sfactor,
      GLenum dfactor)
  : State(),
    sfactor_(sfactor),
    dfactor_(dfactor)
  {
  }
  virtual void enable(RenderState *state)
  {
    glBlendFunc(sfactor_, dfactor_);
  }
  GLenum sfactor_;
  GLenum dfactor_;
};

BlendState::BlendState(TextureBlendMode blendMode)
: State()
{
  switch(blendMode) {
  case BLEND_MODE_ALPHA:
    // c = c0*(1-c1.a) + c1*c1.a
    setBlendEquation(GL_FUNC_ADD);
    setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  case BLEND_MODE_MULTIPLY:
    // c = c0*c1, a=a0*a1
    setBlendEquation(GL_FUNC_ADD);
    setBlendFunc(GL_DST_COLOR, GL_ZERO);
    break;
  case BLEND_MODE_ADD:
    // c = c0+c1, a=a0+a1
    setBlendEquation(GL_FUNC_ADD);
    setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_SMOOTH_ADD: // aka average
    // c = 0.5*c0 + 0.5*c1
    // a = a0 + a1
    setBlendEquation(GL_FUNC_ADD);
    setBlendFuncSeparate(
        GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA,
        GL_ONE, GL_ONE);
    setBlendColor(Vec4f(0.5f));
    break;
  case BLEND_MODE_SUBSTRACT:
    // c = c0-c1, a=a0-a1
    setBlendEquation(GL_FUNC_SUBTRACT);
    setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_REVERSE_SUBSTRACT:
    // c = c1-c0, a=c1-c0
    setBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_LIGHTEN:
    // c = max(c0,c1), a = max(a0,a1)
    setBlendEquation(GL_MAX);
    setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_DARKEN:
    // c = min(c0,c1), a = min(a0,a1)
    setBlendEquation(GL_MIN);
    setBlendFunc(GL_ONE, GL_ONE);
    break;
  case BLEND_MODE_SCREEN:
    // c = c0 - c1*(1-c0), a = a0 - a1*(1-a0)
    setBlendEquation(GL_FUNC_SUBTRACT);
    setBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
    break;
  case BLEND_MODE_SRC_ALPHA:
    // c = c1*c1.a
    setBlendFunc(GL_SRC_ALPHA, GL_ZERO);
    break;
  case BLEND_MODE_SRC:
  default:
    break;
  }
}
BlendState::BlendState(GLenum sfactor, GLenum dfactor)
: State()
{
  setBlendFunc(sfactor,dfactor);
}

string BlendState::name()
{
  return "BlendState";
}

void BlendState::setBlendFunc(GLenum sfactor, GLenum dfactor)
{
  if(blendFunc_.get()) {
    disjoinStates(blendFunc_);
  }
  if(sfactor!=GL_ONE || dfactor!=GL_ZERO) {
    blendFunc_ = ref_ptr<State>::manage(new BlendFuncState(sfactor,dfactor));
    joinStates(blendFunc_);
  } else {
    blendFunc_ = ref_ptr<State>();
  }
}

void BlendState::setBlendFuncSeparate(
    GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
  if(blendFunc_.get()) {
    disjoinStates(blendFunc_);
  }
  if(srcRGB!=GL_ONE || dstRGB!=GL_ZERO || srcAlpha!=GL_ONE || dstAlpha!=GL_ZERO) {
    blendFunc_ = ref_ptr<State>::manage(
        new BlendFuncSeparateState(srcRGB, dstRGB, srcAlpha, dstAlpha));
    joinStates(blendFunc_);
  } else {
    blendFunc_ = ref_ptr<State>();
  }
}

void BlendState::setBlendEquation(GLenum equation)
{
  if(blendEquation_.get()) {
    disjoinStates(blendEquation_);
  }
  if(equation==GL_FUNC_ADD) {
    blendEquation_ = ref_ptr<State>();
  } else {
    blendEquation_ = ref_ptr<State>::manage(new BlendEquationState(equation));
    joinStates(blendEquation_);
  }
}

void BlendState::setBlendColor(const Vec4f &col)
{
  if(blendColor_.get()) {
    disjoinStates(blendColor_);
  }
  if(isApprox(col, Vec4f(0.0f))) {
    blendColor_ = ref_ptr<State>();
  } else {
    blendColor_ = ref_ptr<State>::manage(new BlendColorState(col));
    joinStates(blendColor_);
  }
}

void BlendState::enable(RenderState *state)
{
  glEnable(GL_BLEND);
  State::enable(state);
}

void BlendState::disable(RenderState *state)
{
  State::disable(state);
  glDisable (GL_BLEND);
}
