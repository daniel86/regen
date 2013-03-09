/*
 * camera.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <ogle/states/shader-input-state.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/matrix.h>

namespace ogle {
/**
 * \brief Camera with perspective projection.
 */
class Camera : public ShaderInputState
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

  Camera();

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

  const Mat4f& projection() const;
  const ref_ptr<ShaderInputMat4>& inverseProjectionUniform() const;

  ShaderInputMat4* projectionUniform();
  ShaderInputMat4* viewProjectionUniform();

  /**
   * Camera aspect ratio.
   */
  GLdouble aspect() const;

  /**
   * Camera field of view.
   */
  GLfloat fov() const;
  /**
   * Camera field of view.
   */
  const ref_ptr<ShaderInput1f>& fovUniform() const;

  /**
   * Camera near plane distance.
   */
  GLfloat near() const;
  /**
   * Camera near plane distance.
   */
  const ref_ptr<ShaderInput1f>& nearUniform() const;

  /**
   * Camera far plane distance.
   */
  GLfloat far() const;
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

protected:
  Vec3f position_;
  Vec3f lastPosition_;

  Vec3f direction_;

  GLfloat fov_;
  GLfloat near_;
  GLfloat far_;

  Mat4f proj_;
  Mat4f projInv_;
  Mat4f view_;
  Mat4f viewInv_;
  Mat4f viewproj_;
  Mat4f viewprojInv_;

  GLfloat sensitivity_;
  GLfloat walkSpeed_;
  GLfloat aspect_;

  GLboolean isAudioListener_;
  GLboolean projectionChanged_;

  ref_ptr<ShaderInputMat4> u_view_;
  ref_ptr<ShaderInputMat4> u_viewInv_;
  ref_ptr<ShaderInputMat4> u_proj_;
  ref_ptr<ShaderInputMat4> u_projInv_;
  ref_ptr<ShaderInputMat4> u_viewproj_;
  ref_ptr<ShaderInputMat4> u_viewprojInv_;

  ref_ptr<ShaderInput3f> u_position_;
  ref_ptr<ShaderInput1f> u_fov_;
  ref_ptr<ShaderInput1f> u_near_;
  ref_ptr<ShaderInput1f> u_far_;
  ref_ptr<ShaderInput3f> u_vel_;
};
} // end ogle namespace

#endif /* _CAMERA_H_ */
