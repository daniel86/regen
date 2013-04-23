/*
 * vao-state.cpp
 *
 *  Created on: 21.04.2013
 *      Author: daniel
 */

#include <regen/meshes/mesh-state.h>

#include "vao-state.h"
using namespace regen;

VAOState::VAOState(const ref_ptr<ShaderState> &shader)
: State(), shader_(shader)
{
}

const ref_ptr<VertexArrayObject>& VAOState::vao() const
{ return vao_; }
void VAOState::set_vao(const ref_ptr<VertexArrayObject> &vao)
{ vao_ = vao; }

void VAOState::readAttributes(ShaderInputState *mesh, list<ShaderInputLocation> &attributes)
{
  const ref_ptr<Shader> &shader = shader_->shader();
  const ShaderInputState::InputContainer &x = mesh->inputs();
  const list<ShaderInputLocation> &parentAttributes = shader->attributes();

  for(list<ShaderInputLocation>::const_iterator
      it=parentAttributes.begin(); it!=parentAttributes.end(); ++it)
  {
    if(mesh->hasInput(it->input->name())) continue;
    attributes.push_back(*it);
  }
  for(ShaderInputState::InputContainer::const_iterator
      it=x.begin(); it!=x.end(); ++it)
  {
    const ref_ptr<ShaderInput> in = it->in_;
    if(!in->isVertexAttribute()) continue;
    GLint loc = shader->attributeLocation(it->name_);
    if(loc<0) continue;
    attributes.push_back(ShaderInputLocation(in,loc));
  }
}

void VAOState::updateVAO(RenderState *rs, Mesh *mesh, GLuint arrayBuffer)
{
  list<ShaderInputLocation> attributes;
  readAttributes(mesh,attributes);

  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  rs->vao().push(vao_->id());

  glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
  if(mesh->numIndices()>0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer());
  }
  for(list<ShaderInputLocation>::const_iterator
      it=attributes.begin(); it!=attributes.end(); ++it)
  {
    const ref_ptr<ShaderInput> in = it->input;
    in->enableAttribute(it->location);
  }

  rs->vao().pop();
}

void VAOState::updateVAO(RenderState *rs, Mesh *mesh)
{
  list<ShaderInputLocation> attributes;
  readAttributes(mesh,attributes);

  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  rs->vao().push(vao_->id());

  if(mesh->numIndices()>0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer());
  }
  for(list<ShaderInputLocation>::const_iterator
      it=attributes.begin(); it!=attributes.end(); ++it)
  {
    const ref_ptr<ShaderInput> in = it->input;
    glBindBuffer(GL_ARRAY_BUFFER, in->buffer());
    in->enableAttribute(it->location);
  }

  rs->vao().pop();
}

void VAOState::enable(RenderState *rs)
{
  State::enable(rs);
  rs->vao().push(vao_->id());
}

void VAOState::disable(RenderState *rs)
{
  rs->vao().pop();
  State::disable(rs);
}
