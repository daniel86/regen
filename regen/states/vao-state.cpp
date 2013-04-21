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

void VAOState::updateVAO(RenderState *rs, Mesh *mesh, GLuint arrayBuffer)
{
  const ShaderInputState::InputContainer &x = mesh->inputs();

  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  rs->vao().push(vao_->id());
  glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
  for(ShaderInputState::InputContainer::const_iterator
      it=x.begin(); it!=x.end(); ++it)
  {
    const ref_ptr<ShaderInput> in = it->in_;
    if(!in->isVertexAttribute()) continue;
    GLint loc = shader_->shader()->attributeLocation(it->name_);
    if(loc<0) continue;
    in->enableAttribute(loc);
  }
  rs->vao().pop();
}

void VAOState::updateVAO(RenderState *rs, Mesh *mesh)
{
  const ShaderInputState::InputContainer &x = mesh->inputs();

  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  rs->vao().push(vao_->id());
  for(ShaderInputState::InputContainer::const_iterator
      it=x.begin(); it!=x.end(); ++it)
  {
    const ref_ptr<ShaderInput> in = it->in_;
    if(!in->isVertexAttribute()) continue;
    GLint loc = shader_->shader()->attributeLocation(it->name_);
    if(loc<0) continue;
    glBindBuffer(GL_ARRAY_BUFFER, in->buffer());
    in->enableAttribute(loc);
  }
  if(mesh->numIndices()>0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer());
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
