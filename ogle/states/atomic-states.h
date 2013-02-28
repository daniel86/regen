/*
 * atomic-states.h
 *
 *  Created on: 28.02.2013
 *      Author: daniel
 */

#ifndef ATOMIC_STATES_H_
#define ATOMIC_STATES_H_

#include <ogle/states/state.h>

/**
 * Toggles server side GL state.
 */
class ToggleState : public State
{
public:
  ToggleState(RenderState::Toggle key, GLboolean toggle)
  : State(), key_(key), toggle_(toggle){}

  RenderState::Toggle key() const
  { return key_; }
  GLboolean toggle() const
  { return toggle_; }

  void enable(RenderState *rs)
  { rs->toggles().push(key_, toggle_); }
  void disable(RenderState *rs)
  { rs->toggles().pop(key_); }

protected:
  RenderState::Toggle key_;
  GLboolean toggle_;
};

/**
 * Specifies the depth comparison function. Symbolic constants
 * GL_NEVER,GL_LESS,GL_EQUAL,GL_LEQUAL,GL_GREATER,
 * GL_NOTEQUAL,GL_GEQUAL,GL_ALWAYS are accepted.
 * The initial value is GL_LESS.
 */
class DepthFuncState : public State
{
public:
  DepthFuncState(GLenum depthFunc)
  : State(), depthFunc_(depthFunc) {}

  void enable(RenderState *rs)
  { rs->depthFunc().push(depthFunc_); }
  void disable(RenderState *rs)
  { rs->depthFunc().pop(); }

protected:
  GLenum depthFunc_;
};

/**
 * Specify mapping of depth values from normalized device coordinates
 * to window coordinates.
 * 'nearVal' specifies the mapping of the near clipping plane to window coordinates.
 * The initial value is 0.
 * 'farVal' specifies the mapping of the far clipping plane to window coordinates.
 * The initial value is 1.
 */
class DepthRangeState : public State
{
public:
  DepthRangeState(GLdouble nearVal, GLdouble farVal)
  : State(), nearVal_(nearVal), farVal_(farVal) {}

  void enable(RenderState *rs)
  { rs->depthRange().push(DepthRange(nearVal_, farVal_)); }
  void disable(RenderState *rs)
  { rs->depthRange().pop(); }

protected:
  GLdouble nearVal_, farVal_;
};

/**
 * Specifies whether the depth buffer is enabled for writing.
 * If flag is GL_FALSE, depth buffer writing is disabled.
 * Otherwise, it is enabled. Initially, depth buffer writing is enabled.
 */
class ToggleDepthWriteState : public State
{
public:
  ToggleDepthWriteState(GLboolean toggle)
  : State(), toggle_(toggle) { }

  void enable(RenderState *rs)
  { rs->depthMask().push(toggle_); }
  void disable(RenderState *rs)
  { rs->depthMask().pop(); }

protected:
  GLboolean toggle_;
};

/**
 * Set the blend color.
 * Initially the GL_BLEND_COLOR is set to (0,0,0,0).
 */
class BlendColorState : public State
{
public:
  BlendColorState(const Vec4f &col) : State(), col_(col) {}

  void enable(RenderState *state)
  { state->blendColor().push(col_); }
  void disable(RenderState *state)
  { state->blendColor().pop(); }

protected:
  Vec4f col_;
};

/**
 * Specify the equation used for both the RGB blend equation and the
 * Alpha blend equation.
 * 'buf' specifies the index of the draw buffer for which to set the blend equation.
 * 'mode' specifies how source and destination colors are combined.
 * It must be GL_FUNC_ADD,GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT,GL_MIN,GL_MAX.
 * Initially, both the RGB blend equation and the alpha blend equation
 * are set to GL_FUNC_ADD.
 */
class BlendEquationState : public State
{
public:
  BlendEquationState(GLenum equation)
  : State(), equation_(BlendEquation(equation,equation)) {}

  void enable(RenderState *state)
  { state->blendEquation().push(equation_); }
  void disable(RenderState *state)
  { state->blendEquation().pop(); }

protected:
  BlendEquation equation_;
};

