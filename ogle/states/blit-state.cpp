/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blit-state.h"

BlitToScreen::BlitToScreen(
    ref_ptr<FrameBufferObject> &fbo,
    const Vec2ui &windowSize,
    GLenum attachment)
: State(),
  fbo_(fbo),
  attachment_(attachment),
  windowSize_(windowSize),
  filterMode_(GL_LINEAR),
  sourceBuffer_(GL_COLOR_BUFFER_BIT),
  screenBuffer_(GL_FRONT)
{
}

string BlitToScreen::name()
{
  return "BlitToScreen";
}

void BlitToScreen::set_filterMode(GLenum filterMode)
{
  filterMode_ = filterMode;
}
void BlitToScreen::set_screenBuffer(GLenum screenBuffer)
{
  screenBuffer_ = screenBuffer;
}
void BlitToScreen::set_sourceBuffer(GLenum sourceBuffer)
{
  sourceBuffer_ = sourceBuffer;
}

void BlitToScreen::enable(RenderState *state)
{
  State::enable(state);
  FrameBufferObject::blitCopyToScreen(
      *fbo_.get(),
      windowSize_.x, windowSize_.y,
      attachment_,
      sourceBuffer_,
      filterMode_,
      screenBuffer_);
}
