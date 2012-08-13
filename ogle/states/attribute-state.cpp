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

class TransformFeedbackState : public State
{
public:
  TransformFeedbackState(
      const list< ref_ptr<VertexAttribute> > &atts,
      const GLenum &transformFeedbackPrimitive,
      const ref_ptr<VertexBufferObject> &transformFeedbackBuffer)
  : State(),
    atts_(atts),
    transformFeedbackPrimitive_(transformFeedbackPrimitive),
    transformFeedbackBuffer_(transformFeedbackBuffer)
  {
  }
  virtual void enable(RenderState *state)
  {
    VertexBufferObject *vbo = state->vbos.top();
    GLint bufferIndex=0;
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        it=atts_.begin(); it!=atts_.end(); ++it)
    {
      const ref_ptr<VertexAttribute> &att = *it;
      glBindBufferRange(
          GL_TRANSFORM_FEEDBACK_BUFFER,
          bufferIndex++,
          transformFeedbackBuffer_->id(),
          att->offset(),
          att->size());
    }
    glBeginTransformFeedback(transformFeedbackPrimitive_);
    State::enable(state);
  }
  virtual void disable(RenderState *state)
  {
    State::disable(state);
    glEndTransformFeedback();
    for(int bufferIndex=0; bufferIndex<atts_.size(); ++bufferIndex)
    {
      glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bufferIndex, 0);
    }
  }
  virtual string name()
  {
    return "TransformFeedbackState";
  }
protected:
  const list< ref_ptr<VertexAttribute> > &atts_;
  const GLenum &transformFeedbackPrimitive_;
  const ref_ptr<VertexBufferObject> &transformFeedbackBuffer_;
};

AttributeState::AttributeState(GLenum primitive)
: State(),
  primitive_(primitive),
  numVertices_(0)
{
  // TODO: AttributeState: use VAO
  vertices_ = attributes_.end();
  normals_ = attributes_.end();
  colors_ = attributes_.end();

  set_primitive(primitive);

  transformFeedbackState_ = ref_ptr<State>::manage(
      new TransformFeedbackState(tfAttributes_, transformFeedbackPrimitive_, tfVBO_));
}

string AttributeState::name()
{
  return FORMAT_STRING("AttributeState");
}

GLenum AttributeState::primitive() const
{
  return primitive_;
}
GLenum AttributeState::transformFeedbackPrimitive() const
{
  return transformFeedbackPrimitive_;
}
void AttributeState::set_primitive(GLenum primitive)
{
  primitive_ = primitive;
  switch(primitive_) {
  case GL_PATCHES:
    transformFeedbackPrimitive_ = GL_TRIANGLES;
    break;
  case GL_POINTS:
    transformFeedbackPrimitive_ = GL_POINTS;
    break;
  case GL_LINES:
  case GL_LINE_LOOP:
  case GL_LINE_STRIP:
  case GL_LINES_ADJACENCY:
  case GL_LINE_STRIP_ADJACENCY:
    transformFeedbackPrimitive_ = GL_LINES;
    break;
  case GL_TRIANGLES:
  case GL_TRIANGLE_STRIP:
  case GL_TRIANGLE_FAN:
  case GL_TRIANGLES_ADJACENCY:
  case GL_TRIANGLE_STRIP_ADJACENCY:
    transformFeedbackPrimitive_ = GL_TRIANGLES;
    break;
  default:
    transformFeedbackPrimitive_ = GL_TRIANGLES;
    break;
  }
}

GLuint AttributeState::numVertices() const
{
  return numVertices_;
}

