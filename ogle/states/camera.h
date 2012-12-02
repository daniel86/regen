/*
 * camera.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <ogle/states/state.h>
#include <ogle/utility/callable.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/matrix.h>
#include <ogle/gl-types/shader-input.h>

/**
 * Base class for camera's.
 * Just provides the projection matrix.
 */
class Camera : public State
{
public:
  Camera();
  ShaderInputMat4* projectionUniform();
  /**
   * VIEW^(-1) * PROJECTION^(-1)
   */
  ShaderInputMat4* viewProjectionUniform();
protected:
  ref_ptr<ShaderInputMat4> projectionUniform_;
  ref_ptr<ShaderInputMat4> viewProjectionUniform_;
};

/**
 * Camera with orthogonal projection.
 */
class OrthoCamera : public Camera
{
public:
  OrthoCamera();
  void updateProjection(
      GLfloat right, GLfloat top);
};

/**
 * Provides view transformation related
 * uniforms (view matrix, camera position, ..).
 */
class PerspectiveCamera : public Camera
{
public:
  typedef enum {
    DIRECTION_UP,
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_FRONT,
    DIRECTION_BACK
  }Direction;

  PerspectiveCamera();

  /**
   * Update the uniform values.
   * Should be done in rendering thread.
   */
  virtual void update(GLfloat dt);
  /**
   * Update the matrices.
   * Could be called from an animation thread.
   */
  void updatePerspective(GLfloat dt);

  void updateProjection(
      GLfloat fov,
      GLfloat near,
      GLfloat far,
      GLfloat aspect);

  /**
   * Matrix projecting world space to view space.
   */
  void set_viewMatrix(const Mat4f &viewMatrix);
  /**
   * Matrix projecting world space to view space.
   */
  const Mat4f& viewMatrix() const;
  const Mat4f& viewProjectionMatrix() const;

  const Mat4f& inverseViewProjectionMatrix() const;
  const Mat4f& inverseViewMatrix() const;

  /**
   * Position of the camera in world space.
   */
  void set_position(const Vec3f &position);
  /**
   * Position of the camera in world space.
   */
  const Vec3f& position() const;

  /**
   * Direction of the camera.
   */
  const Vec3f& direction() const;
  /**
   * Direction of the camera.
   */
  void set_direction(const Vec3f &direction);

  /**
   * Camera velocity.
   */
  const Vec3f& velocity() const;

  /**
   * Model view matrix of the camera.
   */
  ref_ptr<ShaderInputMat4>& viewUniform();
  ref_ptr<ShaderInputMat4>& viewProjectionUniform();
  ref_ptr<ShaderInputMat4>& inverseViewProjectionUniform();
  ref_ptr<ShaderInputMat4>& inverseViewUniform();
  ref_ptr<ShaderInputMat4>& inverseProjectionUniform();

  ref_ptr<ShaderInput1f>& fovUniform();
  ref_ptr<ShaderInput1f>& nearUniform();
  ref_ptr<ShaderInput1f>& farUniform();
  ref_ptr<ShaderInput3f>& velocity();
  ref_ptr<ShaderInput3f>& positionUniform();

  /**
   * Rotates camera by specified amount.
   */
  void rotate(float xAmplitude, float yAmplitude, float deltaT);
  /**
   * Translates camera by specified amount.
   */
  void translate(Direction direction, float deltaT);

  /**
   * Sensitivity of movement.
   */
  float sensitivity() const;
  /**
   * Sensitivity of movement.
   */
  void set_sensitivity(float sensitivity);

  /**
   * Speed of movement.
   */
  float walkSpeed() const;
  /**
   * Speed of movement.
   */
  void set_walkSpeed(float walkSpeed);

  /**
   * Sets this camera to be the audio listener.
   * This is an exclusive state.
   */
  void set_isAudioListener(GLboolean useAudio);

  GLdouble aspect() const { return aspect_; }

  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);

protected:
  Vec3f position_;
  Vec3f direction_;

  Mat4f view_;
  Mat4f viewProjection_;
  Mat4f invView_;
  Mat4f invViewProjection_;

  Vec3f lastPosition_;
  Mat4f *lastViewMatrix_;
  Mat4f *lastProjectionMatrix_;

  GLfloat sensitivity_;
  GLfloat walkSpeed_;
  GLfloat aspect_;

  GLboolean isAudioListener_;

  ref_ptr<ShaderInputMat4> viewUniform_;
  ref_ptr<ShaderInputMat4> invViewUniform_;
  ref_ptr<ShaderInputMat4> invViewProjectionUniform_;
  ref_ptr<ShaderInputMat4> invProjectionUniform_;

  ref_ptr<ShaderInput3f> cameraPositionUniform_;
  ref_ptr<ShaderInput1f> fovUniform_;
  ref_ptr<ShaderInput1f> nearUniform_;
  ref_ptr<ShaderInput1f> farUniform_;
  ref_ptr<ShaderInput3f> velocity_;
};

#endif /* _CAMERA_H_ */
