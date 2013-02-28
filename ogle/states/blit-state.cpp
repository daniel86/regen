/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blit-state.h"

BlitToScreen::BlitToScreen(
    const ref_ptr<FrameBufferObject> &fbo,
    Vec2ui *viewport,
    GLenum attachment)
: State(),
  fbo_(fbo),
  attachment_(attachment),
  filterMode_(GL_LINEAR),
  screenBuffer_(GL_FRONT),
  sourceBuffer_(GL_COLOR_BUFFER_BIT),
  viewport_(viewport)
{
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
  fbo_->blitCopyToScreen(
      viewport_->x, viewport_->y,
      attachment_,
      sourceBuffer_,
      filterMode_,
      screenBuffer_);
}

///////////////

BlitTexToScreen::BlitTexToScreen(
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &texture,
    Vec2ui *viewport,
    GLenum attachment)
: BlitToScreen(fbo,viewport,attachment),
  texture_(texture),
  baseAttachment_(attachment)
{
}

void BlitTexToScreen::enable(RenderState *state)
{
  attachment_ = baseAttachment_ + !texture_->bufferIndex();
  BlitToScreen::enable(state);
  attachment_ = baseAttachment_;
}
