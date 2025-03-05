#include "screen-state.h"
#include "regen/gl-types/fbo.h"

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
	joinShaderInput(viewport_);

	inverseViewport_ = ref_ptr<ShaderInput2f>::alloc("inverseViewport");
	inverseViewport_->setUniformData(Vec2f(0.0f));
	joinShaderInput(inverseViewport_);
}

void ScreenState::enable(RenderState *state) {
	if (lastViewportStamp_ != windowViewport_->stamp()) {
		auto winViewport = windowViewport_->getVertex(0);
		glViewport_.z = winViewport.r.x;
		glViewport_.w = winViewport.r.y;
		viewport_->setVertex(0, Vec2f(winViewport.r.x, winViewport.r.y));
		inverseViewport_->setUniformData(
				Vec2f(1.0f / (GLfloat) winViewport.r.x, 1.0f / (GLfloat) winViewport.r.y));
		winViewport.unmap();
		lastViewportStamp_ = windowViewport_->stamp();
	}

	state->drawFrameBuffer().push(0);
	FBO::screen().drawBuffer_.push(drawBuffer_);
	state->viewport().push(glViewport_);
	State::enable(state);
}

void ScreenState::disable(RenderState *state) {
	State::disable(state);
	state->viewport().pop();
	FBO::screen().drawBuffer_.pop();
	state->drawFrameBuffer().pop();
}
