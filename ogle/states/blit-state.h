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
      GLenum attachment);
  virtual void enable(RenderState *state);

  virtual string name();
protected:
  ref_ptr<FrameBufferObject> fbo_;
  GLenum attachment_;
};

#endif /* BLIT_TO_SCREEN_H_ */
