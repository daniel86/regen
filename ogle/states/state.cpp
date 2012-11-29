/*
 * state.cpp
 *
 *  Created on: 01.08.2012
 *      Author: daniel
 */

#include "state.h"
#include "shader-input-state.h"
#include <ogle/states/render-state.h>

static inline bool isShaderInputState(State *s)
{
  return dynamic_cast<ShaderInputState*>(s)!=NULL;
}

State::State()
: EventObject(),
  isHidden_(GL_FALSE)
{
}
StateSequence::StateSequence()
: State()
{
}

void State::shaderDefine(const string &name, const string &value)
{
  shaderDefines_[name] = value;
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
  if(isShaderInputState(s)) {
    ShaderInputState *inState = (ShaderInputState*)s;
    const list< ref_ptr<ShaderInput> > &in = inState->inputs();
    for(list< ref_ptr<ShaderInput> >::const_iterator
        it=in.begin(); it!=in.end(); ++it)
    {
      const ref_ptr<ShaderInput> &att = *it;
      att->set_isConstant(isConstant);
    }
  }
  for(list< ref_ptr<State> >::iterator
      it=s->joined().begin(); it!=s->joined().end(); ++it)
  {
    setConstantUniforms_(it->get(), isConstant);
  }
}
void State::setConstantUniforms(GLboolean isConstant)
{
  setConstantUniforms_(this, isConstant);
}

list< ref_ptr<State> >& State::joined()
{
  return joined_;
}

void State::configureShader(ShaderConfig *cfg)
{
  for(map<string,string>::iterator
      it=shaderDefines_.begin(); it!=shaderDefines_.end(); ++it)
  {
    cfg->define(it->first, it->second);
  }
  for(list< ref_ptr<State> >::iterator
      it=joined_.begin(); it!=joined_.end(); ++it)
  {
    (*it)->configureShader(cfg);
  }
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
void StateSequence::enable(RenderState *state)
{
  for(list< ref_ptr<State> >::iterator
      it=joined_.begin(); it!=joined_.end(); ++it)
  {
    (*it)->enable(state);
    (*it)->disable(state);
  }
}
void StateSequence::disable(RenderState *state)
{
}

void State::joinStates(ref_ptr<State> state)
{
  joined_.push_back(state);
}
void State::disjoinStates(ref_ptr<State> state)
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

void State::joinShaderInput(ref_ptr<ShaderInput> in)
{
  joinStates(ref_ptr<State>::manage(new ShaderInputState(in)));
}
void State::disjoinShaderInput(ref_ptr<ShaderInput> in)
{
  for(list< ref_ptr<State> >::iterator
      it=joined_.begin(); it!=joined_.end(); ++it)
  {
    State *state = it->get();
    ShaderInputState *inState = dynamic_cast<ShaderInputState*>(state);
    if(inState==NULL) { continue; }

    if(inState->hasInput(in->name())) {
      joined_.erase(it);
      return;
    }
  }
}
