#include "blit-state.h"

using namespace regen;

BlitToFBO::BlitToFBO(
		const ref_ptr<FBO> &src,
		const ref_ptr<FBO> &dst,
		GLenum srcAttachment,
		GLenum dstAttachment,
		GLboolean keepRatio)
		: BlitState(),
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

void BlitToFBO::disable(RenderState *rs) {
	State::disable(rs);
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
		const ref_ptr<Screen> &screen,
		GLenum attachment,
		GLboolean keepRatio)
		: BlitState(),
		  fbo_(fbo),
		  screen_(screen),
		  attachment_(attachment),
		  filterMode_(GL_LINEAR),
		  sourceBuffer_(GL_COLOR_BUFFER_BIT),
		  keepRatio_(keepRatio) {
}

void BlitToScreen::set_filterMode(GLenum filterMode) { filterMode_ = filterMode; }

void BlitToScreen::set_sourceBuffer(GLenum sourceBuffer) { sourceBuffer_ = sourceBuffer; }

void BlitToScreen::disable(RenderState *rs) {
	State::enable(rs);
	auto viewport = screen_->viewport();
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
		const ref_ptr<Screen> &screen,
		GLenum attachment)
		: BlitToScreen(fbo, screen, attachment),
		  texture_(texture),
		  baseAttachment_(attachment) {
}

void BlitTexToScreen::enable(RenderState *state) {
	attachment_ = baseAttachment_ + !texture_->objectIndex();
	BlitToScreen::enable(state);
}

ref_ptr<BlitState> BlitState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	if (!input.hasAttribute("src-fbo")) {
		REGEN_WARN("Ignoring " << input.getDescription() << " without src-fbo attribute.");
		return {};
	}
	if (!input.hasAttribute("dst-fbo")) {
		REGEN_WARN("Ignoring " << input.getDescription() << " without dst-fbo attribute.");
		return {};
	}
	const std::string srcID = input.getValue("src-fbo");
	const std::string dstID = input.getValue("dst-fbo");
	ref_ptr<FBO> src, dst;
	if (srcID != "SCREEN") {
		src = scene->getResource<FBO>(srcID);
		if (src.get() == nullptr) {
			REGEN_WARN("Unable to find FBO with name '" << srcID << "'.");
			return {};
		}
	}
	if (dstID != "SCREEN") {
		dst = scene->getResource<FBO>(dstID);
		if (dst.get() == nullptr) {
			REGEN_WARN("Unable to find FBO with name '" << dstID << "'.");
			return {};
		}
	}
	bool keepAspect = input.getValue<GLuint>("keep-aspect", false);
	if (src.get() != nullptr && dst.get() != nullptr) {
		if (input.getValue("src-attachment") == "depth" || input.getValue("dst-attachment") == "depth") {
			auto blit = ref_ptr<BlitToFBO>::alloc(
					src, dst,
					GL_DEPTH_ATTACHMENT,
					GL_DEPTH_ATTACHMENT,
					keepAspect);
			blit->set_sourceBuffer(GL_DEPTH_BUFFER_BIT);
			blit->set_filterMode(GL_NEAREST);
			return blit;
		} else {
			auto srcAttachment = input.getValue<GLuint>("src-attachment", 0u);
			auto dstAttachment = input.getValue<GLuint>("dst-attachment", 0u);
			auto state = ref_ptr<BlitToFBO>::alloc(
					src, dst,
					GL_COLOR_ATTACHMENT0 + srcAttachment,
					GL_COLOR_ATTACHMENT0 + dstAttachment,
					keepAspect);
			if (input.hasAttribute("filter-mode")) {
				auto filterMode = glenum::filterMode(input.getValue<std::string>("filter-mode", "LINEAR"));
				if (filterMode != GL_NEAREST && filterMode != GL_LINEAR) {
					REGEN_WARN("Invalid filter mode '" << input.getValue("filter-mode") << "' for " << input.getDescription() << ".");
				} else {
					state->set_filterMode(filterMode);
				}
			}
			return state;
		}
	} else if (src.get() != nullptr) {
		// Blit Texture to Screen
		auto srcAttachment = input.getValue<GLuint>("src-attachment", 0u);
		return ref_ptr<BlitToScreen>::alloc(src,
											scene->screen(),
											GL_COLOR_ATTACHMENT0 + srcAttachment,
											keepAspect);
	} else if (dst.get() != nullptr) {
		REGEN_WARN(input.getDescription() << ", blitting Screen to FBO not supported.");
	} else {
		REGEN_WARN("No src or dst FBO specified for " << input.getDescription() << ".");
	}
	return {};
}
