/*
 * blit-to-screen.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef BLIT_TO_SCREEN_H_
#define BLIT_TO_SCREEN_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/fbo.h>

/**
 * Blits a FBO color attachment on screen.
 */
class BlitToScreen : public State
{
public:
  BlitToScreen(
      ref_ptr<FrameBufferObject> &fbo,
      const Vec2ui &windowSize,
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
  virtual void enable(RenderState *state);
  virtual string name();
protected:
  ref_ptr<FrameBufferObject> fbo_;
  GLenum attachment_;
  GLenum filterMode_;
  GLenum screenBuffer_;
  GLenum sourceBuffer_;
  const Vec2ui &windowSize_;
};

#endif /* BLIT_TO_SCREEN_H_ */
