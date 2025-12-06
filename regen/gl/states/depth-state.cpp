#include <regen/utility/strings.h>

#include "depth-state.h"
#include "atomic-states.h"

using namespace regen;

void DepthState::set_useDepthWrite(bool useDepthWrite) {
	if (depthWriteToggle_.get()) {
		disjoinStates(depthWriteToggle_);
	}
	depthWriteToggle_ = ref_ptr<ToggleDepthWriteState>::alloc(useDepthWrite);
	joinStates(depthWriteToggle_);
}

void DepthState::set_useDepthTest(bool useDepthTest) {
	if (depthTestToggle_.get()) {
		disjoinStates(depthTestToggle_);
	}
	if (useDepthTest) {
		depthTestToggle_ = ref_ptr<ToggleState>::alloc(RenderState::DEPTH_TEST, true);
	} else {
		depthTestToggle_ = ref_ptr<ToggleState>::alloc(RenderState::DEPTH_TEST, false);
	}
	joinStates(depthTestToggle_);
}

void DepthState::set_depthFunc(GLenum depthFunc) {
	if (depthFunc_.get()) {
		disjoinStates(depthFunc_);
	}
	depthFunc_ = ref_ptr<DepthFuncState>::alloc(depthFunc);
	joinStates(depthFunc_);
}

void DepthState::set_depthRange(double nearVal, double farVal) {
	if (depthRange_.get()) {
		disjoinStates(depthRange_);
	}
	depthRange_ = ref_ptr<DepthRangeState>::alloc(nearVal, farVal);
	joinStates(depthRange_);
}

ref_ptr<DepthState> DepthState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();

	depth->set_useDepthTest(input.getValue<bool>("test", true));
	depth->set_useDepthWrite(input.getValue<bool>("write", true));

	if (input.hasAttribute("range")) {
		auto range = input.getValue<Vec2f>("range", Vec2f::zero());
		depth->set_depthRange(range.x, range.y);
	}

	if (input.hasAttribute("function")) {
		depth->set_depthFunc(glenum::compareFunction(input.getValue("function")));
	}

	return depth;
}
