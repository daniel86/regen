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
: ShaderInputState(), primitive_(primitive), numIndices_(0u), feedbackCount_(0)
{
  draw_ = &MeshState::drawArrays;
  set_primitive(primitive);
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

void MeshState::setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex)
{
  indices_ = indices;
  numIndices_ = indices_->numVertices();
  maxIndex_ = maxIndex;
  VBOManager::add(indices_);

  draw_ = &MeshState::drawElements;
  feedbackCount_ = numIndices_;
  if(feedbackState_.get()) {
    feedbackState_->set_feedbackCount(feedbackCount_);
  }
}

const ref_ptr<FeedbackState>& MeshState::feedbackState()
{
  if(feedbackState_.get()==NULL) {
    feedbackState_ = ref_ptr<FeedbackState>::manage(new FeedbackState(feedbackPrimitive_, feedbackCount_));
    joinStates(ref_ptr<State>::cast(feedbackState_));
  }
  return feedbackState_;
}

void MeshState::draw(GLuint numInstances)
{
  (this->*draw_)(numInstances);
}
void MeshState::drawArrays(GLuint numInstances)
{
  glDrawArraysInstancedEXT(
      primitive_,
      0,
      numVertices_,
      numInstances);
}
void MeshState::drawElements(GLuint numInstances)
{
  glDrawElementsInstancedEXT(
      primitive_,
      numIndices_,
      indices_->dataType(),
      BUFFER_OFFSET(indices_->offset()),
      numInstances);
}

void MeshState::enable(RenderState *state)
{
  ShaderInputState::enable(state);
  if(!state->shader().stack_.isEmpty()) { // XXX
    draw( state->shader().stack_.top()->numInstances() );
  }
}

ref_ptr<ShaderInput> MeshState::positions() const
{ return getInput(ATTRIBUTE_NAME_POS); }
ref_ptr<ShaderInput> MeshState::normals() const
{ return getInput(ATTRIBUTE_NAME_NOR); }
ref_ptr<ShaderInput> MeshState::colors() const
{ return getInput(ATTRIBUTE_NAME_COL0); }

GLuint MeshState::numIndices() const
{ return numIndices_; }
GLuint MeshState::maxIndex()
{ return maxIndex_; }
const ref_ptr<VertexAttribute>& MeshState::indices() const
{ return indices_; }
