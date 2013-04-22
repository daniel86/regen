/*
 * vao-state.h
 *
 *  Created on: 21.04.2013
 *      Author: daniel
 */

#ifndef VAO_STATE_H_
#define VAO_STATE_H_

#include <regen/states/state.h>
#include <regen/states/shader-state.h>
#include <regen/meshes/mesh-state.h>
#include <regen/gl-types/vao.h>
#include <regen/gl-types/shader.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
/**
 * \brief Vertex Array Objects (VAO) are OpenGL Objects that store the
 * set of bindings between Vertex Attributes and the user's source vertex data.
 */
class VAOState : public State
{
public:
  /**
   * @param shader the shader that defines the attribute locations.
   */
  VAOState(const ref_ptr<ShaderState> &shader);

  /**
   * Load vertex specification defined by mesh attributes.
   */
  void updateVAO(RenderState *rs, Mesh *mesh);
  /**
   * Load vertex specification defined by mesh attributes.
   * Force given vertex array buffer.
   */
  void updateVAO(RenderState *rs, Mesh *mesh, GLuint arrayBuffer);

  /**
   * @return the VAO.
   */
  const ref_ptr<VertexArrayObject>& vao() const;
  /**
   * @param v the VAO.
   */
  void set_vao(const ref_ptr<VertexArrayObject> &v);

  // override
  void enable(RenderState *rs);
  void disable(RenderState *rs);

protected:
  ref_ptr<ShaderState> shader_;
  ref_ptr<VertexArrayObject> vao_;

  void readAttributes(ShaderInputState *mesh, list<ShaderInputLocation> &attributes);
};
} // namespace

#endif /* VAO_STATE_H_ */
