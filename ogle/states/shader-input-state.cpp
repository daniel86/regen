/*
 * shader-input-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <ogle/utility/gl-util.h>
#include <ogle/utility/string-util.h>
#include <ogle/gl-types/vbo-manager.h>

#include "shader-input-state.h"
using namespace ogle;

ShaderInputState::ShaderInputState()
: State(), useVBOManager_(GL_TRUE)
{}
ShaderInputState::ShaderInputState(const ref_ptr<ShaderInput> &in, const string &name)
: State()
{ setInput(in,name); }

ShaderInputState::~ShaderInputState()
{
  while(!inputs_.empty())
  { removeInput(inputs_.begin()->name_); }
}

void ShaderInputState::set_useVBOManager(GLboolean v)
{
  useVBOManager_ = v;
}

ref_ptr<ShaderInput> ShaderInputState::getInput(const string &name)
{
  for(InputIt it=inputs_.begin(); it!=inputs_.end(); ++it)
  {
    if(name.compare(it->name_) == 0) return it->in_;
  }
  return ref_ptr<ShaderInput>();
}

GLboolean ShaderInputState::hasInput(const string &name) const
{
  return inputMap_.count(name)>0;
}

ShaderInputState::InputContainer* ShaderInputState::inputsPtr()
{
  return &inputs_;
}
const ShaderInputState::InputContainer& ShaderInputState::inputs() const
{
  return inputs_;
}

ShaderInputState::InputItConst ShaderInputState::setInput(const ref_ptr<ShaderInput> &in, const string &name)
{
  string inputName = (name.empty() ? in->name() : name);

  if(inputMap_.count(inputName)>0) {
    removeInput(inputName);
  } else { // insert into map of known attributes
    inputMap_.insert(inputName);
  }

  inputs_.push_front(Named(in, inputName));

  shaderDefine(FORMAT_STRING("HAS_"<<in->name()), "TRUE");
  if(in->numInstances()>1)
  { shaderDefine("HAS_INSTANCES", "TRUE"); }

  if(in->isVertexAttribute() && useVBOManager_)
  { VBOManager::add(ref_ptr<VertexAttribute>::cast(in)); }

  return inputs_.begin();
}

void ShaderInputState::removeInput(const ref_ptr<ShaderInput> &in)
{
  inputMap_.erase(in->name());
  removeInput(in->name());
}

void ShaderInputState::removeInput(const string &name)
{
  InputIt it;
  for(it=inputs_.begin(); it!=inputs_.end(); ++it) {
    if(it->name_ == name) { break; }
  }
  if(it==inputs_.end()) { return; }
  inputs_.erase(it);
}
