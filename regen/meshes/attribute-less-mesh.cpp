/*
 * attribute-less-mesh.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include "attribute-less-mesh.h"
using namespace regen;

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
: Mesh(GL_POINTS)
{
  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  numVertices_ = numVertices;
}

void AttributeLessMesh::enable(RenderState *rs)
{
  vao_->bind();
  Mesh::enable(rs);
}

void AttributeLessMesh::disable(RenderState *rs)
{
  Mesh::disable(rs);
  vao_->unbind();
}
