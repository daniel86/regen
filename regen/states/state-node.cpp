/*
 * state-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <stack>
#include <regen/animations/animation-manager.h>

#include "state-node.h"

using namespace regen;

StateNode::StateNode()
		: state_(ref_ptr<State>::alloc()),
		  parent_(nullptr),
		  isHidden_(GL_FALSE),
		  name_("Node") {
}

StateNode::StateNode(const ref_ptr<State> &state)
		: state_(state),
		  parent_(nullptr),
		  isHidden_(GL_FALSE),
		  name_("Node") {
}

GLboolean StateNode::hasParent() const { return parent_ != nullptr; }

void StateNode::clear() {
	while (!childs_.empty()) {
		removeChild(childs_.begin()->get());
	}
}

void StateNode::traverse(RenderState *rs) {
	if (!isHidden_ && !state_->isHidden()) {
		state_->enable(rs);

		for (auto &child: childs_) { child.get()->traverse(rs); }

		state_->disable(rs);
	}
}

void StateNode::addChild(const ref_ptr<StateNode> &child) {
	if (child->parent_ != nullptr) {
		child->parent_->removeChild(child.get());
	}
	childs_.push_back(child);
	child->set_parent(this);
}

void StateNode::addFirstChild(const ref_ptr<StateNode> &child) {
	if (child->parent_ != nullptr) {
		child->parent_->removeChild(child.get());
	}
	childs_.push_front(child);
	child->set_parent(this);
}

void StateNode::removeChild(StateNode *child) {
	for (auto it = childs_.begin(); it != childs_.end(); ++it) {
		if (it->get() == child) {
			child->set_parent(nullptr);
			childs_.erase(it);
			break;
		}
	}
}

static void getStateCamera(const ref_ptr<State> &state, ref_ptr<Camera> *out) {
	if (dynamic_cast<Camera *>(state.get()))
		*out = ref_ptr<Camera>::dynamicCast(state);
	auto it = state->joined().begin();

	while (out->get() == nullptr && it != state->joined().end()) {
		getStateCamera(*it, out);
		++it;
	}
}

ref_ptr<State> StateNode::getParentCamera() {
	ref_ptr<Camera> out;
	if (hasParent()) {
		getStateCamera(parent_->state(), &out);
		if (out.get() == nullptr)
			return parent_->getParentCamera();
	}
	return out;
}

StateNode *StateNode::findNodeWithName(const std::string &name) {
	std::stack<StateNode *> stack;
	stack.push(this);

	while (!stack.empty()) {
		auto node = stack.top();
		stack.pop();

		if (node->name_ == name) {
			return node;
		}

		for (auto &child: node->childs_) {
			stack.push(child.get());
		}
	}

	return nullptr;
}

//////////////
//////////////

RootNode::RootNode() : StateNode() {
}

void RootNode::init() {
}

void RootNode::render(GLdouble dt) {
	GL_ERROR_LOG();
	RenderState::get()->setDeltaTime(dt);
	traverse(RenderState::get());
	GL_ERROR_LOG();
}

void RootNode::postRender(GLdouble dt) {
	GL_ERROR_LOG();
	//AnimationManager::get().nextFrame();
	// some animations modify the vertex data,
	// updating the vbo needs a context so we do it here in the main thread..
	AnimationManager::get().updateGraphics(RenderState::get(), dt);
	// invoke event handler of queued events
	EventObject::emitQueued();
	GL_ERROR_LOG();
}

//////////////
//////////////

LoopNode::LoopNode(GLuint numIterations)
		: StateNode(),
		  numIterations_(numIterations) {}

LoopNode::LoopNode(const ref_ptr<State> &state, GLuint numIterations)
		: StateNode(state),
		  numIterations_(numIterations) {}

GLuint LoopNode::numIterations() const { return numIterations_; }

void LoopNode::set_numIterations(GLuint numIterations) { numIterations_ = numIterations; }

void LoopNode::traverse(RenderState *rs) {
	for (auto i = 0u; i < numIterations_; ++i) { StateNode::traverse(rs); }
}
