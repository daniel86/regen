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

Mesh::Mesh(GLenum primitive,
    const ref_ptr<ShaderInputContainer> &inputs)
: State(),
  HasInput(inputs),
  primitive_(primitive),
  feedbackCount_(0)
{
  vao_ = ref_ptr<VAO>::alloc();
  hasInstances_ = GL_FALSE;
  draw_ = &ShaderInputContainer::drawArrays;
  set_primitive(primitive);
}

Mesh::Mesh(const ref_ptr<Mesh> &meshResource)
: State(meshResource),
  HasInput(meshResource->inputContainer()),
  primitive_(meshResource->primitive_),
  feedbackCount_(0)
{
  vao_ = ref_ptr<VAO>::alloc();
  hasInstances_ = GL_FALSE;
  draw_ = meshResource->draw_;
  set_primitive(primitive_);
}

Mesh::Mesh(GLenum primitive, VBO::Usage usage)
: State(),
  HasInput(usage),
  primitive_(primitive),
  feedbackCount_(0)
{
  vao_ = ref_ptr<VAO>::alloc();
  hasInstances_ = GL_FALSE;
  draw_ = &ShaderInputContainer::drawArrays;
  set_primitive(primitive);
}

void Mesh::addShaderInput(const string &name, const ref_ptr<ShaderInput> &in)
{
  if(!meshShader_.get()) return;

  if(in->isVertexAttribute()) {
    GLint loc = meshShader_->attributeLocation(name);
    if(loc==-1) {
      // not used in shader
      return;
    }
    if(!in->bufferIterator().get()) {
      // allocate VBO memory if not already allocated
      inputContainer_->inputBuffer()->alloc(in);
    }
    if(in->numInstances()>1) {
      inputContainer_->set_numInstances(in->numInstances());
    }

    map<GLint, list<ShaderInputLocation>::iterator>::iterator needle = vaoLocations_.find(loc);
    if(needle == vaoLocations_.end()) {
      vaoAttributes_.push_back(ShaderInputLocation(in,loc));
      list<ShaderInputLocation>::iterator it = vaoAttributes_.end();
      --it;
      vaoLocations_[loc] = it;
    }
    else {
      *needle->second = ShaderInputLocation(in,loc);
    }
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
    if(loc==-1) {
      // not used in shader
      return;
    }
    meshUniforms_[loc] = ShaderInputLocation(in,loc);
  }
}

const ref_ptr<VAO>& Mesh::vao() const
{ return vao_; }

void Mesh::updateVAO(
    RenderState *rs,
    const StateConfig &cfg,
    const ref_ptr<Shader> &meshShader)
{
  // remember the shader
  meshShader_ = meshShader;
  hasInstances_ = GL_FALSE;

  // reset attribute list
  vaoAttributes_.clear();
  vaoLocations_.clear();
  meshUniforms_.clear();
  // and load from Config
  for(ShaderInputList::const_iterator
      it=cfg.inputs_.begin(); it!=cfg.inputs_.end(); ++it)
  { addShaderInput(it->name_, it->in_); }
  // Get input from mesh and joined states (might be handled by StateConfig allready)
  ShaderInputList localInputs;
  collectShaderInput(localInputs);
  for(ShaderInputList::const_iterator it=localInputs.begin(); it!=localInputs.end(); ++it)
  { addShaderInput(it->name_, it->in_); }
  // Add Textures
  for(map<string, ref_ptr<Texture> >::const_iterator
      it=cfg.textures_.begin(); it!=cfg.textures_.end(); ++it)
  { addShaderInput(it->first, it->second); }

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
  vao_->resetGL();
  rs->vao().push(vao_->id());
  // Setup attributes
  for(list<ShaderInputLocation>::const_iterator
      it=vaoAttributes_.begin(); it!=vaoAttributes_.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = it->input;
    if(lastArrayBuffer!=in->buffer()) {
      lastArrayBuffer = in->buffer();
      glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    }
    in->enableAttribute(it->location);
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
    feedbackState_ = ref_ptr<FeedbackState>::alloc(feedbackPrimitive_, feedbackCount_);
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
    // XXX: problems for shared shader....
    //if(x.input->stamp() != x.uploadStamp && x.input->active()) {
      x.input->enableUniform(x.location);
      x.uploadStamp = x.input->stamp();
    //}
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
: Mesh(GL_POINTS, VBO::USAGE_STATIC)
{
  inputContainer_->set_numVertices(numVertices);
}
