/*
 * attribute-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/utility/gl-util.h>
#include <regen/utility/string-util.h>
#include <regen/states/feedback-state.h>
#include <regen/gl-types/vbo-manager.h>

#include "mesh-state.h"
using namespace regen;

Mesh::Mesh(GLenum primitive)
: ShaderInputState(), primitive_(primitive), numIndices_(0u), feedbackCount_(0)
{
  draw_ = &Mesh::drawArrays;
  set_primitive(primitive);
}

GLenum Mesh::primitive() const
{
  return primitive_;
}
void Mesh::set_primitive(GLenum primitive)
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

void Mesh::setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex)
{
  indices_ = indices;
  numIndices_ = indices_->numVertices();
  maxIndex_ = maxIndex;
  VBOManager::add(indices_);

  draw_ = &Mesh::drawElements;
  feedbackCount_ = numIndices_;
  if(feedbackState_.get()) {
    feedbackState_->set_feedbackCount(feedbackCount_);
  }
}

const ref_ptr<FeedbackState>& Mesh::feedbackState()
{
  if(feedbackState_.get()==NULL) {
    feedbackState_ = ref_ptr<FeedbackState>::manage(new FeedbackState(feedbackPrimitive_, feedbackCount_));
    joinStates(ref_ptr<State>::cast(feedbackState_));
  }
  return feedbackState_;
}

void Mesh::draw(GLuint numInstances)
{
  (this->*draw_)(numInstances);
}
void Mesh::drawArrays(GLuint numInstances)
{
  glDrawArraysInstancedEXT(
      primitive_,
      0,
      numVertices_,
      numInstances);
}
void Mesh::drawElements(GLuint numInstances)
{
  glDrawElementsInstancedEXT(
      primitive_,
      numIndices_,
      indices_->dataType(),
      BUFFER_OFFSET(indices_->offset()),
      numInstances);
}

void Mesh::enable(RenderState *state)
{
  ShaderInputState::enable(state);
  (this->*draw_)( numInstances() );
}

ref_ptr<ShaderInput> Mesh::positions() const
{ return getInput(ATTRIBUTE_NAME_POS); }
ref_ptr<ShaderInput> Mesh::normals() const
{ return getInput(ATTRIBUTE_NAME_NOR); }
ref_ptr<ShaderInput> Mesh::colors() const
{ return getInput(ATTRIBUTE_NAME_COL0); }

GLuint Mesh::numIndices() const
{ return numIndices_; }
GLuint Mesh::maxIndex()
{ return maxIndex_; }
const ref_ptr<VertexAttribute>& Mesh::indices() const
{ return indices_; }
GLuint Mesh::indexBuffer() const
{ return indices_.get() ? indices_->buffer() : 0; }
