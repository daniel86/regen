/*
 * state.cpp
 *
 *  Created on: 01.08.2012
 *      Author: daniel
 */

#include <regen/gl-types/shader-input-container.h>

#include "state.h"

using namespace regen;

State::State()
		: EventObject(),
		  isHidden_(GL_FALSE),
		  shaderVersion_(130) {
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

void State::shaderDefine(const std::string &name, const std::string &value) { shaderDefines_[name] = value; }

const std::map<std::string, std::string> &State::shaderDefines() const { return shaderDefines_; }

void State::shaderFunction(const std::string &name, const std::string &value) { shaderFunctions_[name] = value; }

const std::map<std::string, std::string> &State::shaderFunctions() const { return shaderFunctions_; }

GLboolean State::isHidden() const { return isHidden_; }

void State::set_isHidden(GLboolean isHidden) { isHidden_ = isHidden; }

static void setConstantUniforms_(State *s, GLboolean isConstant) {
	HasInput *inState = dynamic_cast<HasInput *>(s);
	if (inState) {
		const ShaderInputList &in = inState->inputContainer()->inputs();
		for (ShaderInputList::const_iterator it = in.begin(); it != in.end(); ++it) {
			it->in_->set_isConstant(isConstant);
		}
	}
	for (std::list<ref_ptr<State> >::const_iterator
				 it = s->joined().begin(); it != s->joined().end(); ++it) {
		setConstantUniforms_(it->get(), isConstant);
	}
}

void State::setConstantUniforms(GLboolean isConstant) {
	setConstantUniforms_(this, isConstant);
}

const std::list<ref_ptr<State> > &State::joined() const { return joined_; }

void State::enable(RenderState *state) {
	for (std::list<ref_ptr<State> >::iterator
				 it = joined_.begin(); it != joined_.end(); ++it) {
		if (!(*it)->isHidden()) (*it)->enable(state);
	}
}

void State::disable(RenderState *state) {
	for (std::list<ref_ptr<State> >::reverse_iterator
				 it = joined_.rbegin(); it != joined_.rend(); ++it) {
		if (!(*it)->isHidden()) (*it)->disable(state);
	}
}

void State::attach(const ref_ptr<EventObject> &obj) {
	attached_.push_back(obj);
}

void State::joinStates(const ref_ptr<State> &state) { joined_.push_back(state); }

void State::joinStatesFront(const ref_ptr<State> &state) { joined_.push_front(state); }

void State::disjoinStates(const ref_ptr<State> &state) {
	for (std::list<ref_ptr<State> >::iterator
				 it = joined_.begin(); it != joined_.end(); ++it) {
		if (it->get() == state.get()) {
			joined_.erase(it);
			return;
		}
	}
}

void State::joinShaderInput(const ref_ptr<ShaderInput> &in, const std::string &name) {
	HasInput *inState = dynamic_cast<HasInput *>(this);
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
	HasInput *inState = dynamic_cast<HasInput *>(this);
	if (inputStateBuddy_.get()) { inState = inputStateBuddy_.get(); }
	else if (!inState) return;
	inState->inputContainer()->removeInput(in);
}

void State::collectShaderInput(ShaderInputList &out) {
	HasInput *inState = dynamic_cast<HasInput *>(this);
	if (inState != NULL) {
		const ref_ptr<ShaderInputContainer> &container = inState->inputContainer();
		out.insert(out.end(), container->inputs().begin(), container->inputs().end());
	}

	for (std::list<ref_ptr<State> >::iterator
				 it = joined_.begin(); it != joined_.end(); ++it) { (*it)->collectShaderInput(out); }
}

ref_ptr<ShaderInput> State::findShaderInput(const std::string &name) {
	ref_ptr<ShaderInput> ret;

	HasInput *inState = dynamic_cast<HasInput *>(this);
	if (inState != NULL) {
		const ShaderInputList &l = inState->inputContainer()->inputs();
		for (ShaderInputList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const NamedShaderInput &inNamed = *it;
			if (name == inNamed.name_ ||
				name == inNamed.in_->name())
				return inNamed.in_;
		}
	}

	for (std::list<ref_ptr<State> >::const_iterator
				 it = joined().begin(); it != joined().end(); ++it) {
		ret = (*it)->findShaderInput(name);
		if (ret.get() != NULL) break;
	}

	return ret;
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
	for (std::list<ref_ptr<State> >::iterator
				 it = joined_.begin(); it != joined_.end(); ++it) {
		(*it)->enable(state);
		(*it)->disable(state);
	}
	globalState_->disable(state);
}

void StateSequence::disable(RenderState *state) {}
