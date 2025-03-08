/*
 * blit-to-screen.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/atomic-states.h>

#include "depth-state.h"

using namespace regen;

void DepthState::set_useDepthWrite(GLboolean useDepthWrite) {
	if (depthWriteToggle_.get()) {
		disjoinStates(depthWriteToggle_);
	}
	depthWriteToggle_ = ref_ptr<ToggleDepthWriteState>::alloc(useDepthWrite);
	joinStates(depthWriteToggle_);
}

void DepthState::set_useDepthTest(GLboolean useDepthTest) {
	if (depthTestToggle_.get()) {
		disjoinStates(depthTestToggle_);
	}
	if (useDepthTest) {
		depthTestToggle_ = ref_ptr<ToggleState>::alloc(RenderState::DEPTH_TEST, GL_TRUE);
	} else {
		depthTestToggle_ = ref_ptr<ToggleState>::alloc(RenderState::DEPTH_TEST, GL_FALSE);
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

void DepthState::set_depthRange(GLdouble nearVal, GLdouble farVal) {
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
		auto range = input.getValue<Vec2f>("range", Vec2f(0.0f));
		depth->set_depthRange(range.x, range.y);
	}

	if (input.hasAttribute("function")) {
		depth->set_depthFunc(glenum::compareFunction(input.getValue("function")));
	}

	return depth;
}
