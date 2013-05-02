/*
 * attribute-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/feedback-state.h>
#include <regen/gl-types/gl-util.h>

#include "mesh-state.h"
using namespace regen;

Mesh::Mesh(GLenum primitive, const ref_ptr<ShaderInputContainer> &inputs)
: State(), HasInput(inputs), primitive_(primitive), feedbackCount_(0)
{
  draw_ = &Mesh::drawArrays;
  set_primitive(primitive);
}
Mesh::Mesh(GLenum primitive, VertexBufferObject::Usage usage)
: State(), HasInput(usage), primitive_(primitive), feedbackCount_(0)
{
  draw_ = &Mesh::drawArrays;
  set_primitive(primitive);
  // XXX: set shader defines
  //shaderDefine(REGEN_STRING("HAS_"<<in->name()), "TRUE");
  //if(in->numInstances()>1)
  //{ shaderDefine("HAS_INSTANCES", "TRUE"); }
  // XXX
  //draw_ = &Mesh::drawElements;
  //feedbackCount_ = numIndices_;
  //if(feedbackState_.get()) {
  //  feedbackState_->set_feedbackCount(feedbackCount_);
  //}
}

void Mesh::beginUpload(ShaderInputContainer::DataLayout layout)
{
  inputContainer_->beginUpload(layout);
}
void Mesh::endUpload()
{
  inputContainer_->endUpload();
  updateVAO(RenderState::get());
}

const ref_ptr<VertexArrayObject>& Mesh::vao() const
{ return vao_; }
void Mesh::set_vao(const ref_ptr<VertexArrayObject> &vao)
{ vao_ = vao; }

void Mesh::updateVAO(
    RenderState *rs,
    const Config &cfg,
    const ref_ptr<Shader> &meshShader)
{
  meshAttributes_.clear();
  meshShader_ = meshShader;

  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  rs->vao().push(vao_->id());

  GLuint lastArrayBuffer=0;

  for(map<string, ref_ptr<ShaderInput> >::const_iterator
      it=cfg.inputs_.begin(); it!=cfg.inputs_.end(); ++it)
  {
    const string &name = it->first;
    const ref_ptr<ShaderInput> &in = it->second;
    if(!in->isVertexAttribute()) continue;

    GLint loc = meshShader->attributeLocation(name);
    if(loc!=-1) {
      if(lastArrayBuffer!=in->buffer()) {
        lastArrayBuffer = in->buffer();
        glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
      }
      in->enableAttribute(loc);
      meshAttributes_.push_back(ShaderInputLocation(in,loc));
    }
  }

  if(inputContainer_->indexBuffer()>0)
  { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inputContainer_->indexBuffer()); }

  rs->vao().pop();
}
void Mesh::updateVAO(RenderState *rs)
{
  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  rs->vao().push(vao_->id());

  GLuint lastArrayBuffer=0;

  for(list<ShaderInputLocation>::const_iterator
      it=meshAttributes_.begin(); it!=meshAttributes_.end(); ++it)
  {
    const ShaderInputLocation &x = *it;

    if(lastArrayBuffer!=x.input->buffer()) {
      lastArrayBuffer = x.input->buffer();
      glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    }
    x.input->enableAttribute(x.location);
  }

  if(inputContainer_->indexBuffer()>0)
  { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inputContainer_->indexBuffer()); }

  rs->vao().pop();
}

GLenum Mesh::primitive() const
{ return primitive_; }

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

const ref_ptr<FeedbackState>& Mesh::feedbackState()
{
  if(feedbackState_.get()==NULL) {
    feedbackState_ = ref_ptr<FeedbackState>::manage(new FeedbackState(feedbackPrimitive_, feedbackCount_));
    joinStates(feedbackState_);
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
      inputContainer_->numVertices(),
      numInstances);
}
void Mesh::drawElements(GLuint numInstances)
{
  glDrawElementsInstancedEXT(
      primitive_,
      inputContainer_->numIndices(),
      inputContainer_->indices()->dataType(),
      BUFFER_OFFSET(inputContainer_->indices()->offset()),
      numInstances);
}

void Mesh::enable(RenderState *rs)
{
  State::enable(rs);
  rs->vao().push(vao_->id());
  (this->*draw_)( inputContainer_->numInstances() );
}
void Mesh::disable(RenderState *rs)
{
  State::disable(rs);
  rs->vao().pop();
}

ref_ptr<ShaderInput> Mesh::positions() const
{ return inputContainer_->getInput(ATTRIBUTE_NAME_POS); }
ref_ptr<ShaderInput> Mesh::normals() const
{ return inputContainer_->getInput(ATTRIBUTE_NAME_NOR); }
ref_ptr<ShaderInput> Mesh::colors() const
{ return inputContainer_->getInput(ATTRIBUTE_NAME_COL0); }

////////////
////////////

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
: Mesh(GL_POINTS, VertexBufferObject::USAGE_STATIC)
{
  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  inputContainer_->set_numVertices(numVertices);
}
