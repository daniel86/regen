/*
 * shader-input-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>

#include "shader-input-state.h"
using namespace regen;

ShaderInputState::ShaderInputState(GLboolean useAutoUpload, VertexBufferObject::Usage usage)
: State(), numVertices_(0), numInstances_(1), useAutoUpload_(useAutoUpload)
{
  inputBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(usage));
}
ShaderInputState::ShaderInputState(
    const ref_ptr<ShaderInput> &in, const string &name,
    GLboolean useAutoUpload, VertexBufferObject::Usage usage)
: State(), numVertices_(0), numInstances_(1), useAutoUpload_(useAutoUpload)
{
  inputBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(usage));
  setInput(in,name);
}

ShaderInputState::~ShaderInputState()
{
  while(!inputs_.empty())
  { removeInput(inputs_.begin()->name_); }
}

VertexBufferObject& ShaderInputState::inputBuffer() const
{ return *inputBuffer_.get(); }

GLuint ShaderInputState::numVertices() const
{ return numVertices_; }
GLuint ShaderInputState::numInstances() const
{ return numInstances_; }

GLboolean ShaderInputState::useAutoUpload() const
{ return useAutoUpload_; }

ref_ptr<ShaderInput> ShaderInputState::getInput(const string &name) const
{
  for(InputItConst it=inputs_.begin(); it!=inputs_.end(); ++it)
  {
    if(name.compare(it->name_) == 0) return it->in_;
  }
  return ref_ptr<ShaderInput>();
}

GLboolean ShaderInputState::hasInput(const string &name) const
{ return inputMap_.count(name)>0; }
const ShaderInputState::InputContainer& ShaderInputState::inputs() const
{ return inputs_; }

ShaderInputState::InputItConst ShaderInputState::setInput(
    const ref_ptr<ShaderInput> &in, const string &name)
{
  string inputName = (name.empty() ? in->name() : name);

  if(in->numVertices()>1)
  { numVertices_ = in->numVertices(); }
  if(in->numInstances()>1)
  { numInstances_ = in->numInstances(); }

  if(inputMap_.count(inputName)>0) {
    removeInput(inputName);
  } else { // insert into map of known attributes
    inputMap_.insert(inputName);
  }

  inputs_.push_front(Named(in, inputName));

  shaderDefine(FORMAT_STRING("HAS_"<<in->name()), "TRUE");
  if(in->numInstances()>1)
  { shaderDefine("HAS_INSTANCES", "TRUE"); }

  if(in->isVertexAttribute() && useAutoUpload_)
  { inputBuffer_->alloc(in); }

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

  if(it->in_->isVertexAttribute() && useAutoUpload_)
  { inputBuffer_->free(it->in_->bufferIterator().get()); }

  inputs_.erase(it);
}