/**
 * Specify pixel arithmetic.
 * 'buf' specifies the index of the draw buffer for which to set the blend function.
 * v.xz-'sfactor' specifies how the red, green, blue, and alpha source blending
 * factors are computed. The initial value is GL_ONE.
 * v.yw-'dfactor' specifies how the red, green, blue, and alpha destination
 * blending factors are computed.
 * The following symbolic constants are accepted:
 * GL_ZERO,GL_ONE,GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR,GL_DST_COLOR,
 * GL_ONE_MINUS_DST_COLOR,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
 * GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA.GL_CONSTANT_COLOR,
 * GL_ONE_MINUS_CONSTANT_COLOR,GL_CONSTANT_ALPHA,
 * GL_ONE_MINUS_CONSTANT_ALPHA. The initial value is GL_ZERO.
 */
class BlendFuncState : public State
{
public:
  BlendFuncState(
      GLenum srcRGB, GLenum dstRGB,
      GLenum srcAlpha, GLenum dstAlpha)
  : State(), func_(BlendFunction(srcRGB,dstRGB,srcAlpha,dstAlpha)) {}

  void enable(RenderState *state)
  { state->blendFunction().push(func_); }
  void disable(RenderState *state)
  { state->blendFunction().pop(); }

protected:
  BlendFunction func_;
};

/**
 * Specifies whether front- or back-facing facets are candidates for culling.
 * Symbolic constants GL_FRONT,GL_BACK, GL_FRONT_AND_BACK are accepted.
 * The initial value is GL_BACK.
 */
class CullFaceState : public State
{
public:
  CullFaceState(GLenum face) : State(), face_(face) {}

  void enable(RenderState *rs)
  { rs->cullFace().push(face_); }
  void disable(RenderState *rs)
  { rs->cullFace().pop(); }

protected:
  GLenum face_;
};

/**
 * Set the scale and units used to calculate depth values.
 * v.x-'factor' specifies a scale factor that is used to create a variable
 * depth offset for each polygon. The initial value is 0.
 * v.y-'units' is multiplied by an implementation-specific value to
 * create a constant depth offset. The initial value is 0.
 * This state also enables the polygon offset toggle.
 */
class PolygonOffsetState : public State
{
public:
  PolygonOffsetState(GLfloat factor, GLfloat units)
  : State(), factor_(factor), units_(units) {}

  void enable(RenderState *rs) {
    rs->toggles().push(RenderState::POLYGON_OFFSET_FILL, GL_TRUE);
    rs->polygonOffset().push(Vec2f(factor_, units_));
  }
  void disable(RenderState *rs) {
    rs->toggles().pop(RenderState::POLYGON_OFFSET_FILL);
    rs->polygonOffset().pop();
  }

protected:
  GLfloat factor_, units_;
};

/**
 * Specifies how polygons will be rasterized.
 * Accepted values are GL_POINT,GL_LINE,GL_FILL.
 * The initial value is GL_FILL for both front- and back-facing polygons.
 */
class FillModeState : public State
{
public:
  FillModeState(GLenum mode) : State(), mode_(mode) {}

  void enable(RenderState *rs)
  { rs->polygonMode().push(mode_); }
  void disable(RenderState *rs)
  { rs->polygonMode().pop(); }

protected:
  GLenum mode_;
};

/**
 * Specifies the number of vertices that
 * will be used to make up a single patch primitive.
 */
class SetPatchVertices : public State
{
public:
  SetPatchVertices(GLuint numPatchVertices)
  : State(), numPatchVertices_(numPatchVertices) {}

  void enable(RenderState *rs)
  { rs->patchVertices().push(numPatchVertices_); }
  void disable(RenderState *rs)
  { rs->patchVertices().pop(); }

protected:
  GLuint numPatchVertices_;
};

/**
 * Specifies the default outer or inner tessellation levels
 * to be used when no tessellation control shader is present.
 */
class SetTessLevel : public State
{
public:
  SetTessLevel(const ref_ptr<ShaderInput4f> &inner, const ref_ptr<ShaderInput4f> &outer)
  : State(), inner_(inner), outer_(outer) {}

  void enable(RenderState *rs)
  { rs->patchLevel().push(PatchLevels(inner(), outer())); }
  void disable(RenderState *rs)
  { rs->patchLevel().pop(); }

  const Vec4f& inner() const
  { return inner_->getVertex4f(0); }
  const Vec4f& outer() const
  { return outer_->getVertex4f(0); }

protected:
  ref_ptr<ShaderInput4f> inner_;
  ref_ptr<ShaderInput4f> outer_;
};

#endif /* ATOMIC_STATES_H_ */
