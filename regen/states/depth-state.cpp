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
