/*
 * blend-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __BLEND_STATE_H_
#define __BLEND_STATE_H_

#include <ogle/states/state.h>

/**
 * In RGBA mode, pixels can be drawn using a function that blends
 * the incoming (source) RGBA values with the RGBA values
 * that are already in the frame buffer (the destination values).
 */
class BlendState : public State
{
public:
  /**
   * sfactor specifies which method is used to scale the source color components.
   * dfactor specifies which method is used to scale the destination color components.
   */
  BlendState(
    GLenum sfactor=GL_SRC_ALPHA,
    GLenum dfactor=GL_ONE_MINUS_SRC_ALPHA);

  /**
   * specify pixel arithmetic.
   * sfactor specifies which method is used to scale the source color components.
   * dfactor specifies which method is used to scale the destination color components.
   */
  void setBlendFunc(
      GLenum sfactor=GL_SRC_ALPHA,
      GLenum dfactor=GL_ONE_MINUS_SRC_ALPHA);

  /**
   * specify pixel arithmetic for RGB and alpha components separately.
   */
  void setBlendFuncSeparate(
      GLenum srcRGB=GL_ONE,
      GLenum dstRGB=GL_ZERO,
      GLenum srcAlpha=GL_ONE,
      GLenum dstAlpha=GL_ZERO);

  /**
   * The GL_BLEND_COLOR may be used to calculate the source and destination
   * blending factors. The color components are clamped to the range [0,1]
   * before being stored. See glBlendFunc for a complete description of the
   * blending operations. Initially the GL_BLEND_COLOR is set to (0, 0, 0, 0).
   */
  void setBlendColor(const Vec4f &col=Vec4f(0.0f));

  /**
   * specifies how source and destination colors are combined.
   * It must be GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX.
   * Initially, both the RGB blend equation and the alpha blend equation are set to GL_FUNC_ADD.
   */
  void setBlendEquation(GLenum equation=GL_FUNC_ADD);

  // override
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
  virtual string name();
protected:
  ref_ptr<State> blendFunc_;
  ref_ptr<State> blendColor_;
  ref_ptr<State> blendEquation_;
};

#endif /* __BLEND_STATE_H_ */
