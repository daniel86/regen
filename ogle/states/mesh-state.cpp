/*
 * attribute-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "mesh-state.h"

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

MeshState::MeshState(GLenum primitive)
: AttributeState(),
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

string MeshState::name()
{
  return FORMAT_STRING("MeshState");
}

GLenum MeshState::primitive() const
{
  return primitive_;
}
GLenum MeshState::transformFeedbackPrimitive() const
{
  return transformFeedbackPrimitive_;
}
void MeshState::set_primitive(GLenum primitive)
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

GLuint MeshState::numVertices() const
{
  return numVertices_;
}

const AttributeIteratorConst& MeshState::vertices() const
{
  return vertices_;
}
const AttributeIteratorConst& MeshState::normals() const
{
  return normals_;
}
const AttributeIteratorConst& MeshState::colors() const
{
  return colors_;
}

AttributeIteratorConst MeshState::setAttribute(ref_ptr<VertexAttribute> attribute)
{
  if(attribute->divisor()==0) {
    // it is a per vertex attribute
    numVertices_ = attribute->numVertices();
  }
  AttributeIteratorConst last = AttributeState::setAttribute(attribute);
  if(attribute->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = last;
  } else if(attribute->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = last;
  } else if(attribute->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = last;
  }
  return last;
}
void MeshState::removeAttribute(ref_ptr<VertexAttribute> att)
{
  if(att->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = attributes_.end();
  } else if(att->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = attributes_.end();
  } else if(att->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = attributes_.end();
  }
  AttributeState::removeAttribute(att);
}

/////////////

void MeshState::draw(GLuint numInstances)
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

ref_ptr<VertexBufferObject>& MeshState::transformFeedbackBuffer()
{
  return tfVBO_;
}

AttributeIteratorConst MeshState::getTransformFeedbackAttribute(const string &name) const
{
  AttributeIteratorConst it;
  for(it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it;
  }
  return it;
}
VertexAttribute* MeshState::getTransformFeedbackAttributePtr(const string &name)
{
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it->get();
  }
  return NULL;
}
bool MeshState::hasTransformFeedbackAttribute(const string &name) const
{
  return tfAttributeMap_.count(name)>0;
}

list< ref_ptr<VertexAttribute> >* MeshState::tfAttributesPtr()
{
  return &tfAttributes_;
}
const list< ref_ptr<VertexAttribute> >& MeshState::tfAttributes() const
{
  return tfAttributes_;
}

AttributeIteratorConst MeshState::setTransformFeedbackAttribute(
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

void MeshState::removeTransformFeedbackAttribute(ref_ptr<VertexAttribute> att)
{
  removeTransformFeedbackAttribute(att->name());
}

void MeshState::removeTransformFeedbackAttribute(const string &name)
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

void MeshState::updateTransformFeedbackBuffer()
{
  if(tfAttributes_.empty()) { return; }
  tfVBO_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_DYNAMIC,
      VertexBufferObject::attributeStructSize(tfAttributes_)
  ));
  tfVBO_->allocateSequential(tfAttributes_);
}

void MeshState::drawTransformFeedback(GLuint numInstances)
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

void MeshState::enable(RenderState *state)
{
  AttributeState::enable(state);
  draw(state->numInstances());
}

void MeshState::configureShader(ShaderConfiguration *shaderCfg)
{
  AttributeState::configureShader(shaderCfg);
  for(list< ref_ptr<VertexAttribute> >::iterator
      it=tfAttributes_.begin(); it!=tfAttributes_.end(); ++it)
  {
    shaderCfg->setTransformFeedbackAttribute(it->get());
  }
  updateTransformFeedbackBuffer();
}

////////////

IndexedMeshState::IndexedMeshState(GLenum primitive)
: MeshState(primitive),
  numIndices_(0u)
{

}

string IndexedMeshState::name()
{
  return FORMAT_STRING("IndexedAttributeState");
}

GLuint IndexedMeshState::numIndices() const
{
  return numIndices_;
}

GLuint IndexedMeshState::maxIndex()
{
  return maxIndex_;
}

ref_ptr<VertexAttribute>& IndexedMeshState::indices()
{
  return indices_;
}

void IndexedMeshState::draw(GLuint numInstances)
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

void IndexedMeshState::setIndices(
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

void IndexedMeshState::setFaceIndicesui(
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

TFMeshState::TFMeshState(ref_ptr<MeshState> attState)
: attState_(attState)
{

}

string TFMeshState::name()
{
  return FORMAT_STRING("TFMeshState");
}

void TFMeshState::enable(RenderState *state)
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
void TFMeshState::disable(RenderState *state)
{
  State::disable(state);
  state->popVBO();
}