const AttributeIteratorConst& AttributeState::vertices() const
{
  return vertices_;
}
const AttributeIteratorConst& AttributeState::normals() const
{
  return normals_;
}
const AttributeIteratorConst& AttributeState::colors() const
{
  return colors_;
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

/////////////

void AttributeState::draw(GLuint numInstances)
{
  if(numInstances>1) {
    glDrawArraysInstanced(
        primitive_,
        0,
        numVertices_,
        numInstances);
  } else {
    glDrawArrays(
        primitive_,
        // this is not the offset in the buffer
        // but the first index of vertex data to consider
        0,
        numVertices_);
  }
}

/////////////

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
  numVertices_ = attribute->numVertices();

  if(attributeMap_.count(attribute->name())>0) {
    removeAttribute(attribute->name());
  } else { // insert into map of known attributes
    attributeMap_.insert(attribute->name());
  }
  attribute->set_buffer(0);

  attributes_.push_back(attribute);
  interleavedAttributes_.push_back(attribute);
  AttributeIteratorConst last = attributes_.end();
  --last;

  if(attribute->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = last;
  } else if(attribute->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = last;
  } else if(attribute->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = last;
  }

  return last;
}

void AttributeState::removeAttribute(ref_ptr<VertexAttribute> att)
{
  attributeMap_.erase(att->name());

  if(att->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = attributes_.end();
  } else if(att->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = attributes_.end();
  } else if(att->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = attributes_.end();
  }

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
}

/////////////

ref_ptr<VertexBufferObject>& AttributeState::transformFeedbackBuffer()
{
  return tfVBO_;
}

AttributeIteratorConst AttributeState::getTransformFeedbackAttribute(const string &name) const
{
  AttributeIteratorConst it;
  for(it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it;
  }
  return it;
}
VertexAttribute* AttributeState::getTransformFeedbackAttributePtr(const string &name)
{
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it->get();
  }
  return NULL;
}
bool AttributeState::hasTransformFeedbackAttribute(const string &name) const
{
  return tfAttributeMap_.count(name)>0;
}

list< ref_ptr<VertexAttribute> >* AttributeState::tfAttributesPtr()
{
  return &tfAttributes_;
}
const list< ref_ptr<VertexAttribute> >& AttributeState::tfAttributes() const
{
  return tfAttributes_;
}

AttributeIteratorConst AttributeState::setTransformFeedbackAttribute(
    ref_ptr<VertexAttribute> attribute)
{
  if(tfAttributes_.empty()) {
    joinStates(transformFeedbackState_);
  }

  if(tfAttributeMap_.count(attribute->name())>0) {
    removeTransformFeedbackAttribute(attribute->name());
  } else { // insert into map of known attributes
    tfAttributeMap_[attribute->name()] = attribute;
  }
  attribute->set_size(numVertices_ * attribute->elementSize());

  tfAttributes_.push_back(attribute);
  AttributeIteratorConst last = tfAttributes_.end();
  --last;

  return last;
}

void AttributeState::removeTransformFeedbackAttribute(ref_ptr<VertexAttribute> att)
{
  removeTransformFeedbackAttribute(att->name());
}

void AttributeState::removeTransformFeedbackAttribute(const string &name)
{
  tfAttributeMap_.erase(name);
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      tfAttributes_.erase(it);
      return;
    }
  }

  if(tfAttributes_.size()==0) {
    disjoinStates(transformFeedbackState_);
  }
}

void AttributeState::updateTransformFeedbackBuffer()
{
  if(tfAttributes_.empty()) { return; }
  tfVBO_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_DYNAMIC,
      VertexBufferObject::attributeStructSize(tfAttributes_)
  ));
  tfVBO_->allocateSequential(tfAttributes_);
}

