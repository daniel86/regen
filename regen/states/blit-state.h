/*
 * blit-to-screen.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef BLIT_TO_SCREEN_H_
#define BLIT_TO_SCREEN_H_

#include <regen/states/state.h>
#include <regen/gl-types/fbo.h>

namespace ogle {
/**
 * \brief Blits a FrameBufferObject color attachment to screen.
 */
class BlitToScreen : public State
{
public:
  /**
   * @param fbo FBO to blit.
   * @param viewport the screen viewport.
   * @param attachment color attachment to blit.
   */
  BlitToScreen(
      const ref_ptr<FrameBufferObject> &fbo,
      const ref_ptr<ShaderInput2i> &viewport,
      GLenum attachment=GL_COLOR_ATTACHMENT0);

  /**
   * filterMode must be GL_NEAREST or GL_LINEAR.
   */
  void set_filterMode(GLenum filterMode=GL_LINEAR);
  /**
   * GL_FRONT or GL_BACK.
   * If the window does not use double buffering you want to
   * use GL_FRONT.
   */
  void set_screenBuffer(GLenum screenBuffer=GL_FRONT);
  /**
   * The bitwise OR of the flags indicating which buffers are to be copied.
   * The allowed flags are  GL_COLOR_BUFFER_BIT,
   * GL_DEPTH_BUFFER_BIT and GL_STENCIL_BUFFER_BIT.
   */
  void set_sourceBuffer(GLenum sourceBuffer=GL_COLOR_BUFFER_BIT);

  // override
  void enable(RenderState *state);
protected:
  ref_ptr<FrameBufferObject> fbo_;
  ref_ptr<ShaderInput2i> viewport_;
  GLenum attachment_;
  GLenum filterMode_;
  GLenum screenBuffer_;
  GLenum sourceBuffer_;
};

/**
 * \brief Blits a FrameBufferObject color attachment to screen.
 *
 * This is useful for ping-pong textures consisting of 2 images.
 */
class BlitTexToScreen : public BlitToScreen
{
public:
  /**
   * @param fbo a FBO.
   * @param texture a texture.
   * @param viewport the screen viewport.
   * @param attachment the first texture attachment.
   */
  BlitTexToScreen(
      const ref_ptr<FrameBufferObject> &fbo,
      const ref_ptr<Texture> &texture,
      const ref_ptr<ShaderInput2i> &viewport,
      GLenum attachment=GL_COLOR_ATTACHMENT0);

  // override
  virtual void enable(RenderState *state);
protected:
  ref_ptr<Texture> texture_;
  GLenum baseAttachment_;
};

} // end ogle namespace

#endif /* BLIT_TO_SCREEN_H_ */
