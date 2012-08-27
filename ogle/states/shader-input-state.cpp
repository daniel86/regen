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
ShaderInputState::ShaderInputState(ref_ptr<ShaderInput> in)
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

GLuint ShaderInputState::vertexBuffer() const
{
  ShaderInputIteratorConst it;
  for(it = inputs_.begin(); it != inputs_.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = *it;
    if(in->buffer()!=0)
    {
      return in->buffer();
    }
  }
  return 0;
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
ref_ptr<ShaderInput> ShaderInputState::getInputPtr(const string &name)
{
  for(list< ref_ptr<ShaderInput> >::iterator
      it = inputs_.begin(); it != inputs_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) return *it;
  }
  return ref_ptr<ShaderInput>();
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

list< ref_ptr<VertexAttribute> > ShaderInputState::interleavedAttributes()
{
  list< ref_ptr<VertexAttribute> > atts;
  for(list< ref_ptr<ShaderInput> >::iterator it = inputs_.begin();
      it != inputs_.end(); ++it)
  {
    ref_ptr<ShaderInput> &in = *it;
    if(in->numVertices()>1) {
      atts.push_back(ref_ptr<VertexAttribute>::cast(in));
    }
  }
  return atts;
}
list< ref_ptr<VertexAttribute> > ShaderInputState::sequentialAttributes()
{
  list< ref_ptr<VertexAttribute> > atts;
  for(list< ref_ptr<ShaderInput> >::iterator it = inputs_.begin();
      it != inputs_.end(); ++it)
  {
    ref_ptr<ShaderInput> &in = *it;
    if(in->numInstances()>1) {
      atts.push_back(ref_ptr<VertexAttribute>::cast(in));
    }
  }
  return atts;
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

  return inputs_.begin();
}

void ShaderInputState::removeInput(ref_ptr<ShaderInput> &in)
{
  inputMap_.erase(in->name());
  removeInput(in->name());
}

void ShaderInputState::removeInput(const string &name)
{
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
    shaderCfg->setShaderInput(*it);
  }
}
