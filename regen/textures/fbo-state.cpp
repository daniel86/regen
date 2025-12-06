#include "regen/utility/strings.h"
#include "regen/gl/states/atomic-states.h"

#include "fbo-state.h"
#include "screen-state.h"

using namespace regen;

FBOState::FBOState(const ref_ptr<FBO> &fbo)
		: State(), fbo_(fbo) {
	setInput(fbo->viewport());
	setInput(fbo->inverseViewport());
	for (uint32_t attachmentIdx = 0; attachmentIdx < fbo->colorTextures().size(); ++attachmentIdx) {
		auto &attachment = fbo->colorTextures()[attachmentIdx];
		shaderDefine(REGEN_STRING("HAS_ATTACHMENT_" << attachment->name()), "TRUE");
		shaderDefine(REGEN_STRING("ATTACHMENT_IDX_" << attachment->name()), REGEN_STRING(attachmentIdx));
		shaderDefine(REGEN_STRING("ATTACHMENT_NAME_" << attachmentIdx), attachment->name());
	}
	shaderDefine("NUM_ATTACHMENTS", REGEN_STRING(fbo->colorTextures().size()));
}

void FBOState::createClearState() {
	if (!clearCallable_.get()) {
		clearCallable_ = ref_ptr<ClearState>::alloc(fbo_);
		joinStates(clearCallable_);
		// make sure clearing is done before draw buffer configuration
		if (drawBufferCallable_.get() != nullptr) {
			disjoinStates(drawBufferCallable_);
			joinStates(drawBufferCallable_);
		}
	}
}

void FBOState::setClearDepth() {
	createClearState();
	clearCallable_->addClearBit(GL_DEPTH_BUFFER_BIT);
}

void FBOState::setClearStencil() {
	createClearState();
	clearCallable_->addClearBit(GL_STENCIL_BUFFER_BIT);
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

void FBOState::setParentBufferState(const ref_ptr<FBOState> &parentFBO) {
	parentFBO_ = parentFBO;
}

void FBOState::enable(RenderState *rs) {
	rs->drawFrameBuffer().apply(fbo_->id());
	rs->viewport().apply(fbo_->glViewport());
	State::enable(rs);
}

void FBOState::disable(RenderState *rs) {
	State::disable(rs);
	if (parentFBO_.get()) {
		rs->drawFrameBuffer().apply(parentFBO_->fbo_->id());
		rs->viewport().apply(parentFBO_->fbo_->glViewport());
	}
}

void FBOState::resize(uint32_t width, uint32_t height) {
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

static std::vector<GLenum> getFBOAttachments_(scene::SceneInputNode &input, const std::string &key) {
	auto attachments_str = getFBOAttachments(input, key);
	std::vector<GLenum> attachments(attachments_str.size());
	for (uint32_t i = 0u; i < attachments_str.size(); ++i) {
		auto &attachments_str_i = attachments_str[i];
		if (attachments_str_i.empty()) {
			REGEN_WARN("Empty attachment in " << input.getDescription() << ".");
		}
		else if (attachments_str_i == "none" || attachments_str_i == "NONE") {
			attachments[i] = GL_NONE;
		}
		else if (attachments_str_i == "front-left") {
			attachments[i] = GL_FRONT_LEFT;
		}
		else if (attachments_str_i == "front-right") {
			attachments[i] = GL_FRONT_RIGHT;
		}
		else if (attachments_str_i == "back-left") {
			attachments[i] = GL_BACK_LEFT;
		}
		else if (attachments_str_i == "back-right") {
			attachments[i] = GL_BACK_RIGHT;
		}
		else {
			int v;
			std::stringstream ss(attachments_str[i]);
			ss >> v;
			attachments[i] = GL_COLOR_ATTACHMENT0 + v;
		}
	}
	return attachments;
}

ref_ptr<State> FBOState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	if (input.getName() == "SCREEN") {
		GLenum drawBuffer = glenum::drawBuffer(
				input.getValue<std::string>("draw-buffer", "FRONT"));
		ref_ptr<ScreenState> screenState =
				ref_ptr<ScreenState>::alloc(scene->screen(), drawBuffer);
		ref_ptr<State> parent = ctx.parent()->getParentFrameBuffer();
		if (parent.get() != nullptr) {
			screenState->setParentBufferState(ref_ptr<FBOState>::dynamicCast(parent));
		}

		return screenState;
	} else {
		ref_ptr<FBO> fbo = scene->getResource<FBO>(input.getName());
		if (fbo.get() == nullptr) {
			REGEN_WARN("Unable to find FBO for '" << input.getDescription() << "'.");
			return {};
		}
		ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);
		ref_ptr<State> parent = ctx.parent()->getParentFrameBuffer();
		if (parent.get() != nullptr) {
			fboState->setParentBufferState(ref_ptr<FBOState>::dynamicCast(parent));
		}

		if (input.hasAttribute("clear-depth") &&
			input.getValue<int>("clear-depth", 1)) {
			fboState->setClearDepth();
		}

		if (input.hasAttribute("clear-stencil") &&
			input.getValue<int>("clear-stencil", 1)) {
			fboState->setClearStencil();
		}

		if (input.hasAttribute("clear-buffers")) {
			const auto buffers = getFBOAttachments_(input, "clear-buffers");
			ClearColorState::Data data;
			data.clearColor = input.getValue<Vec4f>("clear-color", Vec4f::zero());
			data.colorBuffers = DrawBuffers(buffers);
			fboState->setClearColor(data);
		}

		if (input.hasAttribute("draw-buffers")) {
			const auto buffers = getFBOAttachments_(input, "draw-buffers");
			fboState->setDrawBuffers(buffers);
		} else if (input.hasAttribute("ping-pong-buffers")) {
			const auto buffers = getFBOAttachments_(input, "ping-pong-buffers");
			fboState->setPingPongBuffers(buffers);
		}

		return fboState;
	}
}
