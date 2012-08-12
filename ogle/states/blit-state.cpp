/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blit-state.h"

BlitToScreen::BlitToScreen(
    ref_ptr<FrameBufferObject> &fbo,
    GLenum attachment)
: State(),
  fbo_(fbo),
  attachment_(attachment)
{
}

string BlitToScreen::name()
{
  return "BlitToScreen";
}

void BlitToScreen::enable(RenderState *state)
{
  FrameBufferObject::bindDefault();
  glDrawBuffer(GL_FRONT);
  FrameBufferObject::blitCopyToScreen(
      *fbo_.get(),
      fbo_->width(), fbo_->height(),
      attachment_,
      GL_COLOR_BUFFER_BIT,
      GL_NEAREST,
      GL_FRONT);
  State::enable(state);
}
