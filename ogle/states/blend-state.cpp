/*
 * blend-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blend-state.h"

ostream& operator<<(ostream &out, const BlendMode &mode)
{
  switch(mode) {
  case BLEND_MODE_SRC_ALPHA:            return out << "srcAlpha";
  case BLEND_MODE_ALPHA:                return out << "alpha";
  case BLEND_MODE_MULTIPLY:             return out << "mul";
  case BLEND_MODE_DIVIDE:               return out << "div";
  case BLEND_MODE_DIFFERENCE:           return out << "diff";
  case BLEND_MODE_SMOOTH_ADD:           return out << "smoothAdd";
  case BLEND_MODE_ADD:                  return out << "add";
  case BLEND_MODE_SUBSTRACT:            return out << "sub";
  case BLEND_MODE_REVERSE_SUBSTRACT:    return out << "reverseSub";
  case BLEND_MODE_LIGHTEN:              return out << "lighten";
  case BLEND_MODE_DARKEN:               return out << "darken";
  case BLEND_MODE_SCREEN:               return out << "screen";
  case BLEND_MODE_OVERLAY:              return out << "overlay";
  case BLEND_MODE_DODGE:                return out << "dodge";
  case BLEND_MODE_BURN:                 return out << "burn";
  case BLEND_MODE_SOFT:                 return out << "soft";
  case BLEND_MODE_LINEAR:               return out << "linear";
  case BLEND_MODE_HUE:                  return out << "hue";
  case BLEND_MODE_SATURATION:           return out << "sat";
  case BLEND_MODE_VALUE:                return out << "val";
  case BLEND_MODE_COLOR:                return out << "col";
  case BLEND_MODE_MIX:                  return out << "mix";
  case BLEND_MODE_SRC:
  default:                              return out << "src";
  }
}
istream& operator>>(istream &in, BlendMode &mode)
{
  string val;
  in >> val;
  if(val == "src")              mode = BLEND_MODE_SRC;
  else if(val == "srcAlpha")    mode = BLEND_MODE_SRC_ALPHA;
  else if(val == "alpha")       mode = BLEND_MODE_ALPHA;
  else if(val == "mul")         mode = BLEND_MODE_MULTIPLY;
  else if(val == "div")         mode = BLEND_MODE_DIVIDE;
  else if(val == "diff")        mode = BLEND_MODE_DIFFERENCE;
  else if(val == "smoothAdd")   mode = BLEND_MODE_SMOOTH_ADD;
  else if(val == "average")     mode = BLEND_MODE_SMOOTH_ADD;
  else if(val == "add")         mode = BLEND_MODE_ADD;
  else if(val == "sub")         mode = BLEND_MODE_SUBSTRACT;
  else if(val == "reverseSub")  mode = BLEND_MODE_REVERSE_SUBSTRACT;
  else if(val == "lighten")     mode = BLEND_MODE_LIGHTEN;
  else if(val == "darken")      mode = BLEND_MODE_DARKEN;
  else if(val == "screen")      mode = BLEND_MODE_SCREEN;
  else if(val == "overlay")     mode = BLEND_MODE_OVERLAY;
  else if(val == "dodge")       mode = BLEND_MODE_DODGE;
  else if(val == "burn")        mode = BLEND_MODE_BURN;
  else if(val == "soft")        mode = BLEND_MODE_SOFT;
  else if(val == "linear")      mode = BLEND_MODE_LINEAR;
  else if(val == "hue")         mode = BLEND_MODE_HUE;
  else if(val == "sat")         mode = BLEND_MODE_SATURATION;
  else if(val == "val")         mode = BLEND_MODE_VALUE;
  else if(val == "col")         mode = BLEND_MODE_COLOR;
  else if(val == "mix")         mode = BLEND_MODE_MIX;
  else                          mode = BLEND_MODE_SRC;
  return in;
}

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

BlendState::BlendState(BlendMode blendMode)
: State()
{
  switch(blendMode) {
  case BLEND_MODE_ALPHA:
  case BLEND_MODE_FRONT_TO_BACK:
    // c = c0*(1-c1.a) + c1*c1.a
    setBlendEquation(GL_FUNC_ADD);
    setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    break;
  case BLEND_MODE_DST_ALPHA:
  case BLEND_MODE_BACK_TO_FRONT:
    // c = c0*(1-c1.a) + c1*c1.a
    setBlendEquation(GL_FUNC_ADD);
    setBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
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
