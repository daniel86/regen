/*
 * mesh-animation.h
 *
 *  Created on: 21.08.2012
 *      Author: daniel
 */

#ifndef MESH_ANIMATION_H_
#define MESH_ANIMATION_H_

#include <ogle/animations/animation.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/states/mesh-state.h>

/**
 * Interpolates between frames.
 */
class VertexInterpolator
{
public:
  VertexInterpolator();

  /**
   * Do the interpolation.
   */
  virtual ref_ptr<VertexAttribute> operator()(
      ref_ptr<VertexAttribute> &previous,
      ref_ptr<VertexAttribute> &next,
      GLdouble frameTimeNormalized) = 0;
};

/**
 * A frame in the mesh animation.
 */
struct MeshKeyFrame
{
  /**
   * Destination attributes.
   */
  list< ref_ptr<VertexAttribute> > attributes;
  /**
   * Interpolation between this and last frame.
   */
  ref_ptr<VertexInterpolator> interpolator;
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
  /**
   * Attributes of previous frames.
   * (Set by addFrame())
   */
  map< string, ref_ptr<VertexAttribute> > previousAttributes;
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

  MeshAnimation(ref_ptr<MeshState> &mesh);

  /**
   * Set the active tick range.
   * This resets some internal states and the animation will continue
   * next step with the start tick og the range.
   */
  void setTickRange(const Vec2d &tickRange);

  /**
   * Add a custom mesh frame.
   */
  void addFrame(MeshKeyFrame &frame);

  /**
   * Use vertex data as is.
   */
  void addMeshAttribute(MeshKeyFrame &frame, const string &attributeName);

  /**
   * Projects each vertex of the mesh to a sphere.
   */
  void addSphereAttributes(
      MeshKeyFrame &frame,
      GLfloat horizontalRadius,
      GLfloat verticalRadius);

  /**
   * Projects each vertex of the mesh to a box.
   */
  void addBoxAttributes(
      MeshKeyFrame &frame,
      GLfloat width,
      GLfloat height,
      GLfloat depth);

  // override
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);
protected:
  ref_ptr<MeshState> mesh_;
  vector<MeshKeyFrame> frames_;

  map< string, ref_ptr<VertexAttribute> > attributes_;

  // milliseconds from start of animation
  GLdouble elapsedTime_;
  GLdouble ticksPerSecond_;
  GLdouble lastTime_;
  Vec2d tickRange_;
  GLuint lastFramePosition_;
  GLuint startFramePosition_;

  GLuint mapOffset_, mapSize_;
};

///////

/**
 * Linear interpolation of vertex data.
 */
class LinearVertexInterpolator : public VertexInterpolator
{
public:
  LinearVertexInterpolator();
  virtual ref_ptr<VertexAttribute> operator()(
      ref_ptr<VertexAttribute> &previous,
      ref_ptr<VertexAttribute> &next,
      GLdouble frameTimeNormalized);
};

/**
 * Takes values from nearest frame.
 */
class NearestVertexInterpolator : public VertexInterpolator
{
public:
  NearestVertexInterpolator();
  virtual ref_ptr<VertexAttribute> operator()(
      ref_ptr<VertexAttribute> &previous,
      ref_ptr<VertexAttribute> &next,
      GLdouble frameTimeNormalized);
};

/**
 * Interpolates using oscillation equation.
 */
class OscillateVertexInterpolator : public LinearVertexInterpolator
{
public:
  OscillateVertexInterpolator();
  virtual ref_ptr<VertexAttribute> operator()(
      ref_ptr<VertexAttribute> &previous,
      ref_ptr<VertexAttribute> &next,
      GLdouble frameTimeNormalized);

  void set_friction(GLdouble friction);
  void set_frequency(GLdouble frequency);
protected:
  GLdouble friction_;
  GLdouble frequency_;
};

#endif /* MESH_ANIMATION_H_ */
