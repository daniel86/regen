/*
 * shader-input-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "shader-input-state.h"

#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/vbo-state.h>

ShaderInputState::ShaderInputState()
: State()
{
}
ShaderInputState::ShaderInputState(ref_ptr<ShaderInput> &in)
: State()
{
  setInput(in);
}

string ShaderInputState::name()
{
  if(inputs_.size()==1) {
    return (*inputs_.begin())->name();
  } else {
    return FORMAT_STRING("ShaderInputState");
  }
}

GLboolean ShaderInputState::isBufferSet()
{
  ShaderInputIteratorConst it;
  for(it = inputs_.begin(); it != inputs_.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = *it;
    if((in->numVertices()>1 || in->numInstances()>1) && in->buffer()==0)
    {
      return false;
    }
  }
  return true;
}

void ShaderInputState::setBuffer(GLuint buffer)
{
  ShaderInputIteratorConst it;
  for(it = inputs_.begin(); it != inputs_.end(); ++it)
  {
    (*it)->set_buffer(buffer);
  }
}

ShaderInputIteratorConst ShaderInputState::getInput(const string &name) const
{
  ShaderInputIteratorConst it;
  for(it = inputs_.begin(); it != inputs_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) return it;
  }
  return it;
}
ShaderInput* ShaderInputState::getInputPtr(const string &name)
{
  for(list< ref_ptr<ShaderInput> >::iterator
      it = inputs_.begin(); it != inputs_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) return it->get();
  }
  return NULL;
}

bool ShaderInputState::hasInput(const string &name) const
{
  return inputMap_.count(name)>0;
}

list< ref_ptr<ShaderInput> >* ShaderInputState::inputsPtr()
{
  return &inputs_;
}
const list< ref_ptr<ShaderInput> >& ShaderInputState::inputs() const
{
  return inputs_;
}

const list< ref_ptr<VertexAttribute> >& ShaderInputState::interleavedAttributes()
{
  return interleavedAttributes_;
}
const list< ref_ptr<VertexAttribute> >& ShaderInputState::sequentialAttributes()
{
  return sequentialAttributes_;
}

ShaderInputIteratorConst ShaderInputState::setInput(ref_ptr<ShaderInput> in)
{
  if(inputMap_.count(in->name())>0) {
    removeInput(in->name());
  } else { // insert into map of known attributes
    inputMap_.insert(in->name());
  }
  in->set_buffer(0);

  inputs_.push_front(in);
  // input also could be uniform or constant,
  // only instanced attributes and attributes get added to VBO.
  if(in->isVertexAttribute())
  {
    if(in->divisor()==0) {
      interleavedAttributes_.push_back(ref_ptr<VertexAttribute>::cast(in));
    } else {
      sequentialAttributes_.push_back(ref_ptr<VertexAttribute>::cast(in));
    }
  }

  return inputs_.begin();
}

void ShaderInputState::removeInput(ref_ptr<ShaderInput> &in)
{
  inputMap_.erase(in->name());
  removeInput(in->name());
}

void ShaderInputState::removeInput(const string &name)
{
  for(list< ref_ptr<VertexAttribute> >::iterator it = interleavedAttributes_.begin();
      it != interleavedAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      interleavedAttributes_.erase(it);
      break;
    }
  }
  for(list< ref_ptr<VertexAttribute> >::iterator it = sequentialAttributes_.begin();
      it != sequentialAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      sequentialAttributes_.erase(it);
      break;
    }
  }
  for(list< ref_ptr<ShaderInput> >::iterator it = inputs_.begin();
      it != inputs_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      inputs_.erase(it);
      break;
    }
  }
}

//////////

void ShaderInputState::enable(RenderState *state)
{
  State::enable(state);
  for(list< ref_ptr<ShaderInput> >::iterator
      it=inputs_.begin(); it!=inputs_.end(); ++it)
  {
    state->pushShaderInput(it->get());
  }
}

void ShaderInputState::disable(RenderState *state)
{
  for(list< ref_ptr<ShaderInput> >::iterator
      it=inputs_.begin(); it!=inputs_.end(); ++it)
  {
    state->popShaderInput((*it)->name());
  }
  State::disable(state);
}

void ShaderInputState::configureShader(ShaderConfiguration *shaderCfg)
{
  State::configureShader(shaderCfg);
  for(list< ref_ptr<ShaderInput> >::iterator
      it=inputs_.begin(); it!=inputs_.end(); ++it)
  {
    shaderCfg->setShaderInput(it->get());
  }
}
