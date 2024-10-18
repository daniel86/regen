/*
 * fbo-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/atomic-states.h>

#include "fbo-state.h"

using namespace regen;

FBOState::FBOState(const ref_ptr<FBO> &fbo)
		: State(), fbo_(fbo) {
	joinShaderInput(fbo->viewport());
	joinShaderInput(fbo->inverseViewport());
}

void FBOState::setClearDepth() {
	if (clearDepthCallable_.get()) {
		disjoinStates(clearDepthCallable_);
	}
	clearDepthCallable_ = ref_ptr<ClearDepthState>::alloc();
	joinStates(clearDepthCallable_);

	// make sure clearing is done before draw buffer configuration
	if (drawBufferCallable_.get() != NULL) {
		disjoinStates(drawBufferCallable_);
		joinStates(drawBufferCallable_);
	}
}

void FBOState::setClearColor(const ClearColorState::Data &data) {
	if (clearColorCallable_.get() == NULL) {
		clearColorCallable_ = ref_ptr<ClearColorState>::alloc(fbo_);
		joinStates(clearColorCallable_);
	}
	clearColorCallable_->data.push_back(data);

	// make sure clearing is done before draw buffer configuration
	if (drawBufferCallable_.get() != NULL) {
		disjoinStates(drawBufferCallable_);
		joinStates(drawBufferCallable_);
	}
}

void FBOState::setClearColor(const std::list<ClearColorState::Data> &data) {
	if (clearColorCallable_.get()) {
		disjoinStates(clearColorCallable_);
	}
	clearColorCallable_ = ref_ptr<ClearColorState>::alloc(fbo_);
	for (std::list<ClearColorState::Data>::const_iterator
				 it = data.begin(); it != data.end(); ++it) {
		clearColorCallable_->data.push_back(*it);
	}
	joinStates(clearColorCallable_);

	// make sure clearing is done before draw buffer configuration
	if (drawBufferCallable_.get() != NULL) {
		disjoinStates(drawBufferCallable_);
		joinStates(drawBufferCallable_);
	}
}

void FBOState::addDrawBuffer(GLenum colorAttachment) {
	if (drawBufferCallable_.get() == NULL) {
		drawBufferCallable_ = ref_ptr<DrawBufferState>::alloc(fbo_);
		joinStates(drawBufferCallable_);
	}
	DrawBufferState *s = (DrawBufferState *) drawBufferCallable_.get();
	s->colorBuffers.buffers_.push_back(colorAttachment);
}

void FBOState::setDrawBuffers(const std::vector<GLenum> &attachments) {
	if (drawBufferCallable_.get() != NULL) {
		disjoinStates(drawBufferCallable_);
	}
	drawBufferCallable_ = ref_ptr<DrawBufferState>::alloc(fbo_);
	DrawBufferState *s = (DrawBufferState *) drawBufferCallable_.get();
	s->colorBuffers.buffers_ = attachments;
	joinStates(drawBufferCallable_);
}

void FBOState::setPingPongBuffers(const std::vector<GLenum> &attachments) {
	if (drawBufferCallable_.get() != NULL) {
		disjoinStates(drawBufferCallable_);
	}
	drawBufferCallable_ = ref_ptr<PingPongBufferState>::alloc(fbo_);
	PingPongBufferState *s = (PingPongBufferState *) drawBufferCallable_.get();
	s->colorBuffers.buffers_ = attachments;
	joinStates(drawBufferCallable_);
}

void FBOState::enable(RenderState *state) {
	state->drawFrameBuffer().push(fbo_->id());
	state->viewport().push(fbo_->glViewport());
	State::enable(state);
}

void FBOState::disable(RenderState *state) {
	State::disable(state);
	state->viewport().pop();
	state->drawFrameBuffer().pop();
}

void FBOState::resize(GLuint width, GLuint height) {
	fbo_->resize(width, height, fbo_->depth());
}

const ref_ptr<FBO> &FBOState::fbo() { return fbo_; }

///////////////
///////////////

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
	const Vec2i winViewport = windowViewport_->getVertex(0);
	glViewport_.z = winViewport.x;
	glViewport_.w = winViewport.y;
	viewport_->setVertex(0, Vec2f(winViewport.x, winViewport.y));
	inverseViewport_->setUniformData(
			Vec2f(1.0 / (GLfloat) winViewport.x, 1.0 / (GLfloat) winViewport.y));

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
