/*
 * attribute-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "attribute-state.h"

#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/vbo-state.h>

AttributeState::AttributeState()
: State()
{
}

string AttributeState::name()
{
  return FORMAT_STRING("AttributeState");
}

GLboolean AttributeState::isBufferSet()
{
  AttributeIteratorConst it;
  for(it = attributes_.begin(); it != attributes_.end(); ++it)
  {
    if((*it)->buffer()==0) { return false; }
  }
  return true;
}

void AttributeState::setBuffer(GLuint buffer)
{
  AttributeIteratorConst it;
  for(it = attributes_.begin(); it != attributes_.end(); ++it)
  {
    (*it)->set_buffer(buffer);
  }
}

AttributeIteratorConst AttributeState::getAttribute(const string &name) const
{
  AttributeIteratorConst it;
  for(it = attributes_.begin(); it != attributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it;
  }
  return it;
}
VertexAttribute* AttributeState::getAttributePtr(const string &name)
{
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = attributes_.begin(); it != attributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it->get();
  }
  return NULL;
}

bool AttributeState::hasAttribute(const string &name) const
{
  return attributeMap_.count(name)>0;
}

list< ref_ptr<VertexAttribute> >* AttributeState::attributesPtr()
{
  return &attributes_;
}
const list< ref_ptr<VertexAttribute> >& AttributeState::attributes() const
{
  return attributes_;
}

const list< ref_ptr<VertexAttribute> >& AttributeState::interleavedAttributes()
{
  return interleavedAttributes_;
}
const list< ref_ptr<VertexAttribute> >& AttributeState::sequentialAttributes()
{
  return sequentialAttributes_;
}

AttributeIteratorConst AttributeState::setAttribute(
    ref_ptr<VertexAttributeuiv> attribute)
{
  setAttribute(ref_ptr<VertexAttribute>::cast(attribute));
}
AttributeIteratorConst AttributeState::setAttribute(
    ref_ptr<VertexAttributefv> attribute)
{
  setAttribute(ref_ptr<VertexAttribute>::cast(attribute));
}
AttributeIteratorConst AttributeState::setAttribute(
    ref_ptr<VertexAttribute> attribute)
{
  if(attributeMap_.count(attribute->name())>0) {
    removeAttribute(attribute->name());
  } else { // insert into map of known attributes
    attributeMap_.insert(attribute->name());
  }
  attribute->set_buffer(0);

  attributes_.push_back(attribute);
  if(attribute->divisor()==0) {
    interleavedAttributes_.push_back(attribute);
  } else {
    sequentialAttributes_.push_back(attribute);
  }
  AttributeIteratorConst last = attributes_.end();
  --last;

  return last;
}

void AttributeState::removeAttribute(ref_ptr<VertexAttribute> att)
{
  attributeMap_.erase(att->name());
  removeAttribute(att->name());
}

void AttributeState::removeAttribute(const string &name)
{
  for(list< ref_ptr<VertexAttribute> >::iterator it = attributes_.begin();
      it != attributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      attributes_.erase(it);
      break;
    }
  }
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
}

//////////

void AttributeState::enable(RenderState *state)
{
  State::enable(state);
  if(!state->shaders.isEmpty()) {
    // if a shader is enabled by a parent node,
    // then try to enable the vbo attributes on the shader.
    Shader *shader = state->shaders.top();
    for(list< ref_ptr<VertexAttribute> >::iterator
        it = attributes_.begin(); it != attributes_.end(); ++it)
    {
      shader->applyAttribute(it->get());
    }
  }
}

void AttributeState::configureShader(ShaderConfiguration *shaderCfg)
{
  State::configureShader(shaderCfg);
  for(list< ref_ptr<VertexAttribute> >::iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    shaderCfg->setAttribute(it->get());
  }
}
