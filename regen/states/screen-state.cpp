#include "screen-state.h"

using namespace regen;

ScreenState::ScreenState(
		const ref_ptr<Screen> &screen,
		GLenum screenBuffer)
		: State(),
		  screen_(screen),
		  drawBuffer_(screenBuffer) {
	glViewport_ = Vec4ui(0u);

	viewport_ = ref_ptr<ShaderInput2f>::alloc("viewport");
	viewport_->setEditable(false);
	viewport_->setUniformData(Vec2f(0.0f));
	setInput(viewport_);

	inverseViewport_ = ref_ptr<ShaderInput2f>::alloc("inverseViewport");
	inverseViewport_->setEditable(false);
	inverseViewport_->setUniformData(Vec2f(0.0f));
	setInput(inverseViewport_);
}

void ScreenState::enable(RenderState *rs) {
	if (lastViewportStamp_ != screen_->stampOfWriteData()) {
		auto winViewport = screen_->viewport();
		glViewport_.z = winViewport.r.x;
		glViewport_.w = winViewport.r.y;
		viewport_->setVertex(0, Vec2f(
			static_cast<float>(winViewport.r.x),
			static_cast<float>(winViewport.r.y)));
		inverseViewport_->setVertex(0, Vec2f(
			1.0f / static_cast<float>(winViewport.r.x),
			1.0f / static_cast<float>(winViewport.r.y)));
		lastViewportStamp_ = screen_->stampOfWriteData();
	}

	rs->drawFrameBuffer().apply(0);
	FBO::screen().applyDrawBuffer(drawBuffer_);
	rs->viewport().apply(glViewport_);
	State::enable(rs);
}

void ScreenState::disable(RenderState *rs) {
	State::disable(rs);
	if (parentFBO_.get()) {
		rs->drawFrameBuffer().apply(parentFBO_->fbo()->id());
		rs->viewport().apply(parentFBO_->fbo()->glViewport());
	}
}
