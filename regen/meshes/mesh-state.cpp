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

void Mesh::addShaderInput(const string &name, const ref_ptr<ShaderInput> &in)
{
  if(!meshShader_.get()) return;

  if(in->isVertexAttribute()) {
    GLint loc = meshShader_->attributeLocation(name);
    if(loc==-1) return;
    if(!in->bufferIterator().get()) {
      // allocate VBO memory if not already allocated
      inputContainer_->inputBuffer()->alloc(in);
    }
    meshAttributes_[loc] = ShaderInputLocation(in,loc);
  }
  else if(!in->isConstant())
  {
    if(meshShader_->hasUniform(name) &&
        meshShader_->input(name).get()==in.get())
    {
      // shader handles uniform already.
      return;
    }
    GLint loc = meshShader_->uniformLocation(name);
    if(loc==-1) return;
    meshUniforms_[loc] = ShaderInputLocation(in,loc);
  }
}

void Mesh::begin(ShaderInputContainer::DataLayout layout)
{
  inputContainer_->begin(layout);
}
void Mesh::end()
{
  const ShaderInputList &newInputs = inputContainer_->uploadInputs();
  for(ShaderInputList::const_iterator it=newInputs.begin(); it!=newInputs.end(); ++it)
  { addShaderInput(it->name_, it->in_); }
  inputContainer_->end();
  updateVAO(RenderState::get());
  updateDrawFunction();
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
  { addShaderInput(it->first, it->second); }
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
  for(map<GLint, ShaderInputLocation>::const_iterator
      it=meshAttributes_.begin(); it!=meshAttributes_.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = it->second.input;
    if(lastArrayBuffer!=in->buffer()) {
      lastArrayBuffer = in->buffer();
      glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    }
    in->enableAttribute(it->first);
    if(in->numInstances()>1) hasInstances_=GL_TRUE;
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

  for(map<GLint, ShaderInputLocation>::iterator
      it=meshUniforms_.begin(); it!=meshUniforms_.end(); ++it)
  {
    ShaderInputLocation &x = it->second;
    if(x.input->stamp() != x.uploadStamp) {
      x.input->enableUniform(x.location);
      x.uploadStamp = x.input->stamp();
    }
  }

  for(map<GLint,ShaderTextureLocation>::iterator
      it=meshTextures_.begin(); it!=meshTextures_.end(); ++it)
  {
    ShaderTextureLocation &x = it->second;
    GLint &channel = *(x.channel);
    if(x.uploadChannel != channel) {
      glUniform1i(x.location, *(x.channel));
      x.uploadChannel = channel;
    }
  }

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
