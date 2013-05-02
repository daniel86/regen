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
  shaderVersion_(130)
{
}

GLuint State::shaderVersion() const
{
  return shaderVersion_;
}
void State::setShaderVersion(GLuint version)
{
  if(version>shaderVersion_) shaderVersion_=version;
}

void State::shaderDefine(const string &name, const string &value)
{
  shaderDefines_[name] = value;
}
const map<string,string>& State::shaderDefines() const
{
  return shaderDefines_;
}

void State::shaderFunction(const string &name, const string &value)
{
  shaderFunctions_[name] = value;
}
const map<string,string>& State::shaderFunctions() const
{
  return shaderFunctions_;
}

GLboolean State::isHidden() const
{
  return isHidden_;
}
void State::set_isHidden(GLboolean isHidden)
{
  isHidden_ = isHidden;
}

static void setConstantUniforms_(State *s, GLboolean isConstant)
{
  HasInput *inState = dynamic_cast<HasInput*>(s);
  if(inState) {
    const ShaderInputList &in = inState->inputContainer()->inputs();
    for(ShaderInputList::const_iterator it=in.begin(); it!=in.end(); ++it)
    { it->in_->set_isConstant(isConstant); }
  }
  for(list< ref_ptr<State> >::const_iterator
      it=s->joined().begin(); it!=s->joined().end(); ++it)
  {
    setConstantUniforms_(it->get(), isConstant);
  }
}
void State::setConstantUniforms(GLboolean isConstant)
{
  setConstantUniforms_(this, isConstant);
}

const list< ref_ptr<State> >& State::joined() const
{
  return joined_;
}

void State::enable(RenderState *state)
{
  for(list< ref_ptr<State> >::iterator
      it=joined_.begin(); it!=joined_.end(); ++it)
  {
    (*it)->enable(state);
  }
}
void State::disable(RenderState *state)
{
  for(list< ref_ptr<State> >::reverse_iterator
      it=joined_.rbegin(); it!=joined_.rend(); ++it)
  {
    (*it)->disable(state);
  }
}

void State::joinStates(const ref_ptr<State> &state)
{
  joined_.push_back(state);
}
void State::joinStatesFront(const ref_ptr<State> &state)
{
  joined_.push_front(state);
}
void State::disjoinStates(const ref_ptr<State> &state)
{
  for(list< ref_ptr<State> >::iterator
      it=joined_.begin(); it!=joined_.end(); ++it)
  {
    if(it->get() == state.get())
    {
      joined_.erase(it);
      return;
    }
  }
}

void State::joinShaderInput(const ref_ptr<ShaderInput> &in, const string &name)
{
  HasInput *inState = dynamic_cast<HasInput*>(this);
  if(inputStateBuddy_.get())
  { inState = inputStateBuddy_.get(); }
  else if(!inState) {
    ref_ptr<HasInputState> inputState = ref_ptr<HasInputState>::manage(new HasInputState);
    inState = inputState.get();
    joinStatesFront(inputState);
    inputStateBuddy_ = inputState;
  }
  inState->setInput(in,name);
}
void State::disjoinShaderInput(const ref_ptr<ShaderInput> &in)
{
  HasInput *inState = dynamic_cast<HasInput*>(this);
  if(inputStateBuddy_.get())
  { inState = inputStateBuddy_.get(); }
  else if(!inState) return;
  inState->inputContainer()->removeInput(in);
}

//////////
//////////

StateSequence::StateSequence()
: State()
{
  globalState_ = ref_ptr<State>::manage(new State);
}

void StateSequence::set_globalState(const ref_ptr<State> &globalState)
{
  globalState_ = globalState;
}
const ref_ptr<State>& StateSequence::globalState()
{
  return globalState_;
}

void StateSequence::enable(RenderState *state)
{
  globalState_->enable(state);
  for(list< ref_ptr<State> >::iterator
      it=joined_.begin(); it!=joined_.end(); ++it)
  {
    (*it)->enable(state);
    (*it)->disable(state);
  }
  globalState_->disable(state);
}
void StateSequence::disable(RenderState *state)
{
}
