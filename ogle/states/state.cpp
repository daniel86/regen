/*
 * state.cpp
 *
 *  Created on: 01.08.2012
 *      Author: daniel
 */

#include "state.h"
#include "shader-input-state.h"

State::State()
: EventObject()
{
}

list< ref_ptr<State> >& State::joined()
{
  return joined_;
}

void State::configureShader(ShaderConfiguration *cfg)
{
  for(list< ref_ptr<State> >::iterator
      it=joined_.begin(); it!=joined_.end(); ++it)
  {
    (*it)->configureShader(cfg);
  }
}

void State::update(GLfloat dt) {}

void State::enable(RenderState *state)
{
  for(list< ref_ptr<Callable> >::iterator
      it=enabler_.begin(); it!=enabler_.end(); ++it)
  {
    (*it)->call();
  }
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
  for(list< ref_ptr<Callable> >::reverse_iterator
      it=disabler_.rbegin(); it!=disabler_.rend(); ++it)
  {
    (*it)->call();
  }
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
    ShaderInputState *inState = dynamic_cast<ShaderInputState*>(inState);
    if(inState==NULL) { continue; }

    if(inState->hasInput(in->name())) {
      joined_.erase(it);
      return;
    }
  }
}

void State::addEnabler(ref_ptr<Callable> enabler)
{
  enabler_.push_back(enabler);
}
void State::addDisabler(ref_ptr<Callable> disabler)
{
  disabler_.push_back(disabler);
}

void State::removeEnabler(ref_ptr<Callable> enabler)
{
  for(list< ref_ptr<Callable> >::iterator
      it=enabler_.begin(); it!=enabler_.end(); ++it)
  {
    if(it->get() == enabler.get())
    {
      enabler_.erase(it);
      return;
    }
  }
}
void State::removeDisabler(ref_ptr<Callable> disabler)
{
  for(list< ref_ptr<Callable> >::iterator
      it=disabler_.begin(); it!=disabler_.end(); ++it)
  {
    if(it->get() == disabler.get())
    {
      disabler_.erase(it);
      return;
    }
  }
  disabler_.push_back(disabler);
}
