/*
 * state.cpp
 *
 *  Created on: 01.08.2012
 *      Author: daniel
 */

#include <regen/gl-types/input-container.h>
#include <regen/scene/loading-context.h>

#include "state.h"

using namespace regen;

State::State()
		: EventObject(),
		  shaderVersion_(130) {
	isHidden_ = ref_ptr<ShaderInput1i>::alloc("isHidden");
	isHidden_->setUniformData(0);
}

State::State(const ref_ptr<State> &other)
		: EventObject(),
		  shaderDefines_(other->shaderDefines_),
		  shaderFunctions_(other->shaderFunctions_),
		  joined_(other->joined_),
		  inputStateBuddy_(other->inputStateBuddy_),
		  isHidden_(other->isHidden_),
		  shaderVersion_(other->shaderVersion_) {
}

GLuint State::shaderVersion() const { return shaderVersion_; }

void State::setShaderVersion(GLuint version) {
	if (version > shaderVersion_) shaderVersion_ = version;
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

const std::map<std::string, std::string> &State::shaderDefines() const {
	return shaderDefines_;
}

void State::shaderInclude(const std::string &path) {
	shaderIncludes_.push_back(path);
}

void State::shaderFunction(const std::string &name, const std::string &value) { shaderFunctions_[name] = value; }

const std::map<std::string, std::string> &State::shaderFunctions() const { return shaderFunctions_; }

GLboolean State::isHidden() const {
	return isHidden_->getVertex(0).r;
}

void State::set_isHidden(GLboolean isHidden) {
	isHidden_->setVertex(0, isHidden);
}

static void setConstantUniforms_(State *s, GLboolean isConstant) {
	auto *inState = dynamic_cast<HasInput *>(s);
	if (inState) {
		const ShaderInputList &in = inState->inputContainer()->inputs();
		for (const auto &it: in) {
			it.in_->set_isConstant(isConstant);
		}
	}
	for (const auto &it: s->joined()) {
		setConstantUniforms_(it.get(), isConstant);
	}
}

void State::setConstantUniforms(GLboolean isConstant) {
	setConstantUniforms_(this, isConstant);
}

const std::list<ref_ptr<State> > &State::joined() const { return joined_; }

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

void State::joinStates(const ref_ptr<State> &state) { joined_.push_back(state); }

void State::joinStatesFront(const ref_ptr<State> &state) { joined_.push_front(state); }

void State::disjoinStates(const ref_ptr<State> &state) {
	for (auto it = joined_.begin(); it != joined_.end(); ++it) {
		if (it->get() == state.get()) {
			joined_.erase(it);
			return;
		}
	}
}

void State::joinShaderInput(const ref_ptr<ShaderInput> &in, const std::string &name) {
	auto *inState = dynamic_cast<HasInput *>(this);
	if (inputStateBuddy_.get()) { inState = inputStateBuddy_.get(); }
	else if (!inState) {
		ref_ptr<HasInputState> inputState = ref_ptr<HasInputState>::alloc();
		inState = inputState.get();
		joinStatesFront(inputState);
		inputStateBuddy_ = inputState;
	}
	inState->setInput(in, name);
}

void State::disjoinShaderInput(const ref_ptr<ShaderInput> &in) {
	auto *inState = dynamic_cast<HasInput *>(this);
	if (inputStateBuddy_.get()) { inState = inputStateBuddy_.get(); }
	else if (!inState) return;
	inState->inputContainer()->removeInput(in);
}

void State::collectShaderInput(ShaderInputList &out) {
	auto *inState = dynamic_cast<HasInput *>(this);
	if (inState != nullptr) {
		const ref_ptr<InputContainer> &container = inState->inputContainer();
		out.insert(out.end(), container->inputs().begin(), container->inputs().end());
	}

	for (auto it = joined_.begin(); it != joined_.end(); ++it) { (*it)->collectShaderInput(out); }
}

std::optional<StateInput> State::findShaderInput(const std::string &name) {
	StateInput ret;

	auto *inState = dynamic_cast<HasInput *>(this);
	if (inState != nullptr) {
		ret.container = inState->inputContainer();
		auto &l = ret.container->inputs();
		for (const auto &inNamed: l) {
			if (name == inNamed.name_ || name == inNamed.in_->name()) {
				ret.in = inNamed.in_;
				ret.block = {};
				return ret;
			}
			if (inNamed.in_->isBufferBlock()) {
				auto block = ref_ptr<BufferBlock>::dynamicCast(inNamed.in_);
				for (auto &blockUniform: block->blockInputs()) {
					if (name == blockUniform.name_ || name == blockUniform.in_->name()) {
						ret.block = block;
						ret.in = blockUniform.in_;
						return ret;
					}
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

//////////
//////////

StateSequence::StateSequence() : State() {
	globalState_ = ref_ptr<State>::alloc();
}

void StateSequence::set_globalState(const ref_ptr<State> &globalState) { globalState_ = globalState; }

const ref_ptr<State> &StateSequence::globalState() const { return globalState_; }

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
