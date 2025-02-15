/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blit-state.h"

using namespace regen;

BlitToFBO::BlitToFBO(
		const ref_ptr<FBO> &src,
		const ref_ptr<FBO> &dst,
		GLenum srcAttachment,
		GLenum dstAttachment,
		GLboolean keepRatio)
		: State(),
		  src_(src),
		  dst_(dst),
		  srcAttachment_(srcAttachment),
		  dstAttachment_(dstAttachment),
		  filterMode_(GL_LINEAR),
		  sourceBuffer_(GL_COLOR_BUFFER_BIT),
		  keepRatio_(keepRatio) {
}

void BlitToFBO::set_filterMode(GLenum filterMode) { filterMode_ = filterMode; }

void BlitToFBO::set_sourceBuffer(GLenum sourceBuffer) { sourceBuffer_ = sourceBuffer; }

void BlitToFBO::enable(RenderState *rs) {
	State::enable(rs);
	src_->blitCopy(
			*dst_.get(),
			srcAttachment_,
			dstAttachment_,
			sourceBuffer_,
			filterMode_,
			keepRatio_);
}

//////////////
//////////////

BlitToScreen::BlitToScreen(
		const ref_ptr<FBO> &fbo,
		const ref_ptr<ShaderInput2i> &viewport,
		GLenum attachment,
		GLboolean keepRatio)
		: State(),
		  fbo_(fbo),
		  viewport_(viewport),
		  attachment_(attachment),
		  filterMode_(GL_LINEAR),
		  sourceBuffer_(GL_COLOR_BUFFER_BIT),
		  keepRatio_(keepRatio) {
}

void BlitToScreen::set_filterMode(GLenum filterMode) { filterMode_ = filterMode; }

void BlitToScreen::set_sourceBuffer(GLenum sourceBuffer) { sourceBuffer_ = sourceBuffer; }

void BlitToScreen::enable(RenderState *rs) {
	State::enable(rs);
	auto viewport = viewport_->getVertex(0);
	fbo_->blitCopyToScreen(
			viewport.r.x,
			viewport.r.y,
			attachment_,
			sourceBuffer_,
			filterMode_,
			keepRatio_);
}

///////////////

BlitTexToScreen::BlitTexToScreen(
		const ref_ptr<FBO> &fbo,
		const ref_ptr<Texture> &texture,
		const ref_ptr<ShaderInput2i> &viewport,
		GLenum attachment)
		: BlitToScreen(fbo, viewport, attachment),
		  texture_(texture),
		  baseAttachment_(attachment) {
}

void BlitTexToScreen::enable(RenderState *state) {
	attachment_ = baseAttachment_ + !texture_->objectIndex();
	BlitToScreen::enable(state);
}
