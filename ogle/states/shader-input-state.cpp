/*
 * shader-input-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "shader-input-state.h"

#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/gl-types/vbo-manager.h>
#include <ogle/states/render-state.h>

ShaderInputState::ShaderInputState()
: State(),
  useVBOManager_(GL_TRUE)
{
}
ShaderInputState::ShaderInputState(const ref_ptr<ShaderInput> &in)
: State()
{
  setInput(in);
}
ShaderInputState::~ShaderInputState()
{
  while(!inputs_.empty())
  {
    removeInput(*inputs_.begin());
  }
}

void ShaderInputState::set_useVBOManager(GLboolean v)
{
  useVBOManager_ = v;
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

GLboolean ShaderInputState::hasInput(const string &name) const
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

ShaderInputIteratorConst ShaderInputState::setInput(const ref_ptr<ShaderInput> &in)
{
  if(inputMap_.count(in->name())>0) {
    removeInput(in->name());
  } else { // insert into map of known attributes
    inputMap_.insert(in->name());
  }

  inputs_.push_front(in);

  shaderDefine(FORMAT_STRING("HAS_"<<in->name()), "TRUE");
  if(in->numInstances()>1) {
    shaderDefine("HAS_INSTANCES", "TRUE");
  }

  if(in->isVertexAttribute() && useVBOManager_) {
    VBOManager::add(ref_ptr<VertexAttribute>::cast(in));
  }

  return inputs_.begin();
}

void ShaderInputState::removeInput(const ref_ptr<ShaderInput> &in)
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
      VBOManager::remove(ref_ptr<VertexAttribute>::cast(*it));
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
