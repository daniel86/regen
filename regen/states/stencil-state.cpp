#include "stencil-state.h"
#include "atomic-states.h"

using namespace regen;

namespace regen {
	class StencilFuncState : public State {
	public:
		StencilFunc v_;

		StencilFuncState() : v_({GL_ALWAYS, 0, 0x0}) {}
		void enable(RenderState *rs) override { rs->stencilFunc().push(v_); }
		void disable(RenderState *rs) override { rs->stencilFunc().pop(); }
	};

	class StencilOpState : public State {
	public:
		StencilOp v_;

		StencilOpState() : v_({GL_KEEP, GL_KEEP, GL_KEEP}) {}
		void enable(RenderState *rs) override { rs->stencilOp().push(v_); }
		void disable(RenderState *rs) override { rs->stencilOp().pop(); }
	};

	class StencilMaskState : public State {
	public:
		unsigned int v_;

		StencilMaskState() : v_(0) {}
		void enable(RenderState *rs) override { rs->stencilMask().push(v_); }
		void disable(RenderState *rs) override { rs->stencilMask().pop(); }
	};
}

StencilState::StencilState() = default;

void StencilState::set_useStencilTest(bool useStencilTest) {
	if (stencilTestToggle_.get()) {
		disjoinStates(stencilTestToggle_);
	}
	if (useStencilTest) {
		stencilTestToggle_ = ref_ptr<ToggleState>::alloc(RenderState::STENCIL_TEST, GL_TRUE);
	} else {
		stencilTestToggle_ = ref_ptr<ToggleState>::alloc(RenderState::STENCIL_TEST, GL_FALSE);
	}
	joinStates(stencilTestToggle_);
}

void StencilState::set_stencilMask(unsigned int mask) {
	if(!stencilMaskState_.get()) {
		stencilMaskState_ = ref_ptr<StencilMaskState>::alloc();
		joinStates(stencilMaskState_);
	}
	((StencilMaskState*)stencilMaskState_.get())->v_ = mask;
}

void StencilState::set_stencilTestMask(unsigned int mask) {
	if(!stencilFuncState_.get()) {
		stencilFuncState_ = ref_ptr<StencilFuncState>::alloc();
		joinStates(stencilFuncState_);
	}
	((StencilFuncState*)stencilFuncState_.get())->v_.mask_ = mask;
}

void StencilState::set_stencilTestFunc(GLenum func) {
	if(!stencilFuncState_.get()) {
		stencilFuncState_ = ref_ptr<StencilFuncState>::alloc();
		joinStates(stencilFuncState_);
	}
	((StencilFuncState*)stencilFuncState_.get())->v_.func_ = func;
}

void StencilState::set_stencilTestRef(int ref) {
	if(!stencilFuncState_.get()) {
		stencilFuncState_ = ref_ptr<StencilFuncState>::alloc();
		joinStates(stencilFuncState_);
	}
	((StencilFuncState*)stencilFuncState_.get())->v_.ref_ = ref;
}

void StencilState::set_depthTestPass(GLenum op) {
	if(!stencilOpState_.get()) {
		stencilOpState_ = ref_ptr<StencilOpState>::alloc();
		joinStates(stencilOpState_);
	}
	((StencilOpState*)stencilOpState_.get())->v_.z = static_cast<int>(op);
}

void StencilState::set_depthTestFail(GLenum op) {
	if(!stencilOpState_.get()) {
		stencilOpState_ = ref_ptr<StencilOpState>::alloc();
		joinStates(stencilOpState_);
	}
	((StencilOpState*)stencilOpState_.get())->v_.y = static_cast<int>(op);
}

void StencilState::set_stencilTestFail(GLenum op) {
	if(!stencilOpState_.get()) {
		stencilOpState_ = ref_ptr<StencilOpState>::alloc();
		joinStates(stencilOpState_);
	}
	((StencilOpState*)stencilOpState_.get())->v_.x = static_cast<int>(op);
}

ref_ptr<StencilState> StencilState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto state = ref_ptr<StencilState>::alloc();

	if (input.hasAttribute("test")) {
		state->set_useStencilTest(input.getValue<bool>("test", true));
	}
	if (input.hasAttribute("mask")) {
		state->set_stencilMask(input.getValue<unsigned int>("mask", 0));
	}
	if (input.hasAttribute("test-mask")) {
		state->set_stencilTestMask(input.getValue<unsigned int>("test-mask", 0));
	}
	if (input.hasAttribute("test-func")) {
		state->set_stencilTestFunc(glenum::compareFunction(
				input.getValue<std::string>("test-func", "ALWAYS")));
	}
	if (input.hasAttribute("test-ref")) {
		state->set_stencilTestRef(input.getValue<int>("test-ref", 0));
	}
	if (input.hasAttribute("dppass")) {
		state->set_depthTestPass(glenum::stencilOp(
				input.getValue<std::string>("dppass", "KEEP")));
	}
	if (input.hasAttribute("dpfail")) {
		state->set_depthTestFail(glenum::stencilOp(
				input.getValue<std::string>("dpfail", "KEEP")));
	}
	if (input.hasAttribute("sfail")) {
		state->set_stencilTestFail(glenum::stencilOp(
				input.getValue<std::string>("sfail", "KEEP")));
	}

	return state;
}
