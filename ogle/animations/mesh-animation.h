/*
 * mesh-animation-gpu.h
 *
 *  Created on: 21.08.2012
 *      Author: daniel
 */

#ifndef MESH_ANIMATION_GPU_H_
#define MESH_ANIMATION_GPU_H_

#include <ogle/animations/animation.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/gl-types/shader.h>
#include <ogle/meshes/mesh-state.h>

namespace ogle {

/**
 * A frame in the mesh animation.
 */
struct MeshKeyFrame
{
  /**
   * Destination attributes.
   */
  list< ShaderAttributeLocation > attributes;
  /**
   * Number of ticks the frame should animate.
   */
  GLdouble timeInTicks;
  /**
   * First tick of this frame.
   * (Set by addFrame())
   */
  GLdouble startTick;
  /**
   * Last tick of this frame.
   * (Set by addFrame())
   */
  GLdouble endTick;
};

/**
 * Interpolation is done in a GLSL shader.
 * Different modes can be accessed by their name.
 * This struct is used to select the interpolation mode
 * used in the generated shader.
 */
struct AnimInterpoation {
  // attribute to interpolate
  string attributeName;
  // name of the interpolation
  string interpolationName;
  // include path for the interpolation GLSL code
  string interpolationKey;
  AnimInterpoation(const string &a_name, const string &i_name)
  : attributeName(a_name), interpolationName(i_name), interpolationKey("") {}
  AnimInterpoation(const string &a_name, const string &i_name, const string &i_key)
  : attributeName(a_name), interpolationName(i_name), interpolationKey(i_key) {}
};

/**
 * Animates vertex attributes.
 */
class MeshAnimation : public Animation
{
public:
  /**
   * The active tick range reached the final position;
   */
  static GLuint ANIMATION_STOPPED;

  MeshAnimation(const ref_ptr<MeshState> &mesh,
      list<AnimInterpoation> &interpolations);

  const ref_ptr<Shader>& interpolationShader() const;

  /**
   * Set the active tick range.
   * This resets some internal states and the animation will continue
   * next step with the start tick og the range.
   */
  void setTickRange(const Vec2d &tickRange);

  /**
   * Add a custom mesh frame.
   */
  void addFrame(
      const list< ref_ptr<VertexAttribute> > &attributes,
      GLdouble timeInTicks);

  /**
   * Add a frame for the original mesh attributes.
   */
  void addMeshFrame(GLdouble timeInTicks);

  /**
   * Projects each vertex of the mesh to a sphere.
   */
  void addSphereAttributes(
      GLfloat horizontalRadius,
      GLfloat verticalRadius,
      GLdouble timeInTicks);

  /**
   * Projects each vertex of the mesh to a box.
   */
  void addBoxAttributes(
      GLfloat width,
      GLfloat height,
      GLfloat depth,
      GLdouble timeInTicks);

  // override
  virtual void glAnimate(RenderState *rs, GLdouble dt);
  virtual void animate(GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;
protected:
  ref_ptr<Shader> interpolationShader_;
  ShaderInput1f *frameTimeUniform_;
  ShaderInput1f *frictionUniform_;
  ShaderInput1f *frequencyUniform_;

  ref_ptr<MeshState> mesh_;
  GLuint meshBufferOffset_;

  GLint lastFrame_, nextFrame_;

  ref_ptr<VertexBufferObject> feedbackBuffer_;

  ref_ptr<VertexBufferObject> animationBuffer_;
  GLint pingFrame_, pongFrame_;
  VBOBlockIterator pingIt_;
  VBOBlockIterator pongIt_;
  vector<MeshKeyFrame> frames_;

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
};

} // end ogle namespace

#endif /* MESH_ANIMATION_H_ */
