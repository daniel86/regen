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
#ifdef USE_FBO_UBO
	// TODO: there are some nodes in the scene graph with special handling for viewport and inverseViewport
	//         uniforms. This includes BloomPass and FilterSequence. I think the problem is that FBOState appears
	//         as a parent, and then the shader is compiled for a uniform block.
	//         However, BloomPass and FilterSequence rather use gUniform* functions to set the uniforms, which
	//         pretty sure does not work with uniform blocks.
	joinShaderInput(fbo->uniforms());
#else
	joinShaderInput(fbo->viewport());
	joinShaderInput(fbo->inverseViewport());
#endif
	for (auto &attachment : fbo->colorTextures()) {
		shaderDefine(REGEN_STRING("FBO_ATTACHMENT_" << attachment->name()), "TRUE");
	}
}

void FBOState::setClearDepth() {
	if (clearDepthCallable_.get()) {
		disjoinStates(clearDepthCallable_);
	}
	clearDepthCallable_ = ref_ptr<ClearDepthState>::alloc();
	joinStates(clearDepthCallable_);

	// make sure clearing is done before draw buffer configuration
	if (drawBufferCallable_.get() != nullptr) {
		disjoinStates(drawBufferCallable_);
		joinStates(drawBufferCallable_);
	}
}

void FBOState::setClearColor(const ClearColorState::Data &data) {
	if (clearColorCallable_.get() == nullptr) {
		clearColorCallable_ = ref_ptr<ClearColorState>::alloc(fbo_);
		joinStates(clearColorCallable_);
	}
	clearColorCallable_->data.push_back(data);

	// make sure clearing is done before draw buffer configuration
	if (drawBufferCallable_.get() != nullptr) {
		disjoinStates(drawBufferCallable_);
		joinStates(drawBufferCallable_);
	}
}

void FBOState::setClearColor(const std::list<ClearColorState::Data> &data) {
	if (clearColorCallable_.get()) {
		disjoinStates(clearColorCallable_);
	}
	clearColorCallable_ = ref_ptr<ClearColorState>::alloc(fbo_);
	for (auto it = data.begin(); it != data.end(); ++it) {
		clearColorCallable_->data.push_back(*it);
	}
	joinStates(clearColorCallable_);

	// make sure clearing is done before draw buffer configuration
	if (drawBufferCallable_.get() != nullptr) {
		disjoinStates(drawBufferCallable_);
		joinStates(drawBufferCallable_);
	}
}

void FBOState::addDrawBuffer(GLenum colorAttachment) {
	if (drawBufferCallable_.get() == nullptr) {
		drawBufferCallable_ = ref_ptr<DrawBufferState>::alloc(fbo_);
		joinStates(drawBufferCallable_);
	}
	auto *s = (DrawBufferState *) drawBufferCallable_.get();
	s->colorBuffers.buffers_.push_back(colorAttachment);
}

void FBOState::setDrawBuffers(const std::vector<GLenum> &attachments) {
	if (drawBufferCallable_.get() != nullptr) {
		disjoinStates(drawBufferCallable_);
	}
	drawBufferCallable_ = ref_ptr<DrawBufferState>::alloc(fbo_);
	auto *s = (DrawBufferState *) drawBufferCallable_.get();
	s->colorBuffers.buffers_ = attachments;
	joinStates(drawBufferCallable_);
}

void FBOState::setPingPongBuffers(const std::vector<GLenum> &attachments) {
	if (drawBufferCallable_.get() != nullptr) {
		disjoinStates(drawBufferCallable_);
	}
	drawBufferCallable_ = ref_ptr<PingPongBufferState>::alloc(fbo_);
	auto *s = (PingPongBufferState *) drawBufferCallable_.get();
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
	auto winViewport = windowViewport_->getVertex(0);
	glViewport_.z = winViewport.r.x;
	glViewport_.w = winViewport.r.y;
	viewport_->setVertex(0, Vec2f(winViewport.r.x, winViewport.r.y));
	inverseViewport_->setUniformData(
			Vec2f(1.0f / (GLfloat) winViewport.r.x, 1.0f / (GLfloat) winViewport.r.y));
	winViewport.unmap();

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