void AttributeState::drawTransformFeedback(GLuint numInstances)
{
  if(numInstances>1) {
    glDrawArraysInstanced(
        transformFeedbackPrimitive_,
        0,
        numVertices_,
        numInstances);
  } else {
    glDrawArrays(
        transformFeedbackPrimitive_,
        0,
        numVertices_);
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
  draw(state->numInstances());
}

void AttributeState::configureShader(ShaderConfiguration *shaderCfg)
{
  State::configureShader(shaderCfg);
  for(list< ref_ptr<VertexAttribute> >::iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    shaderCfg->setAttribute(it->get());
  }
  for(list< ref_ptr<VertexAttribute> >::iterator
      it=tfAttributes_.begin(); it!=tfAttributes_.end(); ++it)
  {
    shaderCfg->setTransformFeedbackAttribute(it->get());
  }
  updateTransformFeedbackBuffer();
}

////////////

IndexedAttributeState::IndexedAttributeState(GLenum primitive)
: AttributeState(primitive),
  numIndices_(0u)
{

}

string IndexedAttributeState::name()
{
  return FORMAT_STRING("IndexedAttributeState");
}

GLuint IndexedAttributeState::numIndices() const
{
  return numIndices_;
}

GLuint IndexedAttributeState::maxIndex()
{
  return maxIndex_;
}

ref_ptr<VertexAttribute>& IndexedAttributeState::indices()
{
  return indices_;
}

GLboolean IndexedAttributeState::isBufferSet()
{
  if(!AttributeState::isBufferSet()) {
    return false;
  } else {
    return indices_->buffer()!=0;
  }
}

void IndexedAttributeState::setBuffer(GLuint buffer)
{
  AttributeState::setBuffer(buffer);
  indices_->set_buffer(buffer);
}

void IndexedAttributeState::draw(GLuint numInstances)
{
  if(numInstances>1) {
    glDrawElementsInstanced(
        primitive_,
        numIndices_,
        indices_->dataType(),
        BUFFER_OFFSET(indices_->offset()),
        numInstances);
  } else {
    glDrawElements(
        primitive_,
        numIndices_,
        indices_->dataType(),
        BUFFER_OFFSET(indices_->offset()));
  }
}

void IndexedAttributeState::setIndices(
    ref_ptr< VertexAttribute > indices,
    GLuint maxIndex)
{
  if(indices_.get()) {
    for(list< ref_ptr<VertexAttribute> >::iterator
        it=sequentialAttributes_.begin(); it!=sequentialAttributes_.end(); ++it)
    {
      if(it->get() == indices_.get()) {
        sequentialAttributes_.erase(it);
        break;
      }
    }
  }
  indices_ = indices;
  numIndices_ = indices_->numVertices();
  maxIndex_ = maxIndex;
  sequentialAttributes_.push_back(indices_);
}

void IndexedAttributeState::setFaceIndicesui(
    GLuint *faceIndices,
    GLuint numFaceIndices,
    GLuint numFaces)
{
  const GLuint numIndices = numFaces*numFaceIndices;

  // find max index
  GLuint maxIndex = 0;
  for(GLuint i=0; i<numIndices; ++i)
  {
    GLuint &index = faceIndices[i];
    if(index>maxIndex) { maxIndex=index; }
  }

  byte* indicesBytes = (byte*)faceIndices;
  ref_ptr<VertexAttribute> indicesAtt = ref_ptr<VertexAttribute>::manage(
      new VertexAttributeUint("i", 1));
  indicesAtt->setVertexData(numIndices, indicesBytes);
  setIndices(indicesAtt, maxIndex);
}

////////////

TFAttributeState::TFAttributeState(ref_ptr<AttributeState> attState)
: attState_(attState)
{

}

string TFAttributeState::name()
{
  return FORMAT_STRING("TFAttributeState");
}

void TFAttributeState::enable(RenderState *state)
{
  state->pushVBO(attState_->transformFeedbackBuffer().get());
  State::enable(state);
  if(!state->shaders.isEmpty()) {
    // if a shader is enabled by a parent node,
    // then try to enable the vbo attributes on the shader.
    Shader *shader = state->shaders.top();
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        it=attState_->tfAttributes().begin(); it!=attState_->tfAttributes().end(); ++it)
    {
      shader->applyAttribute(it->get());
    }
  }
  attState_->drawTransformFeedback(state->numInstances());
}
void TFAttributeState::disable(RenderState *state)
{
  State::disable(state);
  state->popVBO();
}
