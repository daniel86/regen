/*
 * camera.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include "camera.h"
#include <ogle/algebra/vector.h>
#include <ogle/algebra/matrix.h>
#include <ogle/av/audio.h>

Camera::Camera()
: State()
{
  projectionUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("projectionMatrix", 1, identity4f()));
  joinUniform(projectionUniform_);
}

UniformMat4* Camera::projectionUniform()
{
  return projectionUniform_.get();
}

///////////

OrthoCamera::OrthoCamera()
: Camera()
{
}

void OrthoCamera::updateProjection(
    GLfloat right, GLfloat top)
{
  projectionUniform_->set_value(getOrthogonalProjectionMatrix(
      0.0, right, 0.0, top, -1.0, 1.0));
}

///////////

PerspectiveCamera::PerspectiveCamera()
: Camera(),
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
  view_ (identity4f()),
  invView_(identity4f()),
  viewProjection_ (identity4f()),
  invViewProjection_(identity4f()),
  sensitivity_(0.000125f),
  walkSpeed_(0.5f)
{
  fovUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("fov", 1, 45.0));
  joinUniform(fovUniform_);

  nearUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("near", 1, 1.0));
  joinUniform(nearUniform_);

  farUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("far", 1, 200.0));
  joinUniform(farUniform_);

  velocity_ = ref_ptr<UniformVec3>::manage(
      new UniformVec3("cameraVelocity", 1, Vec3f(0.0f, 0.0f, 0.0f)));
  joinUniform(velocity_);

  cameraPositionUniform_ = ref_ptr<UniformVec3>::manage(
      new UniformVec3("cameraPosition", 1, Vec3f(0.0, 0.0, 0.0)));
  joinUniform(cameraPositionUniform_);

  viewUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("viewMatrix", 1, identity4f()));
  joinUniform(viewUniform_);

  viewProjectionUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("viewProjectionMatrix", 1, identity4f()));
  joinUniform(viewProjectionUniform_);

  invViewUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("inverseViewMatrix", 1, identity4f()));
  joinUniform(invViewUniform_);

  invProjectionUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("inverseProjectionMatrix", 1, identity4f()));
  joinUniform(invProjectionUniform_);

  invViewProjectionUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("inverseViewProjectionMatrix", 1, identity4f()));
  joinUniform(invViewProjectionUniform_);
}

void PerspectiveCamera::set_isAudioListener(GLboolean isAudioListener)
{
  isAudioListener_ = isAudioListener;
  if(isAudioListener_) {
    AudioSystem &audio = AudioSystem::get();
    audio.set_listenerPosition( position_ );
    audio.set_listenerVelocity( velocity_->value() );
    audio.set_listenerOrientation( direction_, UP_VECTOR );
  }
}

UniformMat4* PerspectiveCamera::viewUniform()
{
  return viewUniform_.get();
}
UniformMat4* PerspectiveCamera::viewProjectionUniform()
{
  return viewProjectionUniform_.get();
}
UniformMat4* PerspectiveCamera::inverseProjectionUniform()
{
  return invProjectionUniform_.get();
}
UniformMat4* PerspectiveCamera::inverseViewUniform()
{
  return invViewUniform_.get();
}
UniformMat4* PerspectiveCamera::inverseViewProjectionUniform()
{
  return invViewProjectionUniform_.get();
}

const Mat4f& PerspectiveCamera::viewMatrix() const
{
  return view_;
}
const Mat4f& PerspectiveCamera::inverseViewMatrix() const
{
  return invView_;
}
void PerspectiveCamera::set_viewMatrix(const Mat4f &viewMatrix)
{
  view_ = viewMatrix;
}

const Vec3f& PerspectiveCamera::velocity() const
{
  return velocity_->value();
}

const Vec3f& PerspectiveCamera::position() const
{
  return position_;
}
void PerspectiveCamera::set_position(const Vec3f &position)
{
  position_ = position;
}

const Vec3f& PerspectiveCamera::direction() const
{
  return direction_;
}
void PerspectiveCamera::set_direction(const Vec3f &direction)
{
  direction_ = direction;
}

void PerspectiveCamera::update(GLfloat dt)
{
  viewUniform_->set_value(view_);
  invViewUniform_->set_value(invView_);
  viewProjectionUniform_->set_value(viewProjection_);
  invViewProjectionUniform_->set_value(invViewProjection_);
  cameraPositionUniform_->set_value(position_);
}

void PerspectiveCamera::updateProjection(
    GLfloat fov,
    GLfloat near,
    GLfloat far,
    GLfloat aspect)
{
  fovUniform_->set_value( fov );
  nearUniform_->set_value( near );
  farUniform_->set_value( far );
  aspect_ = aspect;

  projectionUniform_->set_value( projectionMatrix(
          fovUniform_->value(),
          aspect_,
          nearUniform_->value(),
          farUniform_->value())
  );
  invProjectionUniform_->set_value(
      projectionMatrixInverse(projectionUniform_->value()));
  viewProjection_ = view_ * projectionUniform_->value();
  invViewProjection_ = invProjectionUniform_->value() * invView_;
}

void PerspectiveCamera::updatePerspective(GLfloat dt)
{
  view_ = getLookAtMatrix(position_, direction_, UP_VECTOR);
  invView_ = lookAtCameraInverse(view_);
  viewProjection_ = view_ * projectionUniform_->value();
  invViewProjection_ = invProjectionUniform_->value() * invView_;

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

float PerspectiveCamera::sensitivity() const
{
  return sensitivity_;
}
void PerspectiveCamera::set_sensitivity(float sensitivity)
{
  sensitivity_ = sensitivity;
}

float PerspectiveCamera::walkSpeed() const
{
  return walkSpeed_;
}
void PerspectiveCamera::set_walkSpeed(float walkSpeed)
{
  walkSpeed_ = walkSpeed;
}

void PerspectiveCamera::rotate(float xAmplitude, float yAmplitude, float deltaT)
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

void PerspectiveCamera::translate(Direction d, float deltaT)
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
