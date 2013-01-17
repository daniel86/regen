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
  void updateProjection(GLfloat right, GLfloat top);
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
  virtual void update(GLdouble dt);
  /**
   * Update the matrices.
   * Could be called from an animation thread.
   */
  void updatePerspective(GLdouble dt);

  /**
   * Sets the camera projection parameters: field of view, near and far distance
   * and aspect ratio.
   */
  void updateProjection(GLfloat fov, GLfloat near, GLfloat far, GLfloat aspect);

  void set_viewMatrix(const Mat4f &viewMatrix);
  const Mat4f& viewMatrix() const;
  const ref_ptr<ShaderInputMat4>& viewUniform() const;

  const Mat4f& viewProjectionMatrix() const;
  const ref_ptr<ShaderInputMat4>& viewProjectionUniform() const;

  const Mat4f& inverseViewProjectionMatrix() const;
  const ref_ptr<ShaderInputMat4>& inverseViewProjectionUniform() const;

  const Mat4f& inverseViewMatrix() const;
  const ref_ptr<ShaderInputMat4>& inverseViewUniform() const;

  const ref_ptr<ShaderInputMat4>& inverseProjectionUniform() const;

  /**
   * Camera aspect ratio.
   */
  GLdouble aspect() const;

  /**
   * Camera field of view.
   */
  GLfloat& fov() const;
  /**
   * Camera field of view.
   */
  const ref_ptr<ShaderInput1f>& fovUniform() const;

  /**
   * Camera near plane distance.
   */
  GLfloat& near() const;
  /**
   * Camera near plane distance.
   */
  const ref_ptr<ShaderInput1f>& nearUniform() const;

  /**
   * Camera far plane distance.
   */
  GLfloat& far() const;
  /**
   * Camera far plane distance.
   */
  const ref_ptr<ShaderInput1f>& farUniform() const;

  /**
   * Position of the camera in world space.
   */
  void set_position(const Vec3f &position);
  /**
   * Position of the camera in world space.
   */
  const Vec3f& position() const;
  /**
   * Position of the camera in world space.
   */
  const ref_ptr<ShaderInput3f>& positionUniform() const;

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
   * Camera velocity.
   */
  const ref_ptr<ShaderInput3f>& velocityUniform() const;

  /**
   * Rotates camera by specified amount.
   */
  void rotate(GLfloat xAmplitude, GLfloat yAmplitude, GLdouble deltaT);
  /**
   * Translates camera by specified amount.
   */
  void translate(Direction direction, GLdouble deltaT);

  /**
   * Sensitivity of movement.
   */
  GLfloat sensitivity() const;
  /**
   * Sensitivity of movement.
   */
  void set_sensitivity(GLfloat sensitivity);

  /**
   * Speed of movement.
   */
  GLfloat walkSpeed() const;
  /**
   * Speed of movement.
   */
  void set_walkSpeed(float walkSpeed);

  /**
   * Sets this camera to be the audio listener.
   * This is an exclusive state.
   */
  void set_isAudioListener(GLboolean useAudio);

  // override
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
