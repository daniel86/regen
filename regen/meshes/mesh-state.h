/*
 * mesh-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef __MESH_H_
#define __MESH_H_

#include <vector>

#include <regen/states/state.h>
#include <regen/states/feedback-state.h>
#include <regen/gl-types/shader-input-container.h>
#include <regen/gl-types/vbo.h>
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
     * Shallow copy constructor.
     * Vertex data is not copied.
     * @param meshResource another mesh that provides vertex data.
     */
    Mesh(const ref_ptr<Mesh> &meshResource);
    /**
     * @param primitive Specifies what kind of primitives to render.
     * @param usage VBO usage.
     */
    Mesh(GLenum primitive, VBO::Usage usage);
    ~Mesh();

    /**
     * @param out Set of meshes using the ShaderInputcontainer of this mesh
     *          (meshes created by copy constructor).
     */
    void getMeshViews(std::set<Mesh*> &out);

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
    void updateVAO(
         RenderState *rs,
         const StateConfig &cfg,
         const ref_ptr<Shader> &shader);
    /**
     * Update VAO using last StateConfig.enable.
     * @param rs the render state.
     */
    void updateVAO(RenderState *rs);

    /**
     * @return VAO that is used to render from array data.
     */
    const ref_ptr<VAO>& vao() const;

    /**
     * @return face primitive of this mesh.
     */
    GLenum primitive() const;
    /**
     * @param primitive face primitive of this mesh.
     */
    void set_primitive(GLenum primitive);

    /**
     * The center position of this mesh.
     */
    void set_centerPosition(const Vec3f &center);
    /**
     * Extends relative to center position.
     */
    void set_extends(const Vec3f &minPosition, const Vec3f &maxPosition);

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
     * @return the bone indices attribute.
     */
    ref_ptr<ShaderInput> boneIndices() const;
    /**
     * @return the bone weights attribute.
     */
    ref_ptr<ShaderInput> boneWeights() const;

    /**
     * The center position of this mesh.
     */
    const Vec3f& centerPosition();
    /**
     * Minimum extends relative to center position.
     */
    const Vec3f& minPosition();
    /**
     * Maximum extends relative to center position.
     */
    const Vec3f& maxPosition();

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

    ref_ptr<VAO> vao_;

    std::list<ShaderInputLocation> vaoAttributes_;
    std::map<GLint,std::list<ShaderInputLocation>::iterator> vaoLocations_;

    ref_ptr<Shader> meshShader_;
    std::map<GLint, ShaderInputLocation> meshUniforms_;

    GLboolean hasInstances_;

    GLenum feedbackPrimitive_;
    ref_ptr<FeedbackState> feedbackState_;
    GLuint feedbackCount_;

    ref_ptr<Mesh> sourceMesh_;
    std::set<Mesh*> meshViews_;
    GLboolean isMeshView_;

    Vec3f centerPosition_;
    Vec3f minPosition_;
    Vec3f maxPosition_;

    void (ShaderInputContainer::*draw_)(GLenum);
    void updateDrawFunction();

    void addShaderInput(const std::string &name, const ref_ptr<ShaderInput> &in);
  };
  typedef std::vector< ref_ptr<Mesh> > MeshVector;
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
