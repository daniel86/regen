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
: ShaderInputState(),
  primitive_(primitive)
{
  // TODO: AttributeState: use VAO
  vertices_ = inputs_.end();
  normals_ = inputs_.end();
  colors_ = inputs_.end();

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

const ShaderInputIteratorConst& MeshState::vertices() const
{
  return vertices_;
}
const ShaderInputIteratorConst& MeshState::normals() const
{
  return normals_;
}
const ShaderInputIteratorConst& MeshState::colors() const
{
  return colors_;
}

ShaderInputIteratorConst MeshState::setInput(ref_ptr<ShaderInput> in)
{
  if(in->numVertices()>1) {
    // it is a per vertex attribute
    numVertices_ = in->numVertices();
  }

  ShaderInputIteratorConst it = ShaderInputState::setInput(in);

  if(in->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = it;
  } else if(in->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = it;
  } else if(in->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = it;
  }

  return it;
}
void MeshState::removeInput(ref_ptr<ShaderInput> &in)
{
  if(in->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = inputs_.end();
  } else if(in->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = inputs_.end();
  } else if(in->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = inputs_.end();
  }
  ShaderInputState::removeInput(in);
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
      it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      return it->get();
    }
  }
  return NULL;
}
GLboolean MeshState::hasTransformFeedbackAttribute(const string &name) const
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

AttributeIteratorConst MeshState::setTransformFeedbackAttribute(ref_ptr<ShaderInput> in)
{
  if(tfAttributeMap_.count(in->name())>0) {
    removeTransformFeedbackAttribute(in->name());
  } else { // insert into map of known attributes
    tfAttributeMap_[in->name()] = in;
  }

  if(tfAttributes_.empty()) {
    joinStates(transformFeedbackState_);
  }

  in->set_size(numVertices_ * in->elementSize());
  in->set_numVertices(numVertices_);

  tfAttributes_.push_front(ref_ptr<VertexAttribute>::cast(in));

  return tfAttributes_.begin();
}

void MeshState::removeTransformFeedbackAttribute(ref_ptr<ShaderInput> in)
{
  removeTransformFeedbackAttribute(in->name());
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
      VertexBufferObject::USAGE_STREAM,
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
  ShaderInputState::enable(state);
  draw(state->numInstances());
}

void MeshState::configureShader(ShaderConfiguration *shaderCfg)
{
  ShaderInputState::configureShader(shaderCfg);
  for(list< ref_ptr<VertexAttribute> >::iterator
      it=tfAttributes_.begin(); it!=tfAttributes_.end(); ++it)
  {
    shaderCfg->setTransformFeedbackAttribute((ShaderInput*)it->get());
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
  ref_ptr<ShaderInput> indicesAtt = ref_ptr<ShaderInput>::manage(new ShaderInput1ui("i"));
  indicesAtt->setVertexData(numIndices, indicesBytes);
  setIndices(ref_ptr<VertexAttribute>::cast(indicesAtt), maxIndex);
}

AttributeIteratorConst IndexedMeshState::setTransformFeedbackAttribute(ref_ptr<ShaderInput> in)
{
  AttributeIteratorConst it = MeshState::setTransformFeedbackAttribute(in);

  in->set_size(numIndices_ * in->elementSize());
  in->set_numVertices(numIndices_);

  return it;
}

////////////

TFMeshState::TFMeshState(ref_ptr<MeshState> attState)
: State(),
  attState_(attState)
{

}

string TFMeshState::name()
{
  return FORMAT_STRING("TFMeshState");
}

void TFMeshState::enable(RenderState *state)
{
  state->pushVBO(attState_->transformFeedbackBuffer().get());
  for(list< ref_ptr<VertexAttribute> >::iterator
      it=attState_->tfAttributesPtr()->begin(); it!=attState_->tfAttributesPtr()->end(); ++it)
  {
    state->pushShaderInput((ShaderInput*)it->get());
  }
  State::enable(state);
  attState_->drawTransformFeedback(state->numInstances());
}

void TFMeshState::disable(RenderState *state)
{
  State::disable(state);
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=attState_->tfAttributes().begin(); it!=attState_->tfAttributes().end(); ++it)
  {
    state->popShaderInput((*it)->name());
  }
  state->popVBO();
}
