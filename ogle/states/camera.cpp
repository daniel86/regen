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
#include <ogle/states/render-state.h>

Camera::Camera()
: ShaderInputState()
{
  projectionUniform_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("projectionMatrix"));
  projectionUniform_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(projectionUniform_));

  viewProjectionUniform_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("viewProjectionMatrix"));
  viewProjectionUniform_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(viewProjectionUniform_));
}

ShaderInputMat4* Camera::projectionUniform()
{
  return projectionUniform_.get();
}
ShaderInputMat4* Camera::viewProjectionUniform()
{
  return viewProjectionUniform_.get();
}

///////////

OrthoCamera::OrthoCamera()
: Camera()
{
}

void OrthoCamera::updateProjection(GLfloat right, GLfloat top)
{
  projectionUniform_->setUniformData(
      Mat4f::orthogonalMatrix(0.0, right, 0.0, top, -1.0, 1.0));
  viewProjectionUniform_->setUniformData(projectionUniform_->getVertex16f(0));
}

///////////

PerspectiveCamera::PerspectiveCamera()
: Camera(),
  position_(Vec3f( 0.0, 1.0, 4.0 )),
  direction_(Vec3f( 0, 0, -1 )),
  view_ (Mat4f::identity()),
  viewProjection_ (Mat4f::identity()),
  invView_(Mat4f::identity()),
  invViewProjection_(Mat4f::identity()),
  lastPosition_(position_),
  sensitivity_(0.000125f),
  walkSpeed_(0.5f),
  aspect_(8.0/6.0),
  isAudioListener_(GL_FALSE)
{
  fovUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("fov"));
  fovUniform_->setUniformData(45.0);
  setInput(ref_ptr<ShaderInput>::cast(fovUniform_));

  nearUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("near"));
  nearUniform_->setUniformData(1.0);
  setInput(ref_ptr<ShaderInput>::cast(nearUniform_));

  farUniform_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("far"));
  farUniform_->setUniformData(200.0);
  setInput(ref_ptr<ShaderInput>::cast(farUniform_));

  velocity_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f("cameraVelocity"));
  velocity_->setUniformData(Vec3f(0.0f));
  setInput(ref_ptr<ShaderInput>::cast(velocity_));

  cameraPositionUniform_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f("cameraPosition"));
  cameraPositionUniform_->setUniformData(position_);
  setInput(ref_ptr<ShaderInput>::cast(cameraPositionUniform_));

  viewUniform_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("viewMatrix"));
  viewUniform_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(viewUniform_));

  invViewUniform_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("inverseViewMatrix"));
  invViewUniform_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(invViewUniform_));

  invProjectionUniform_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("inverseProjectionMatrix"));
  invProjectionUniform_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(invProjectionUniform_));

  invViewProjectionUniform_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("inverseViewProjectionMatrix"));
  invViewProjectionUniform_->setUniformData(Mat4f::identity());
  setInput(ref_ptr<ShaderInput>::cast(invViewProjectionUniform_));

  updateProjection(
      fovUniform_->getVertex1f(0),
      nearUniform_->getVertex1f(0),
      farUniform_->getVertex1f(0),
      aspect_);
  updatePerspective(0.0f);
  update(0.0f);
}

void PerspectiveCamera::set_isAudioListener(GLboolean isAudioListener)
{
  isAudioListener_ = isAudioListener;
  if(isAudioListener_) {
    AudioSystem &audio = AudioSystem::get();
    audio.set_listenerPosition( position_ );
    audio.set_listenerVelocity( velocity_->getVertex3f(0) );
    audio.set_listenerOrientation( direction_, UP_VECTOR );
  }
}

const ref_ptr<ShaderInputMat4>& PerspectiveCamera::viewUniform() const
{
  return viewUniform_;
}
const ref_ptr<ShaderInputMat4>& PerspectiveCamera::viewProjectionUniform() const
{
  return viewProjectionUniform_;
}

const ref_ptr<ShaderInputMat4>& PerspectiveCamera::inverseProjectionUniform() const
{
  return invProjectionUniform_;
}
const ref_ptr<ShaderInputMat4>& PerspectiveCamera::inverseViewUniform() const
{
  return invViewUniform_;
}
const ref_ptr<ShaderInputMat4>& PerspectiveCamera::inverseViewProjectionUniform() const
{
  return invViewProjectionUniform_;
}

