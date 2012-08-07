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
#include <ogle/gl-types/uniform.h>

class Camera : public State
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

  void updateMatrix(GLfloat dt);

  /**
   * Matrix projecting world space to view space.
   */
  void set_viewMatrix(const Mat4f &viewMatrix);
  /**
   * Matrix projecting world space to view space.
   */
  const Mat4f& viewMatrix() const;

  /**
   * Inverse of the matrix projecting world space to view space.
   */
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

  const Vec3f& lastPosition() const;
  /**
   */
  const Vec3f& velocity() const;

  /**
   * Model view matrix of the camera.
   */
  UniformMat4* viewMatrixUniform() {
    return viewMatrixUniform_.get();
  }
  /**
   * VIEW^(-1)
   */
  UniformMat4* inverseViewMatrixUniform() {
    return inverseViewMatrixUniform_.get();
  }

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

  void set_isAudioListener(GLboolean useAudio);

protected:
  Mat4f inverseViewMatrix_;
  Mat4f viewMatrix_;

  Vec3f position_;
  Vec3f direction_;

  Vec3f lastPosition_;

  ref_ptr<UniformMat4> viewMatrixUniform_;
  ref_ptr<UniformMat4> inverseViewMatrixUniform_;
  ref_ptr<UniformVec3> cameraPositionUniform_;
  ref_ptr<UniformVec3> velocity_;

  GLfloat sensitivity_;
  GLfloat walkSpeed_;

  GLboolean isAudioListener_;
};

#endif /* _CAMERA_H_ */
