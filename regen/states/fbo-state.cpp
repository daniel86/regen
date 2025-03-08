/*
 * fbo-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/atomic-states.h>

#include "fbo-state.h"
#include "screen-state.h"

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
	for (auto &attachment: fbo->colorTextures()) {
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
	for (const auto &it: data) {
		clearColorCallable_->data.push_back(it);
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

static std::vector<std::string> getFBOAttachments(scene::SceneInputNode &input, const std::string &key) {
	std::vector<std::string> out;
	auto attachments = input.getValue<std::string>(key, "");
	if (attachments.empty()) {
		REGEN_WARN("No attachments specified in " << input.getDescription() << ".");
	} else {
		boost::split(out, attachments, boost::is_any_of(","));
	}
	return out;
}

ref_ptr<State> FBOState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	if (input.getName() == "SCREEN") {
		GLenum drawBuffer = glenum::drawBuffer(
				input.getValue<std::string>("draw-buffer", "FRONT"));
		ref_ptr<ScreenState> screenState =
				ref_ptr<ScreenState>::alloc(scene->getViewport(), drawBuffer);

		return screenState;
	} else {
		ref_ptr<FBO> fbo = scene->getResource<FBO>(input.getName());
		if (fbo.get() == nullptr) {
			REGEN_WARN("Unable to find FBO for '" << input.getDescription() << "'.");
			return {};
		}
		ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);

		if (input.hasAttribute("clear-depth") &&
			input.getValue<bool>("clear-depth", true)) {
			fboState->setClearDepth();
		}

		if (input.hasAttribute("clear-buffers")) {
			std::vector<std::string> idVec = getFBOAttachments(input, "clear-buffers");
			std::vector<GLenum> buffers(idVec.size());
			for (GLuint i = 0u; i < idVec.size(); ++i) {
				GLint v;
				std::stringstream ss(idVec[i]);
				ss >> v;
				buffers[i] = GL_COLOR_ATTACHMENT0 + v;
			}

			ClearColorState::Data data;
			data.clearColor = input.getValue<Vec4f>("clear-color", Vec4f(0.0));
			data.colorBuffers = DrawBuffers(buffers);
			fboState->setClearColor(data);
		}

		if (input.hasAttribute("draw-buffers")) {
			std::vector<std::string> idVec = getFBOAttachments(input, "draw-buffers");
			std::vector<GLenum> buffers(idVec.size());
			for (GLuint i = 0u; i < idVec.size(); ++i) {
				GLint v;
				std::stringstream ss(idVec[i]);
				ss >> v;
				buffers[i] = GL_COLOR_ATTACHMENT0 + v;
			}
			fboState->setDrawBuffers(buffers);
		} else if (input.hasAttribute("ping-pong-buffers")) {
			std::vector<std::string> idVec = getFBOAttachments(input, "ping-pong-buffers");
			std::vector<GLenum> buffers(idVec.size());
			for (GLuint i = 0u; i < idVec.size(); ++i) {
				GLint v;
				std::stringstream ss(idVec[i]);
				ss >> v;
				buffers[i] = GL_COLOR_ATTACHMENT0 + v;
			}
			fboState->setPingPongBuffers(buffers);
		}

		return fboState;
	}
}
