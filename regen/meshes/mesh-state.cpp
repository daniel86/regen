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
  hasInstances_ = GL_FALSE;
  draw_ = &ShaderInputContainer::drawArrays;
  set_primitive(primitive);
}
Mesh::Mesh(GLenum primitive, VertexBufferObject::Usage usage)
: State(), HasInput(usage), primitive_(primitive), feedbackCount_(0)
{
  hasInstances_ = GL_FALSE;
  draw_ = &ShaderInputContainer::drawArrays;
  set_primitive(primitive);
}

void Mesh::begin(ShaderInputContainer::DataLayout layout)
{
  inputContainer_->begin(layout);
}
void Mesh::end()
{
  inputContainer_->end();
  // XXX: new attributes not added to meshAttributes_ list !
  // only works for updating attributes
  updateVAO(RenderState::get());
  //updateDrawFunction();
}

const ref_ptr<VertexArrayObject>& Mesh::vao() const
{ return vao_; }
void Mesh::set_vao(const ref_ptr<VertexArrayObject> &vao)
{ vao_ = vao; }

void Mesh::initializeResources(
    RenderState *rs,
    const Config &cfg,
    const ref_ptr<Shader> &meshShader)
{
  // remember the shader
  meshShader_ = meshShader;
  hasInstances_ = GL_FALSE;

  // reset attribute list
  meshAttributes_.clear();
  meshUniforms_.clear();
  meshTextures_.clear();
  // and load from Config
  for(map<string, ref_ptr<ShaderInput> >::const_iterator
      it=cfg.inputs_.begin(); it!=cfg.inputs_.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = it->second;
    const string &name = in->name();
    if(in->isVertexAttribute()) {
      GLint loc = meshShader->attributeLocation(name);
      if(loc==-1) continue;
      if(!in->bufferIterator().get()) {
        // allocate VBO memory if not already allocated
        inputContainer_->inputBuffer()->alloc(in);
      }
      meshAttributes_.push_back(ShaderInputLocation(in,loc));
    }
    else if(!in->isConstant())
    {
      if(meshShader_->isUniform(name) &&
          meshShader_->input(name).get()==in.get())
      {
        // shader handles uniform already.
        continue;
      }
      GLint loc = meshShader->uniformLocation(name);
      if(loc==-1) continue;
      meshUniforms_.push_back(ShaderInputLocation(in,loc));
    }
  }
  for(list<const TextureState*>::const_iterator
      it=cfg.textures_.begin(); it!=cfg.textures_.end(); ++it)
  {
    // TODO: setup textures below shader
    //if(meshShader_->isSampler(name)) {
    //}
  }
  updateVAO(rs);
  updateDrawFunction();
}

void Mesh::updateDrawFunction()
{
  if(inputContainer_->indexBuffer()>0) {
    if(hasInstances_) {
      draw_ = &ShaderInputContainer::drawElementsInstanced;
    } else {
      draw_ = &ShaderInputContainer::drawElements;
    }
  }
  else {
    if(hasInstances_) {
      draw_ = &ShaderInputContainer::drawArraysInstanced;
    } else {
      draw_ = &ShaderInputContainer::drawArrays;
    }
  }
}

void Mesh::updateVAO(RenderState *rs)
{
  GLuint lastArrayBuffer=0;
  // create a VAO
  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  rs->vao().push(vao_->id());
  // Setup attributes
  for(list<ShaderInputLocation>::const_iterator
      it=meshAttributes_.begin(); it!=meshAttributes_.end(); ++it)
  {
    const ShaderInputLocation &x = *it;
    if(lastArrayBuffer!=x.input->buffer()) {
      lastArrayBuffer = x.input->buffer();
      glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    }
    x.input->enableAttribute(x.location);
    if(x.input->numInstances()>1) hasInstances_=GL_TRUE;
  }
  // bind the index buffer
  if(inputContainer_->indexBuffer()>0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inputContainer_->indexBuffer());
  }
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

void Mesh::enable(RenderState *rs)
{
  State::enable(rs);
  // TODO: uniform and textures overrides should pop back to old value?
  Shader::enableUniforms(rs, meshUniforms_);
  Shader::enableTextures(rs, meshTextures_);
  rs->vao().push(vao_->id());
  (inputContainer_.get()->*draw_)(primitive_);
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
