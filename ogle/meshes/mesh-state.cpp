/*
 * attribute-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <ogle/utility/gl-util.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/feedback-state.h>
#include <ogle/gl-types/vbo-manager.h>

#include "mesh-state.h"
using namespace ogle;

MeshState::MeshState(GLenum primitive)
: ShaderInputState(),
  primitive_(primitive)
{
  vertices_ = inputs_.end();
  normals_ = inputs_.end();
  colors_ = inputs_.end();

  set_primitive(primitive);
  set_feedbackMode(GL_SEPARATE_ATTRIBS);
  set_feedbackStage(GL_VERTEX_SHADER);
}

GLenum MeshState::primitive() const
{
  return primitive_;
}
void MeshState::set_primitive(GLenum primitive)
{
  primitive_ = primitive;
  switch(primitive_) {
  case GL_PATCHES:
    feedbackPrimitive_ = GL_TRIANGLES;
    break;
  case GL_POINTS:
    feedbackPrimitive_ = GL_POINTS;
    break;
  case GL_LINES:
  case GL_LINE_LOOP:
  case GL_LINE_STRIP:
  case GL_LINES_ADJACENCY:
  case GL_LINE_STRIP_ADJACENCY:
    feedbackPrimitive_ = GL_LINES;
    break;
  case GL_TRIANGLES:
  case GL_TRIANGLE_STRIP:
  case GL_TRIANGLE_FAN:
  case GL_TRIANGLES_ADJACENCY:
  case GL_TRIANGLE_STRIP_ADJACENCY:
    feedbackPrimitive_ = GL_TRIANGLES;
    break;
  default:
    feedbackPrimitive_ = GL_TRIANGLES;
    break;
  }
}

GLuint MeshState::numVertices() const
{
  return numVertices_;
}

const ShaderInputItConst& MeshState::positions() const
{
  return vertices_;
}
const ShaderInputItConst& MeshState::normals() const
{
  return normals_;
}
const ShaderInputItConst& MeshState::colors() const
{
  return colors_;
}

ShaderInputItConst MeshState::setInput(const ref_ptr<ShaderInput> &in)
{
  if(in->numVertices()>1) {
    // it is a per vertex attribute
    numVertices_ = in->numVertices();
  }

  ShaderInputItConst it = ShaderInputState::setInput(in);

  if(in->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = it;
  } else if(in->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = it;
  } else if(in->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = it;
  }

  return it;
}
void MeshState::removeInput(const ref_ptr<ShaderInput> &in)
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

void MeshState::draw(GLuint numInstances)
{
  glDrawArraysInstancedEXT(primitive_, 0, numVertices_, numInstances);
}

/////////////

void MeshState::drawFeedback(GLuint numInstances)
{
  glDrawArraysInstancedEXT(feedbackPrimitive_, 0, numVertices_, numInstances);
}

void MeshState::set_feedbackPrimitive(GLenum primitive)
{
  feedbackPrimitive_ = primitive;
}
GLenum MeshState::feedbackPrimitive() const
{
  return feedbackPrimitive_;
}

void MeshState::set_feedbackStage(GLenum stage)
{
  feedbackStage_ = stage;
}
GLenum MeshState::feedbackStage() const
{
  return feedbackStage_;
}

void MeshState::set_feedbackMode(GLenum mode)
{
  if(feedbackMode_ == mode) { return; }
  feedbackMode_ = mode;

  // join feedback state
  if(feedbackState_.get()!=NULL) {
    disjoinStates(feedbackState_);
    if(feedbackMode_==GL_INTERLEAVED_ATTRIBS) {
      feedbackState_ = ref_ptr<State>::manage(
          new FeedbackStateInterleaved(feedbackPrimitive_, feedbackBuffer_));
    } else {
      feedbackState_ = ref_ptr<State>::manage(
          new FeedbackStateSeparate(feedbackPrimitive_, feedbackBuffer_, feedbackAttributes_));
    }
    joinStates(feedbackState_);
  }
}
GLenum MeshState::feedbackMode() const
{
  return feedbackMode_;
}

const list< ref_ptr<VertexAttribute> >& MeshState::feedbackAttributes() const
{
  return feedbackAttributes_;
}

AttributeIteratorConst MeshState::setFeedbackAttribute(
    const string &attributeName, GLenum dataType, GLuint valsPerElement)
{
  // remove if already added
  if(feedbackAttributeMap_.count(attributeName)>0) {
    removeFeedbackAttribute(attributeName);
  }

  // create feedback attribute
  ref_ptr<VertexAttribute> feedback = ref_ptr<VertexAttribute>::cast(
      ShaderInput::create(attributeName, dataType, valsPerElement));
  feedback->set_size(numVertices_ * feedback->elementSize());
  feedback->set_numVertices(numVertices_);
  feedbackAttributes_.push_front(feedback);
  feedbackAttributeMap_[attributeName] = feedback;

  // join feedback state
  if(feedbackState_.get()==NULL) {
    if(feedbackMode_==GL_INTERLEAVED_ATTRIBS) {
      feedbackState_ = ref_ptr<State>::manage(
          new FeedbackStateInterleaved(feedbackPrimitive_, feedbackBuffer_));
    } else {
      feedbackState_ = ref_ptr<State>::manage(
          new FeedbackStateSeparate(feedbackPrimitive_, feedbackBuffer_, feedbackAttributes_));
    }
    joinStates(feedbackState_);
  }

  return feedbackAttributes_.begin();
}
AttributeIteratorConst MeshState::setFeedbackAttribute(const ref_ptr<VertexAttribute> &in)
{
  return setFeedbackAttribute(in->name(), in->dataType(), in->valsPerElement());
}

void MeshState::removeFeedbackAttribute(const string &name)
{
  feedbackAttributeMap_.erase(name);
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = feedbackAttributes_.begin(); it != feedbackAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      feedbackAttributes_.erase(it);
      return;
    }
  }
  if(feedbackAttributes_.size()==0) {
    disjoinStates(feedbackState_);
  }
}
AttributeIteratorConst MeshState::getFeedbackAttribute(const string &name) const
{
  AttributeIteratorConst it;
  for(it = feedbackAttributes_.begin(); it != feedbackAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) return it;
  }
  return it;
}
GLboolean MeshState::hasFeedbackAttribute(const string &name) const
{
  return feedbackAttributeMap_.count(name)>0;
}

const ref_ptr<VertexBufferObject>& MeshState::feedbackBuffer()
{
  return feedbackBuffer_;
}
void MeshState::createFeedbackBuffer()
{
  if(feedbackAttributes_.empty()) { return; }

  GLuint bufferSize = 0;
  for(AttributeIteratorConst it=feedbackAttributes_.begin(); it!=feedbackAttributes_.end(); ++it)
  {
    bufferSize += (*it)->size();
  }

  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_STREAM, bufferSize));
  if(feedbackMode_ == GL_INTERLEAVED_ATTRIBS) {
    feedbackBuffer_->allocateInterleaved(feedbackAttributes_);
  } else {
    feedbackBuffer_->allocateSequential(feedbackAttributes_);
  }
}

//////////

void MeshState::enable(RenderState *state)
{
  ShaderInputState::enable(state);
  if(!state->shader().stack_.isEmpty()) {
    draw( state->shader().stack_.top()->numInstances() );
  }
}
void MeshState::disable(RenderState *state)
{
  ShaderInputState::disable(state);
}

////////////

IndexedMeshState::IndexedMeshState(GLenum primitive)
: MeshState(primitive),
  numIndices_(0u)
{
}

GLuint IndexedMeshState::numIndices() const
{
  return numIndices_;
}

GLuint IndexedMeshState::maxIndex()
{
  return maxIndex_;
}

const ref_ptr<VertexAttribute>& IndexedMeshState::indices() const
{
  return indices_;
}

void IndexedMeshState::draw(GLuint numInstances)
{
  glDrawElementsInstancedEXT(
      primitive_,
      numIndices_,
      indices_->dataType(),
      BUFFER_OFFSET(indices_->offset()),
      numInstances);
}

void IndexedMeshState::drawFeedback(GLuint numInstances)
{
  glDrawArraysInstanced(
      feedbackPrimitive_,
      0,
      numIndices_,
      numInstances);
}

void IndexedMeshState::setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex)
{
  indices_ = indices;
  numIndices_ = indices_->numVertices();
  maxIndex_ = maxIndex;
}

void IndexedMeshState::setFaceIndicesui(GLuint *faceIndices, GLuint numFaceIndices, GLuint numFaces)
{
  numIndices_ = numFaces*numFaceIndices;
  maxIndex_ = 0;

  // find max index
  for(GLuint i=0; i<numIndices_; ++i)
  {
    GLuint &index = faceIndices[i];
    if(index>maxIndex_) { maxIndex_=index; }
  }

  indices_ = ref_ptr<VertexAttribute>::manage(new VertexAttribute(
      "i", GL_UNSIGNED_INT, sizeof(GLuint), 1, 1, GL_FALSE));
  indices_->setVertexData(numIndices_, (byte*)faceIndices);
  VBOManager::add(indices_);
}

AttributeIteratorConst IndexedMeshState::setFeedbackAttribute(
    const string &attributeName, GLenum dataType, GLuint valsPerElement)
{
  AttributeIteratorConst it = MeshState::setFeedbackAttribute(
      attributeName, dataType, valsPerElement);
  (*it)->set_size(numIndices_ * (*it)->elementSize());
  (*it)->set_numVertices(numIndices_);
  return it;
}
