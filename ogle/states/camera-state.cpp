/*
 * camera.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include "camera-state.h"
#include <ogle/algebra/vector.h>
#include <ogle/algebra/matrix.h>
#include <ogle/av/audio.h>

Camera::Camera()
: State(),
  isAudioListener_(false),
#ifdef UP_DIMENSION_Y
  direction_(Vec3f( 0, 0, -1 )),
  position_(Vec3f( 0.0, 1.0, 4.0 )),
  lastPosition_(Vec3f( 0.0, 1.0, 4.0 )),
#else
  position_(Vec3f( 0, 10, 0 )),
  lastPosition_(Vec3f( 0, 10, 0 )),
  direction_ = (Vec3f( 0, -1, 0 )),
#endif
  inverseViewMatrix_(identity4f()),
  viewMatrix_ (identity4f()),
  sensitivity_(0.000125f),
  walkSpeed_(0.5f)
{
  velocity_ = ref_ptr<UniformVec3>::manage(
      new UniformVec3("cameraVelocity", 1, Vec3f(0.0f, 0.0f, 0.0f)));
  joinStates(velocity_);

  cameraPositionUniform_ = ref_ptr<UniformVec3>::manage(
      new UniformVec3("cameraPosition", 1, Vec3f(0.0, 0.0, 0.0)));
  joinStates(cameraPositionUniform_);

  viewMatrixUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("viewMatrix", 1, identity4f()));
  joinStates(viewMatrixUniform_);

  inverseViewMatrixUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("inverseViewMatrix", 1, identity4f()));
  joinStates(inverseViewMatrixUniform_);
}

void Camera::set_isAudioListener(GLboolean isAudioListener)
{
  isAudioListener_ = isAudioListener;
  if(isAudioListener_) {
    AudioSystem &audio = AudioSystem::get();
    audio.set_listenerPosition( position_ );
    audio.set_listenerVelocity( velocity_->value() );
    audio.set_listenerOrientation( direction_, UP_VECTOR );
  }
}

const Mat4f& Camera::viewMatrix() const
{
  return viewMatrix_;
}
const Mat4f& Camera::inverseViewMatrix() const
{
  return inverseViewMatrix_;
}
void Camera::set_viewMatrix(const Mat4f &viewMatrix)
{
  viewMatrix_ = viewMatrix;
}

const Vec3f& Camera::velocity() const
{
  return velocity_->value();
}

const Vec3f& Camera::lastPosition() const
{
  return lastPosition_;
}

const Vec3f& Camera::position() const
{
  return position_;
}
void Camera::set_position(const Vec3f &position)
{
  position_ = position;
}

const Vec3f& Camera::direction() const
{
  return direction_;
}
void Camera::set_direction(const Vec3f &direction)
{
  direction_ = direction;
}

//void Camera::updateTransformationMatrices()
//{
  // TODO: signal handler?
  //viewMatrixUniform_->set_value(camera_->viewMatrix());
  //inverseViewMatrixUniform_->set_value(camera_->inverseViewMatrix());
  //cameraPositionUniform_->set_value( camera_->position() );
//}

void Camera::updateMatrix(float dt)
{
  viewMatrix_ = getLookAtMatrix(position_, direction_, UP_VECTOR);
  inverseViewMatrix_ = lookAtCameraInverse(viewMatrix_);

  // update the camera velocity
  if(dt > 1e-6) {
    velocity_->set_value( (lastPosition_ - position_) / dt );
    lastPosition_ = position_;
    if(isAudioListener_) {
      AudioSystem &audio = AudioSystem::get();
      audio.set_listenerVelocity( velocity_->value() );
    }
  }

  if(isAudioListener_) {
    AudioSystem &audio = AudioSystem::get();
    audio.set_listenerPosition( position_ );
    audio.set_listenerVelocity( velocity_->value() );
    audio.set_listenerOrientation( direction_, UP_VECTOR );
  }
}

float Camera::sensitivity() const
{
  return sensitivity_;
}
void Camera::set_sensitivity(float sensitivity)
{
  sensitivity_ = sensitivity;
}

float Camera::walkSpeed() const
{
  return walkSpeed_;
}
void Camera::set_walkSpeed(float walkSpeed)
{
  walkSpeed_ = walkSpeed;
}

void Camera::rotate(float xAmplitude, float yAmplitude, float deltaT)
{
  if(xAmplitude==0.0f && yAmplitude==0.0f) {
    return;
  }

  float rotX = -xAmplitude*sensitivity_*deltaT;
  float rotY = -yAmplitude*sensitivity_*deltaT;

  Vec3f d = direction_;
#ifdef UP_DIMENSION_Y
  rotateView( d, rotX, 0.0, 1.0, 0.0 );
#else
  rotateView( d, rotX, 0.0, 0.0, 1.0 );
#endif
  normalize( d );
  direction_ = d;

#ifdef UP_DIMENSION_Y
  Vec3f rotAxis = (Vec3f) { -d.z, 0.0, d.x };
#else
  Vec3f rotAxis = (Vec3f) { -d.y, d.x, 0.0 };
#endif
  normalize(rotAxis);
  rotateView( d, rotY, rotAxis.x, rotAxis.y, rotAxis.z );
  normalize( d );
#ifdef UP_DIMENSION_Y
  if(d.y>=-0.99 && d.y<=0.99) direction_ = d;
#else
  if(d.z>=-0.99 && d.z<=0.99) direction_ = d;
#endif
}

void Camera::translate(Direction d, float deltaT)
{
  switch(d) {
  case DIRECTION_FRONT:
    position_ += direction_ * walkSpeed_ * deltaT;
    break;
  case DIRECTION_BACK:
    position_ -= direction_ * walkSpeed_ * deltaT;
    break;
  case DIRECTION_LEFT:
    position_.x += direction_.z * walkSpeed_ * deltaT;
    position_.z -= direction_.x * walkSpeed_ * deltaT;
    break;
  case DIRECTION_RIGHT:
    position_.x -= direction_.z * walkSpeed_ * deltaT;
    position_.z += direction_.x * walkSpeed_ * deltaT;
    break;
  case DIRECTION_DOWN:
  case DIRECTION_UP:
    break;
  }
}
