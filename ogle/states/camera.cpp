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

Camera::Camera() : ShaderInputState()
{
  u_proj_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("projectionMatrix"));
  u_proj_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(u_proj_));

  u_viewproj_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("viewProjectionMatrix"));
  u_viewproj_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(u_viewproj_));
}

ShaderInputMat4* Camera::projectionUniform()
{
  return u_proj_.get();
}
ShaderInputMat4* Camera::viewProjectionUniform()
{
  return u_viewproj_.get();
}

///////////

OrthoCamera::OrthoCamera()
: Camera()
{
}

void OrthoCamera::updateProjection(GLfloat right, GLfloat top)
{
  u_proj_->setUniformData(
      Mat4f::orthogonalMatrix(0.0, right, 0.0, top, -1.0, 1.0));
  u_viewproj_->setUniformData(u_proj_->getVertex16f(0));
}

///////////

PerspectiveCamera::PerspectiveCamera()
: Camera(),
  position_(Vec3f( 0.0, 1.0, 4.0 )),
  lastPosition_(position_),
  direction_(Vec3f( 0, 0, -1 )),
  fov_(45.0),
  near_(1.0f),
  far_(200.0f),
  proj_(Mat4f::identity()),
  projInv_(Mat4f::identity()),
  view_(Mat4f::identity()),
  viewInv_(Mat4f::identity()),
  viewproj_(Mat4f::identity()),
  viewprojInv_(Mat4f::identity()),
  sensitivity_(0.000125f),
  walkSpeed_(0.5f),
  aspect_(8.0/6.0),
  isAudioListener_(GL_FALSE),
  projectionChanged_(GL_FALSE)
{
  u_fov_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fov"));
  u_fov_->setUniformData(fov_);
  setInput(ref_ptr<ShaderInput>::cast(u_fov_));

  u_near_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("near"));
  u_near_->setUniformData(near_);
  setInput(ref_ptr<ShaderInput>::cast(u_near_));

  u_far_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("far"));
  u_far_->setUniformData(far_);
  setInput(ref_ptr<ShaderInput>::cast(u_far_));

  u_vel_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("cameraVelocity"));
  u_vel_->setUniformData(Vec3f(0.0f));
  setInput(ref_ptr<ShaderInput>::cast(u_vel_));

  u_position_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("cameraPosition"));
  u_position_->setUniformData(position_);
  setInput(ref_ptr<ShaderInput>::cast(u_position_));

  u_view_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("viewMatrix"));
  u_view_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(u_view_));

  u_viewInv_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("inverseViewMatrix"));
  u_viewInv_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(u_viewInv_));

  u_projInv_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("inverseProjectionMatrix"));
  u_projInv_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(u_projInv_));

  u_viewprojInv_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("inverseViewProjectionMatrix"));
  u_viewprojInv_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(u_viewprojInv_));

  updateProjection(fov_, near_, far_, aspect_);
  updatePerspective(0.0f);
  update(0.0f);
}

void PerspectiveCamera::set_isAudioListener(GLboolean isAudioListener)
{
  isAudioListener_ = isAudioListener;
  if(isAudioListener_) {
    AudioSystem &audio = AudioSystem::get();
    audio.set_listenerPosition( position_ );
    audio.set_listenerVelocity( u_vel_->getVertex3f(0) );
    audio.set_listenerOrientation( direction_, UP_VECTOR );
  }
}

GLfloat PerspectiveCamera::fov() const
{
  return fov_;
}
GLfloat PerspectiveCamera::near() const
{
  return near_;
}
GLfloat PerspectiveCamera::far() const
{
  return far_;
}
GLdouble PerspectiveCamera::aspect() const
{
  return aspect_;
}

const Mat4f& PerspectiveCamera::viewMatrix() const
{
  return view_;
}
const Mat4f& PerspectiveCamera::projection() const
{
  return proj_;
}
const Mat4f& PerspectiveCamera::viewProjectionMatrix() const
{
  return viewproj_;
}
const Mat4f& PerspectiveCamera::inverseViewProjectionMatrix() const
{
  return viewprojInv_;
}
const Mat4f& PerspectiveCamera::inverseViewMatrix() const
{
  return viewInv_;
}
void PerspectiveCamera::set_viewMatrix(const Mat4f &viewMatrix)
{
  view_ = viewMatrix;
}

