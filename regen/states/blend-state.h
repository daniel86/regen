/*
 * blend-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __BLEND_STATE_H_
#define __BLEND_STATE_H_

#include <regen/states/atomic-states.h>

namespace ogle {
/**
 * \brief Describes how a texture will be mixed with existing pixels.
 */
enum BlendMode {
  BLEND_MODE_SRC,              //!< c = c1
  BLEND_MODE_FRONT_TO_BACK,    //!< c = c1.a*c1
  BLEND_MODE_SRC_ALPHA,        //!< c = c1.a*c1
  BLEND_MODE_BACK_TO_FRONT,    //!< c = c0*(1.0-c0.a) + c1*c0.a
  BLEND_MODE_DST_ALPHA,        //!< c = c0*(1.0-c0.a) + c1*c0.a
  BLEND_MODE_ALPHA,            //!< c = c0*c1.a + c1*(1.0-c1.a)
  BLEND_MODE_MULTIPLY,         //!< c = c0*c1
  BLEND_MODE_ADD,              //!< c = c0+c1
  BLEND_MODE_SMOOTH_ADD,       //!< c = 0.5*c0+0.5*c1
  BLEND_MODE_SUBSTRACT,        //!< c = c0-c1
  BLEND_MODE_REVERSE_SUBSTRACT,//!< c = c1-c0
  BLEND_MODE_DIFFERENCE,       //!< c = abs(c0-c1)
  BLEND_MODE_LIGHTEN,          //!< c = max(c0,c1)
  BLEND_MODE_DARKEN,           //!< c = min(c0,c1)
  BLEND_MODE_DIVIDE,           //!<
  BLEND_MODE_MIX,              //!<
  BLEND_MODE_SCREEN,           //!<
  BLEND_MODE_OVERLAY,          //!<
  BLEND_MODE_HUE,              //!<
  BLEND_MODE_SATURATION,       //!<
  BLEND_MODE_VALUE,            //!<
  BLEND_MODE_COLOR,            //!<
  BLEND_MODE_DODGE,            //!<
  BLEND_MODE_BURN,             //!<
  BLEND_MODE_SOFT,             //!<
  BLEND_MODE_LINEAR            //!<
};

/**
 * \brief Describes how a texture will be mixed with existing pixels.
 *
 * In RGBA mode, pixels can be drawn using a function that blends
 * the incoming (source) RGBA values with the RGBA values
 * that are already in the frame buffer (the destination values).
 */
class BlendState : public ServerSideState
{
public:
  /**
   * @param sfactor specifies which method is used to scale the source color components.
   * @param dfactor specifies which method is used to scale the destination color components.
   */
  BlendState(
    GLenum sfactor=GL_SRC_ALPHA,
    GLenum dfactor=GL_ONE_MINUS_SRC_ALPHA);
  /**
   * @param blendMode blend mode enum.
   */
  BlendState(BlendMode blendMode);

  /**
   * Specify pixel arithmetic.
   * @param sfactor specifies which method is used to scale the source color components.
   * @param dfactor specifies which method is used to scale the destination color components.
   */
  void setBlendFunc(
      GLenum sfactor=GL_SRC_ALPHA,
      GLenum dfactor=GL_ONE_MINUS_SRC_ALPHA);

  /**
   * Specify pixel arithmetic for RGB and alpha components separately.
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
   * Specifies how source and destination colors are combined.
   * It must be GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX.
   * Initially, both the RGB blend equation and the alpha blend equation are set to GL_FUNC_ADD.
   */
  void setBlendEquation(GLenum equation=GL_FUNC_ADD);

protected:
  ref_ptr<State> blendFunc_;
  ref_ptr<State> blendColor_;
  ref_ptr<State> blendEquation_;
};

ostream& operator<<(ostream &out, const BlendMode &v);
istream& operator>>(istream &in, BlendMode &v);

} // end ogle namespace

#endif /* __BLEND_STATE_H_ */
