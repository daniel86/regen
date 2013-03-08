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

namespace ogle {
/**
 * \brief Mesh that can be used when no vertex shader input
 * is required.
 *
 * This effectively means that you have to generate
 * geometry that will be rastarized.
 */
class AttributeLessMesh : public MeshState
{
public:
  AttributeLessMesh(GLuint numVertices);

  // override
  void enable(RenderState *rs);
  void disable(RenderState *rs);

protected:
  ref_ptr<VertexArrayObject> vao_;
};
} // end ogle namespace

#endif /* ATTRIBUTE_LESS_MESH_H_ */
