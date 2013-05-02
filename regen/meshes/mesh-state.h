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
#include <regen/gl-types/vertex-attribute.h>
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
  Mesh(GLenum primitive, const ref_ptr<ShaderInputContainer> &inputs);
  /**
   * @param primitive face primitive of this mesh.
   * @param usage VBO usage.
   */
  Mesh(GLenum primitive, VertexBufferObject::Usage usage);

  void beginUpload(ShaderInputContainer::DataLayout layout);
  void endUpload();

  void updateVAO(
      RenderState *rs,
      const Config &cfg,
      const ref_ptr<Shader> &meshShader);
  void updateVAO(RenderState *rs);
  const ref_ptr<VertexArrayObject>& vao() const;
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

  /**
   * Render primitives from array data.
   */
  void draw(GLuint numInstances);

  // override
  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

protected:
  GLenum primitive_;

  ref_ptr<VertexArrayObject> vao_;
  ref_ptr<Shader> meshShader_;
  list<ShaderInputLocation> meshAttributes_;

  GLenum feedbackPrimitive_;
  ref_ptr<FeedbackState> feedbackState_;
  GLuint feedbackCount_;

  void (Mesh::*draw_)(GLuint numInstances);
  void drawArrays(GLuint numInstances);
  void drawElements(GLuint numInstances);
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
