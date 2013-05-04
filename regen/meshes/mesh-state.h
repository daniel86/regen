/*
 * mesh-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef __MESH_H_
#define __MESH_H_

#include <regen/states/state.h>
#include <regen/states/feedback-state.h>
#include <regen/gl-types/shader-input-container.h>
#include <regen/gl-types/vbo.h>
#include <regen/gl-types/vao.h>
#include <regen/gl-types/shader.h>

namespace regen {
/**
 * \brief A collection of vertices, edges and faces that defines the shape of an object in 3D space.
 *
 * When this State is enabled the actual draw call is done. Make sure to setup shader
 * and server side states before.
 */
class Mesh : public State, public HasInput
{
public:
  /**
   * @param primitive Specifies what kind of primitives to render.
   * @param inputs custom input container.
   */
  Mesh(GLenum primitive, const ref_ptr<ShaderInputContainer> &inputs);
  /**
   * @param primitive Specifies what kind of primitives to render.
   * @param usage VBO usage.
   */
  Mesh(GLenum primitive, VertexBufferObject::Usage usage);

  /**
   * @param layout Start recording added inputs.
   */
  void begin(ShaderInputContainer::DataLayout layout);
  /**
   * Finish previous call to begin(). All recorded inputs are
   * uploaded to VBO memory. And the mesh VAO is updated.
   */
  void end();

  /**
   * Update VAO that is used to render from array data.
   * And setup uniforms and textures not handled in Shader class.
   * Basically all uniforms and textures declared as parent nodes of
   * a Shader instance are auto-enabled by that Shader. All remaining uniforms
   * and textures are activated in Mesh::enable.
   * @param rs the render state.
   * @param cfg the state configuration.
   * @param shader the mesh shader.
   */
  void initializeResources(
      RenderState *rs,
      const Config &cfg,
      const ref_ptr<Shader> &shader);

  /**
   * @return VAO that is used to render from array data.
   */
  const ref_ptr<VertexArrayObject>& vao() const;
  /**
   * @param vao VAO that is used to render from array data.
   */
  void set_vao(const ref_ptr<VertexArrayObject> &vao);

  /**
   * @return face primitive of this mesh.
   */
  GLenum primitive() const;
  /**
   * @param primitive face primitive of this mesh.
   */
  void set_primitive(GLenum primitive);

  /**
   * @return the position attribute.
   */
  ref_ptr<ShaderInput> positions() const;
  /**
   * @return the normal attribute.
   */
  ref_ptr<ShaderInput> normals() const;
  /**
   * @return the color attribute.
   */
  ref_ptr<ShaderInput> colors() const;

  /**
   * Add attributes to capture to this state.
   * @return the transform feedback state.
   */
  const ref_ptr<FeedbackState>& feedbackState();

  // override
  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

protected:
  GLenum primitive_;

  ref_ptr<VertexArrayObject> vao_;
  ref_ptr<Shader> meshShader_;
  list<ShaderInputLocation> meshAttributes_;
  list<ShaderInputLocation> meshUniforms_;
  list<ShaderTextureLocation> meshTextures_;
  GLboolean hasInstances_;

  GLenum feedbackPrimitive_;
  ref_ptr<FeedbackState> feedbackState_;
  GLuint feedbackCount_;

  void (ShaderInputContainer::*draw_)(GLenum);

  void updateVAO(RenderState *rs);
  void updateDrawFunction();
};
} // namespace

namespace regen {
  /**
   * \brief Mesh that can be used when no vertex shader input
   * is required.
   *
   * This effectively means that you have to generate
   * geometry that will be rastarized.
   */
  class AttributeLessMesh : public Mesh
  {
  public:
    /**
     * @param numVertices number of vertices used.
     */
    AttributeLessMesh(GLuint numVertices);
  };
} // namespace

#endif /* __MESH_H_ */
