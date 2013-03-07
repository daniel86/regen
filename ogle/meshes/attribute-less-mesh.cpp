/*
 * attribute-less-mesh.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include "attribute-less-mesh.h"
using namespace ogle;

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
: MeshState(GL_POINTS)
{
  vao_ = ref_ptr<VertexArrayObject>::manage(new VertexArrayObject);
  numVertices_ = numVertices;
}

void AttributeLessMesh::enable(RenderState *rs)
{
  vao_->bind();
  MeshState::enable(rs);
}

void AttributeLessMesh::disable(RenderState *rs)
{
  MeshState::disable(rs);
  vao_->unbind();
}
