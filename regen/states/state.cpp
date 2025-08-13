#include <regen/scene/loading-context.h>

#include "state.h"

using namespace regen;

struct State::StateShared {
	int32_t numVertices_ = 0;
	int32_t numInstances_ = 1;
};

State::State()
		: EventObject(),
		  Resource() {
	shared_ = ref_ptr<StateShared>::alloc();
}

State::State(const ref_ptr<State> &other)
		: EventObject(),
		  joined_(other->joined_),
		  attached_(other->attached_),
		  isHidden_(other->isHidden_),
		  shaderDefines_(other->shaderDefines_),
		  shaderIncludes_(other->shaderIncludes_),
		  shaderFunctions_(other->shaderFunctions_),
		  shaderVersion_(other->shaderVersion_),
		  shared_(other->shared_) {
	inputs_ = other->inputs_;
	inputMap_ = other->inputMap_;
}

State::~State() {
	while (!inputs_.empty()) {
		removeInput(inputs_.begin()->name_);
	}
	attached_.clear();
	joined_.clear();
}

const std::vector<NamedShaderInput> &State::inputs() const {
	return inputs_;
}

int32_t State::numVertices() const {
	return shared_->numVertices_;
}

void State::set_numVertices(int32_t v) {
	shared_->numVertices_ = v;
}

int32_t State::numInstances() const {
	return shared_->numInstances_;
}

void State::set_numInstances(int32_t v) {
	shared_->numInstances_ = v;
}

void State::setConstantUniforms(bool isConstant) {
	for (const auto &it: inputs_) {
		it.in_->set_isConstant(isConstant);
	}
	for (const auto &it: joined()) {
		it->setConstantUniforms(isConstant);
	}
}

void State::enable(RenderState *state) {
	for (auto &it: joined_) {
		if (!it->isHidden()) it->enable(state);
	}
}

void State::disable(RenderState *state) {
	for (auto it = joined_.rbegin(); it != joined_.rend(); ++it) {
		if (!(*it)->isHidden()) (*it)->disable(state);
	}
}

void State::attach(const ref_ptr<EventObject> &obj) {
	attached_.push_back(obj);
}

void State::joinStates(const ref_ptr<State> &state) {
	joined_.push_back(state);
}

void State::joinStatesFront(const ref_ptr<State> &state) {
	joined_.insert(joined_.begin(), state);
}

void State::disjoinStates(const ref_ptr<State> &state) {
	for (auto it = joined_.begin(); it != joined_.end(); ++it) {
		if (it->get() == state.get()) {
			joined_.erase(it);
			return;
		}
	}
}

bool State::hasInput(const std::string &name) const {
	return inputMap_.count(name) > 0;
}

ref_ptr<ShaderInput> State::getInput(const std::string &name) const {
	for (const auto &input: inputs_) {
		if (name == input.name_) return input.in_;
	}
	return {};
}

void State::setInput(const ref_ptr<ShaderInput> &in, const std::string &name, const std::string &memberSuffix) {
	const std::string &inputName = (name.empty() ? in->name() : name);

	if (in->isVertexAttribute() && in->numVertices() > static_cast<uint32_t>(shared_->numVertices_)) {
		shared_->numVertices_ = static_cast<int>(in->numVertices());
	}
	if (in->numInstances() > 1) {
		shared_->numInstances_ = static_cast<int>(in->numInstances());
	}
	// check for instances of attributes within UBO
	if (in->isBufferBlock()) {
		auto *block = dynamic_cast<BufferBlock *>(in.get());
		for (auto &namedInput: block->stagedInputs()) {
			if (namedInput.in_->isVertexAttribute() &&
			    namedInput.in_->numVertices() > static_cast<uint32_t>(shared_->numVertices_)) {
				shared_->numVertices_ = static_cast<int>(namedInput.in_->numVertices());
			}
			if (namedInput.in_->numInstances() > 1) {
				shared_->numInstances_ = static_cast<int>(namedInput.in_->numInstances());
			}
		}
	}

	if (inputMap_.count(inputName) > 0) {
		removeInput(inputName);
	} else { // insert into map of known attributes
		inputMap_.insert(inputName);
	}

	// TODO: Rather push back here. But it seems some code relies on the order of inputs.
	//       This should be fixed in the future.
	inputs_.insert(inputs_.begin(), NamedShaderInput{in, inputName, "", memberSuffix});
}

void State::removeInput(const ref_ptr<ShaderInput> &in) {
	inputMap_.erase(in->name());
	removeInput(in->name());
}

void State::removeInput(const std::string &name) {
	std::vector<NamedShaderInput>::iterator it;
	for (it = inputs_.begin(); it != inputs_.end(); ++it) {
		if (it->name_ == name) { break; }
	}
	if (it == inputs_.end()) { return; }
	//it->in_->set_buffer(0u, {});
	inputs_.erase(it);
}

void State::collectShaderInput(ShaderInputList &out) {
	out.insert(out.end(), inputs_.begin(), inputs_.end());
	for (auto &buddy : joined_) { buddy->collectShaderInput(out); }
}

std::optional<StateInput> State::findShaderInput(const std::string &name) {
	StateInput ret;

	auto &l = inputs();
	for (const auto &inNamed: l) {
		if (name == inNamed.name_ || name == inNamed.in_->name()) {
			ret.in = inNamed.in_;
			ret.block = {};
			return ret;
		}
		if (inNamed.in_->isBufferBlock()) {
			auto block = ref_ptr<BufferBlock>::dynamicCast(inNamed.in_);
			for (auto &blockUniform: block->stagedInputs()) {
				if (name == blockUniform.name_ || name == blockUniform.in_->name()) {
					ret.block = block;
					ret.in = blockUniform.in_;
					return ret;
				}
			}
		}
	}

	for (auto &joined: joined_) {
		auto joinedRet = joined->findShaderInput(name);
		if (joinedRet.has_value()) {
			return joinedRet.value();
		}
	}

	return std::nullopt;
}

void State::shaderDefine(const std::string &name, const std::string &value) {
	shaderDefines_[name] = value;
}

void State::shaderUndefine(const std::string &name) {
	auto it = shaderDefines_.find(name);
	if (it != shaderDefines_.end()) {
		shaderDefines_.erase(it);
	}
}

void State::shaderInclude(const std::string &path) {
	shaderIncludes_.push_back(path);
}

void State::shaderFunction(const std::string &name, const std::string &value) {
	shaderFunctions_[name] = value;
}

//////////
//////////

StateSequence::StateSequence() : State() {
	globalState_ = ref_ptr<State>::alloc();
}

void StateSequence::set_globalState(const ref_ptr<State> &globalState) { globalState_ = globalState; }

void StateSequence::enable(RenderState *state) {
	globalState_->enable(state);
	for (auto &it: joined_) {
		it->enable(state);
		it->disable(state);
	}
	globalState_->disable(state);
}

void StateSequence::disable(RenderState *state) {}

ref_ptr<StateSequence> StateSequence::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<StateSequence> seq = ref_ptr<StateSequence>::alloc();
	// all states allowed as children
	for (auto &n: input.getChildren()) {
		auto processor = ctx.scene()->getStateProcessor(n->getCategory());
		if (processor.get() == nullptr) {
			REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
			return {};
		}
		processor->processInput(ctx.scene(), *n.get(), ctx.parent(), seq);
	}
	return seq;
}
