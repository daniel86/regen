/*
 * mesh-animation-gpu.h
 *
 *  Created on: 21.08.2012
 *      Author: daniel
 */

#ifndef MESH_ANIMATION_GPU_H_
#define MESH_ANIMATION_GPU_H_

#include <regen/animations/animation.h>
#include <regen/gl-types/vertex-attribute.h>
#include <regen/gl-types/shader.h>
#include <regen/meshes/mesh-state.h>

namespace ogle {
/**
 * \brief Animates vertex attributes.
 *
 * The animation is done using key frames. At least
 * two frames are loaded into VRAM during animation.
 * A shader is used to compute an interpolation between those
 * two frames. Transform feedback is used to read
 * the interpolated value into a feedback buffer.
 * This interpolated value is used by meshes during
 * regular rendering.
 */
class MeshAnimation : public Animation
{
public:
  /**
   * Interpolation is done in a GLSL shader.
   * Different modes can be accessed by their name.
   * This struct is used to select the interpolation mode
   * used in the generated shader.
   */
  struct Interpoation {
    string attributeName; /**< attribute to interpolate. */
    string interpolationName; /**< name of the interpolation. */
    string interpolationKey; /**< include path for the interpolation GLSL code. */

    /**
     * @param a_name attribute name.
     * @param i_name interpolation mode name.
     */
    Interpoation(const string &a_name, const string &i_name)
    : attributeName(a_name), interpolationName(i_name), interpolationKey("") {}
    /**
     * @param a_name attribute name.
     * @param i_name interpolation mode name.
     * @param i_key interpolation include key.
     */
    Interpoation(const string &a_name, const string &i_name, const string &i_key)
    : attributeName(a_name), interpolationName(i_name), interpolationKey(i_key) {}
  };

  /**
   * The active tick range reached the final position;
   */
  static GLuint ANIMATION_STOPPED;

  /**
   * @param mesh a mesh.
   * @param interpolations list of interpolation modes for attributes.
   */
  MeshAnimation(const ref_ptr<Mesh> &mesh, list<Interpoation> &interpolations);

  /**
   * @return the shader used for interpolationg beteen frames.
   */
  const ref_ptr<Shader>& interpolationShader() const;

  /**
   * Set the active tick range.
   * This resets some internal states and the animation will continue
   * next step with the start tick of the given range.
   * @param tickRange number of ticks for this morph.
   */
  void setTickRange(const Vec2d &tickRange);

  /**
   * Add a custom mesh frame.
   * @param attributes target attributes.
   * @param timeInTicks number of ticks for this morph.
   */
  void addFrame(
      const list< ref_ptr<VertexAttribute> > &attributes,
      GLdouble timeInTicks);

  /**
   * Add a frame for the original mesh attributes.
   * @param timeInTicks number of ticks for this morph.
   */
  void addMeshFrame(GLdouble timeInTicks);

  /**
   * Projects each vertex of the mesh to a sphere.
   * @param horizontalRadius horizontal sphere radius.
   * @param verticalRadius vertical sphere radius.
   * @param timeInTicks number of ticks for this morph.
   */
  void addSphereAttributes(
      GLfloat horizontalRadius,
      GLfloat verticalRadius,
      GLdouble timeInTicks);

  /**
   * Projects each vertex of the mesh to a box.
   * @param width box width.
   * @param height box height.
   * @param depth box depth.
   * @param timeInTicks number of ticks for this morph.
   */
  void addBoxAttributes(
      GLfloat width,
      GLfloat height,
      GLfloat depth,
      GLdouble timeInTicks);

  // override
  void glAnimate(RenderState *rs, GLdouble dt);

protected:
  struct KeyFrame {
    list< ShaderAttributeLocation > attributes;
    GLdouble timeInTicks;
    GLdouble startTick;
    GLdouble endTick;
  };
  struct ContiguousBlock {
    ContiguousBlock(const ref_ptr<ShaderInput> &in)
    : buffer(in->buffer()), offset(in->offset()), size(in->size()) {}
    GLuint buffer;
    GLuint offset;
    GLuint size;
  };

  ref_ptr<Shader> interpolationShader_;
  ShaderInput1f *frameTimeUniform_;
  ShaderInput1f *frictionUniform_;
  ShaderInput1f *frequencyUniform_;

  ref_ptr<Mesh> mesh_;
  GLuint meshBufferOffset_;

  GLint lastFrame_, nextFrame_;

  ref_ptr<VertexBufferObject> feedbackBuffer_;

  ref_ptr<VertexBufferObject> animationBuffer_;
  GLint pingFrame_, pongFrame_;
  VBOBlockIterator pingIt_;
  VBOBlockIterator pongIt_;
  vector<KeyFrame> frames_;

  // milliseconds from start of animation
  GLdouble elapsedTime_;
  GLdouble ticksPerSecond_;
  GLdouble lastTime_;
  Vec2d tickRange_;
  GLuint lastFramePosition_;
  GLuint startFramePosition_;

  GLuint mapOffset_, mapSize_;

  GLboolean hasMeshInterleavedAttributes_;

  void loadFrame(GLuint frameIndex, GLboolean isPongFrame);
  ref_ptr<VertexAttribute> findLastAttribute(const string &name);

  static void findFrameAfterTick(
      GLdouble tick, GLint &frame, vector<KeyFrame> &keys);
  static void findFrameBeforeTick(
      GLdouble &tick, GLuint &frame, vector<KeyFrame> &keys);
};

} // end ogle namespace

#endif /* MESH_ANIMATION_H_ */
