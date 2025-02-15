/*
 * state-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <regen/animations/animation-manager.h>

#include "state-node.h"

using namespace regen;

NodeEyeDepthComparator::NodeEyeDepthComparator(
		const ref_ptr<Camera> &cam, GLboolean frontToBack)
		: cam_(cam),
		  mode_(((GLint) frontToBack) * 2 - 1) {
}

GLfloat NodeEyeDepthComparator::getEyeDepth(const Vec3f &p) const {
	auto mat = cam_->view()->getVertex(0);
	return mat.r.x[2] * p.x + mat.r.x[6] * p.y + mat.r.x[10] * p.z + mat.r.x[14];
}

ModelTransformation *NodeEyeDepthComparator::findModelTransformation(StateNode *n) const {
	State *nodeState = n->state().get();
	auto *ret = dynamic_cast<ModelTransformation *>(nodeState);
	if (ret != nullptr) { return ret; }

	for (auto it = nodeState->joined().begin(); it != nodeState->joined().end(); ++it) {
		auto *ret = dynamic_cast<ModelTransformation *>(it->get());
		if (ret != nullptr) { return ret; }
	}

	for (auto it = n->childs().begin(); it != n->childs().end(); ++it) {
		ret = findModelTransformation(it->get());
		if (ret != nullptr) { return ret; }
	}

	return nullptr;
}

bool NodeEyeDepthComparator::operator()(ref_ptr<StateNode> &n0, ref_ptr<StateNode> &n1) const {
	ModelTransformation *modelMat0 = findModelTransformation(n0.get());
	ModelTransformation *modelMat1 = findModelTransformation(n1.get());
	if (modelMat0 != nullptr && modelMat1 != nullptr) {
		GLfloat diff = mode_ * (
				getEyeDepth(modelMat0->get()->getVertex(0).r.position()) -
				getEyeDepth(modelMat1->get()->getVertex(0).r.position()));
		return diff < 0;
	} else if (modelMat0 != nullptr) {
		return true;
	} else if (modelMat1 != nullptr) {
		return false;
	} else {
		return n0 < n1;
	}
}

/////////

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

const std::string &StateNode::name() const { return name_; }

void StateNode::set_name(const std::string &name) { name_ = name; }

GLboolean StateNode::isHidden() const { return isHidden_; }

void StateNode::set_isHidden(GLboolean isHidden) { isHidden_ = isHidden; }

const ref_ptr<State> &StateNode::state() const { return state_; }

void StateNode::set_parent(StateNode *parent) { parent_ = parent; }

StateNode *StateNode::parent() const { return parent_; }

GLboolean StateNode::hasParent() const { return parent_ != nullptr; }

void StateNode::clear() {
	while (!childs_.empty()) {
		removeChild(childs_.begin()->get());
	}
}

void StateNode::traverse(RenderState *rs) {
	if (!isHidden_ && !state_->isHidden()) {
		state_->enable(rs);

		for (auto it = childs_.begin(); it != childs_.end(); ++it) { it->get()->traverse(rs); }

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

std::list<ref_ptr<StateNode> > &StateNode::childs() { return childs_; }

static void getStateCamera(const ref_ptr<State> &state, ref_ptr<Camera> *out) {
	// TODO ref_ptr::dynamicCast
	if (dynamic_cast<Camera *>(state.get()))
		*out = ref_ptr<Camera>::dynamicCast(state);
	auto it = state->joined().begin();

	while (out->get() == NULL && it != state->joined().end()) {
		getStateCamera(*it, out);
		++it;
	}
}

ref_ptr<Camera> StateNode::getParentCamera() {
	ref_ptr<Camera> out;
	if (hasParent()) {
		getStateCamera(parent_->state(), &out);
		if (out.get() == NULL)
			return parent_->getParentCamera();
	}
	return out;
}

//////////////
//////////////

RootNode::RootNode() : StateNode() {
}

void RootNode::init() {
}

void RootNode::render(GLdouble dt) {
	GL_ERROR_LOG();
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
