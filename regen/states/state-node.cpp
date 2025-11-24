#include <stack>
#include <regen/animation/animation-manager.h>

#include "state-node.h"
#include "regen/textures/fbo-state.h"

using namespace regen;

StateNode::StateNode()
		: state_(ref_ptr<State>::alloc()),
		  parent_(nullptr),
		  isHidden_(false),
		  name_("Node") {
}

StateNode::StateNode(const ref_ptr<State> &state)
		: state_(state),
		  parent_(nullptr),
		  isHidden_(false),
		  name_("Node") {
}

bool StateNode::hasParent() const { return parent_ != nullptr; }

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
	auto joined = state->joined();
	auto it = joined->begin();

	while (out->get() == nullptr && it != joined->end()) {
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

static void getFrameBufferState(const ref_ptr<State> &state, ref_ptr<FBOState> *out) {
	if (dynamic_cast<FBOState *>(state.get()))
		*out = ref_ptr<FBOState>::dynamicCast(state);
	auto joined = state->joined();
	auto it = joined->begin();

	while (out->get() == nullptr && it != joined->end()) {
		getFrameBufferState(*it, out);
		++it;
	}
}

ref_ptr<State> StateNode::getParentFrameBuffer() {
	ref_ptr<FBOState> out;
	if (hasParent()) {
		getFrameBufferState(parent_->state(), &out);
		if (out.get() == nullptr)
			return parent_->getParentFrameBuffer();
	}
	return out;
}

StateNode *StateNode::findNodeWithName(std::string_view name) {
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

void RootNode::render(float /* dt */) {
	auto rs = RenderState::get();
	traverse(rs);
}

void RootNode::postRender(double dt) {
	//AnimationManager::get().nextFrame();
	// some animations modify the vertex data,
	// updating the vbo needs a context so we do it here in the main thread..
	AnimationManager::get().gpuUpdateStep(dt);
}

//////////////
//////////////

LoopNode::LoopNode(uint32_t numIterations)
		: StateNode(),
		  numIterations_(numIterations) {}

LoopNode::LoopNode(const ref_ptr<State> &state, uint32_t numIterations)
		: StateNode(state),
		  numIterations_(numIterations) {}

uint32_t LoopNode::numIterations() const { return numIterations_; }

void LoopNode::set_numIterations(uint32_t numIterations) { numIterations_ = numIterations; }

void LoopNode::traverse(RenderState *rs) {
	for (auto i = 0u; i < numIterations_; ++i) { StateNode::traverse(rs); }
}