const Vec3f& PerspectiveCamera::velocity() const
{
  return u_vel_->getVertex3f(0);
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

const ref_ptr<ShaderInputMat4>& PerspectiveCamera::viewUniform() const
{
  return u_view_;
}
const ref_ptr<ShaderInputMat4>& PerspectiveCamera::viewProjectionUniform() const
{
  return u_viewproj_;
}
const ref_ptr<ShaderInputMat4>& PerspectiveCamera::inverseProjectionUniform() const
{
  return u_projInv_;
}
const ref_ptr<ShaderInputMat4>& PerspectiveCamera::inverseViewUniform() const
{
  return u_viewInv_;
}
const ref_ptr<ShaderInputMat4>& PerspectiveCamera::inverseViewProjectionUniform() const
{
  return u_viewprojInv_;
}

const ref_ptr<ShaderInput1f>& PerspectiveCamera::fovUniform() const
{
  return u_fov_;
}
const ref_ptr<ShaderInput1f>& PerspectiveCamera::nearUniform() const
{
  return u_near_;
}
const ref_ptr<ShaderInput1f>& PerspectiveCamera::farUniform() const
{
  return u_far_;
}
const ref_ptr<ShaderInput3f>& PerspectiveCamera::velocityUniform() const
{
  return u_vel_;
}
const ref_ptr<ShaderInput3f>& PerspectiveCamera::positionUniform() const
{
  return u_position_;
}

void PerspectiveCamera::update(GLdouble dt)
{
  if(projectionChanged_) {
    u_proj_->setVertex16f(0,proj_);
    u_projInv_->setVertex16f(0,projInv_);
    u_fov_->setVertex1f(0,fov_);
    u_near_->setVertex1f(0,near_);
    u_far_->setVertex1f(0,far_);

    projectionChanged_ = GL_FALSE;
  }

  u_view_->setVertex16f(0,view_);
  u_viewInv_->setVertex16f(0,viewInv_);
  u_viewproj_->setVertex16f(0,viewproj_);
  u_viewprojInv_->setVertex16f(0,viewprojInv_);

  u_position_->setVertex3f(0,position_);

  // update the camera velocity
  if(dt > 1e-6) {
    u_vel_->setVertex3f(0, (lastPosition_ - position_) / dt );
    lastPosition_ = position_;
    if(isAudioListener_) {
      AudioSystem &audio = AudioSystem::get();
      audio.set_listenerVelocity( u_vel_->getVertex3f(0) );
    }
  }

  if(isAudioListener_) {
    AudioSystem &audio = AudioSystem::get();
    audio.set_listenerPosition( position_ );
    audio.set_listenerVelocity( u_vel_->getVertex3f(0) );
    audio.set_listenerOrientation( direction_, UP_VECTOR );
  }
}

void PerspectiveCamera::updateProjection(GLfloat fov, GLfloat near, GLfloat far, GLfloat aspect)
{
  fov_ = fov;
  near_ = near;
  far_ = far;
  aspect_ = aspect;

  proj_ = Mat4f::projectionMatrix(fov_, aspect_, near_, far_);
  projInv_ = proj_.projectionInverse();

  viewproj_ = view_ * proj_;
  viewprojInv_ = projInv_ * viewInv_;

  projectionChanged_ = GL_TRUE;
}

void PerspectiveCamera::updatePerspective(GLdouble dt)
{
  view_ = Mat4f::lookAtMatrix(position_, direction_, UP_VECTOR);
  viewInv_ = view_.lookAtInverse();

  viewproj_ = view_ * proj_;
  viewprojInv_ = projInv_ * viewInv_;
}

GLfloat PerspectiveCamera::sensitivity() const
{
  return sensitivity_;
}
void PerspectiveCamera::set_sensitivity(GLfloat sensitivity)
{
  sensitivity_ = sensitivity;
}

GLfloat PerspectiveCamera::walkSpeed() const
{
  return walkSpeed_;
}
void PerspectiveCamera::set_walkSpeed(GLfloat walkSpeed)
{
  walkSpeed_ = walkSpeed;
}

void PerspectiveCamera::rotate(GLfloat xAmplitude, GLfloat yAmplitude, GLdouble deltaT)
{
  if(xAmplitude==0.0f && yAmplitude==0.0f) {
    return;
  }

  float rotX = -xAmplitude*sensitivity_*deltaT;
  float rotY = -yAmplitude*sensitivity_*deltaT;

  Vec3f d = direction_;
  d.rotate(rotX, 0.0, 1.0, 0.0);
  d.normalize();
  direction_ = d;

  Vec3f rotAxis = (Vec3f) { -d.z, 0.0, d.x };
  rotAxis.normalize();
  d.rotate(rotY, rotAxis.x, rotAxis.y, rotAxis.z);
  d.normalize();
  if(d.y>=-0.99 && d.y<=0.99) direction_ = d;
}

void PerspectiveCamera::translate(Direction d, GLdouble deltaT)
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
