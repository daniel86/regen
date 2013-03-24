/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blit-state.h"
using namespace ogle;

BlitToScreen::BlitToScreen(
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<ShaderInput2i> &viewport,
    GLenum attachment)
: State(),
  fbo_(fbo),
  viewport_(viewport),
  attachment_(attachment),
  filterMode_(GL_LINEAR),
  screenBuffer_(GL_FRONT),
  sourceBuffer_(GL_COLOR_BUFFER_BIT)
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
      viewport_->getVertex2i(0).x,
      viewport_->getVertex2i(0).y,
      attachment_,
      sourceBuffer_,
      filterMode_,
      screenBuffer_);
}

///////////////

BlitTexToScreen::BlitTexToScreen(
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &texture,
    const ref_ptr<ShaderInput2i> &viewport,
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