GLfloat& PerspectiveCamera::fov() const
{
  return fovUniform_->getVertex1f(0);
}
GLfloat& PerspectiveCamera::near() const
{
  return nearUniform_->getVertex1f(0);
}
GLfloat& PerspectiveCamera::far() const
{
  return farUniform_->getVertex1f(0);
}

GLdouble PerspectiveCamera::aspect() const
{
  return aspect_;
}

const ref_ptr<ShaderInput1f>& PerspectiveCamera::fovUniform() const
{
  return fovUniform_;
}
const ref_ptr<ShaderInput1f>& PerspectiveCamera::nearUniform() const
{
  return nearUniform_;
}
const ref_ptr<ShaderInput1f>& PerspectiveCamera::farUniform() const
{
  return farUniform_;
}
const ref_ptr<ShaderInput3f>& PerspectiveCamera::velocityUniform() const
{
  return velocity_;
}
const ref_ptr<ShaderInput3f>& PerspectiveCamera::positionUniform() const
{
  return cameraPositionUniform_;
}

const Mat4f& PerspectiveCamera::viewMatrix() const
{
  return view_;
}
const Mat4f& PerspectiveCamera::viewProjectionMatrix() const
{
  return viewProjection_;
}
void PerspectiveCamera::set_viewMatrix(const Mat4f &viewMatrix)
{
  view_ = viewMatrix;
}

const Mat4f& PerspectiveCamera::inverseViewProjectionMatrix() const
{
  return invViewProjection_;
}
const Mat4f& PerspectiveCamera::inverseViewMatrix() const
{
  return invView_;
}

const Vec3f& PerspectiveCamera::velocity() const
{
  return velocity_->getVertex3f(0);
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

void PerspectiveCamera::update(GLdouble dt)
{
  viewUniform_->setUniformData(view_);
  viewProjectionUniform_->setUniformData(viewProjection_);
  cameraPositionUniform_->setUniformData(position_);

  invViewUniform_->setUniformData(invView_);
  invViewProjectionUniform_->setUniformData(invViewProjection_);
}

void PerspectiveCamera::updateProjection(GLfloat fov, GLfloat near, GLfloat far, GLfloat aspect)
{
  fovUniform_->setUniformData( fov );
  nearUniform_->setUniformData( near );
  farUniform_->setUniformData( far );
  aspect_ = aspect;

  projectionUniform_->setUniformData( Mat4f::projectionMatrix(
          fovUniform_->getVertex1f(0),
          aspect_,
          nearUniform_->getVertex1f(0),
          farUniform_->getVertex1f(0))
  );
  viewProjection_ = view_ * projectionUniform_->getVertex16f(0);

  invProjectionUniform_->setUniformData(
      projectionUniform_->getVertex16f(0).projectionInverse());
  invViewProjection_ = invProjectionUniform_->getVertex16f(0) * invView_;
}

void PerspectiveCamera::updatePerspective(GLdouble dt)
{
  view_ = Mat4f::lookAtMatrix(position_, direction_, UP_VECTOR);
  viewProjection_ = view_ * projectionUniform_->getVertex16f(0);

  invView_ = view_.lookAtInverse();
  invViewProjection_ = invProjectionUniform_->getVertex16f(0) * invView_;

  // update the camera velocity
  if(dt > 1e-6) {
    velocity_->setUniformData( (lastPosition_ - position_) / dt );
    lastPosition_ = position_;
    if(isAudioListener_) {
      AudioSystem &audio = AudioSystem::get();
      audio.set_listenerVelocity( velocity_->getVertex3f(0) );
    }
  }

  if(isAudioListener_) {
    AudioSystem &audio = AudioSystem::get();
    audio.set_listenerPosition( position_ );
    audio.set_listenerVelocity( velocity_->getVertex3f(0) );
    audio.set_listenerOrientation( direction_, UP_VECTOR );
  }
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

void PerspectiveCamera::enable(RenderState *rs)
{
  lastViewMatrix_ = rs->viewMatrix();
  lastProjectionMatrix_ = rs->projectionMatrix();
  rs->set_viewMatrix((Mat4f*)viewUniform_->dataPtr());
  rs->set_projectionMatrix((Mat4f*)projectionUniform_->dataPtr());
  State::enable(rs);
}
void PerspectiveCamera::disable(RenderState *rs)
{
  rs->set_viewMatrix(lastViewMatrix_);
  rs->set_projectionMatrix(lastProjectionMatrix_);
  State::enable(rs);
}
