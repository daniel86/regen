/*
 * attribute-less-mesh.h
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#ifndef ATTRIBUTE_LESS_MESH_H_
#define ATTRIBUTE_LESS_MESH_H_

#include <ogle/meshes/mesh-state.h>
#include <ogle/gl-types/vao.h>

/**
 * Mesh that can be used when no vertex shader input
 * is supposed to be used.
 */
class AttributeLessMesh : public MeshState
{
public:
  AttributeLessMesh(GLuint numVertices);

  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);
protected:
  ref_ptr<VertexArrayObject> vao_;
};

#endif /* ATTRIBUTE_LESS_MESH_H_ */
