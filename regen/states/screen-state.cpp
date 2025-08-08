#include "screen-state.h"

using namespace regen;

ScreenState::ScreenState(
		const ref_ptr<ShaderInput2i> &windowViewport,
		GLenum screenBuffer)
		: State(),
		  windowViewport_(windowViewport),
		  drawBuffer_(screenBuffer) {
	glViewport_ = Vec4ui(0u);

	viewport_ = ref_ptr<ShaderInput2f>::alloc("viewport");
	viewport_->setUniformData(Vec2f(0.0f));
	setInput(viewport_);

	inverseViewport_ = ref_ptr<ShaderInput2f>::alloc("inverseViewport");
	inverseViewport_->setUniformData(Vec2f(0.0f));
	setInput(inverseViewport_);
}

void ScreenState::enable(RenderState *rs) {
	if (lastViewportStamp_ != windowViewport_->stamp()) {
		auto winViewport = windowViewport_->getVertex(0);
		glViewport_.z = winViewport.r.x;
		glViewport_.w = winViewport.r.y;
		viewport_->setVertex(0, Vec2f(
			static_cast<float>(winViewport.r.x),
			static_cast<float>(winViewport.r.y)));
		inverseViewport_->setVertex(0, Vec2f(
			1.0f / static_cast<float>(winViewport.r.x),
			1.0f / static_cast<float>(winViewport.r.y)));
		winViewport.unmap();
		lastViewportStamp_ = windowViewport_->stamp();
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
