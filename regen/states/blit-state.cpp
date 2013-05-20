/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blit-state.h"
using namespace regen;

BlitToScreen::BlitToScreen(
    const ref_ptr<FBO> &fbo,
    const ref_ptr<ShaderInput2i> &viewport,
    GLenum attachment)
: State(),
  fbo_(fbo),
  viewport_(viewport),
  attachment_(attachment),
  filterMode_(GL_LINEAR),
  sourceBuffer_(GL_COLOR_BUFFER_BIT)
{
}

void BlitToScreen::set_filterMode(GLenum filterMode)
{ filterMode_ = filterMode; }
void BlitToScreen::set_sourceBuffer(GLenum sourceBuffer)
{ sourceBuffer_ = sourceBuffer; }

void BlitToScreen::enable(RenderState *rs)
{
  State::enable(rs);
  fbo_->blitCopyToScreen(
      viewport_->getVertex(0).x,
      viewport_->getVertex(0).y,
      attachment_,
      sourceBuffer_,
      filterMode_);
}

///////////////

BlitTexToScreen::BlitTexToScreen(
    const ref_ptr<FBO> &fbo,
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
  attachment_ = baseAttachment_ + !texture_->objectIndex();
  BlitToScreen::enable(state);
}
