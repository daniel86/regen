/*
 * camera.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <regen/algebra/vector.h>
#include <regen/algebra/matrix.h>
#include <regen/av/audio.h>

#include "camera.h"
using namespace regen;

Camera::Camera()
: ShaderInputState(),
  sensitivity_(0.000125f),
  walkSpeed_(0.5f),
  isAudioListener_(GL_FALSE)
{
  position_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("cameraPosition"));
  position_->setUniformData(Vec3f( 0.0, 1.0, 4.0 ));
  setInput(ref_ptr<ShaderInput>::cast(position_));

  direction_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("cameraDirection"));
  direction_->setUniformData(Vec3f( 0, 0, -1 ));
  setInput(ref_ptr<ShaderInput>::cast(direction_));

  vel_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("cameraVelocity"));
  vel_->setUniformData(Vec3f(0.0f));
  setInput(ref_ptr<ShaderInput>::cast(vel_));

  frustum_ = ref_ptr<Frustum>::manage(new Frustum);
  frustum_->setProjection(45.0, 8.0/6.0, 1.0, 200.0);
  setInput(ref_ptr<ShaderInput>::cast(frustum_->fov()));
  setInput(ref_ptr<ShaderInput>::cast(frustum_->near()));
  setInput(ref_ptr<ShaderInput>::cast(frustum_->far()));
  setInput(ref_ptr<ShaderInput>::cast(frustum_->aspect()));

  view_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("viewMatrix"));
  view_->setUniformData(Mat4f::lookAtMatrix(
      position_->getVertex3f(0),
      direction_->getVertex3f(0),
      Vec3f::up()));
  setInput(ref_ptr<ShaderInput>::cast(view_));

  viewInv_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("inverseViewMatrix"));
  viewInv_->setUniformData(view_->getVertex16f(0).lookAtInverse());
  setInput(ref_ptr<ShaderInput>::cast(viewInv_));

  proj_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("projectionMatrix"));
  proj_->setUniformData(Mat4f::projectionMatrix(
      frustum_->fov()->getVertex1f(0),
      frustum_->aspect()->getVertex1f(0),
      frustum_->near()->getVertex1f(0),
      frustum_->far()->getVertex1f(0))
  );
  setInput(ref_ptr<ShaderInput>::cast(proj_));

  projInv_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("inverseProjectionMatrix"));
  projInv_->setUniformData(proj_->getVertex16f(0).projectionInverse());
  setInput(ref_ptr<ShaderInput>::cast(projInv_));

  viewproj_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("viewProjectionMatrix"));
  viewproj_->setUniformData(view_->getVertex16f(0) * proj_->getVertex16f(0));
  setInput(ref_ptr<ShaderInput>::cast(viewproj_));

  viewprojInv_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("inverseViewProjectionMatrix"));
  viewprojInv_->setUniformData(projInv_->getVertex16f(0) * viewInv_->getVertex16f(0));
  setInput(ref_ptr<ShaderInput>::cast(viewprojInv_));
}

const ref_ptr<Frustum>& Camera::frustum() const
{ return frustum_; }

const ref_ptr<ShaderInput3f>& Camera::position() const
{ return position_; }
const ref_ptr<ShaderInput3f>& Camera::velocity() const
{ return vel_; }
const ref_ptr<ShaderInput3f>& Camera::direction() const
{ return direction_; }

const ref_ptr<ShaderInputMat4>& Camera::view() const
{ return view_; }
const ref_ptr<ShaderInputMat4>& Camera::viewInverse() const
{ return viewInv_; }

const ref_ptr<ShaderInputMat4>& Camera::projection() const
{ return proj_; }
const ref_ptr<ShaderInputMat4>& Camera::projectionInverse() const
{ return projInv_; }

const ref_ptr<ShaderInputMat4>& Camera::viewProjection() const
{ return viewproj_; }
const ref_ptr<ShaderInputMat4>& Camera::viewProjectionInverse() const
{ return viewprojInv_; }

GLfloat Camera::sensitivity() const
{ return sensitivity_; }
void Camera::set_sensitivity(GLfloat sensitivity)
{ sensitivity_ = sensitivity; }

GLfloat Camera::walkSpeed() const
{ return walkSpeed_; }
void Camera::set_walkSpeed(GLfloat walkSpeed)
{ walkSpeed_ = walkSpeed; }

void Camera::set_isAudioListener(GLboolean isAudioListener)
{
  isAudioListener_ = isAudioListener;
  if(isAudioListener_) {
    AudioSystem &audio = AudioSystem::get();
    audio.set_listenerPosition( position_->getVertex3f(0) );
    audio.set_listenerVelocity( vel_->getVertex3f(0) );
    audio.set_listenerOrientation( direction_->getVertex3f(0), Vec3f::up() );
  }
}
GLboolean Camera::isAudioListener() const
{ return isAudioListener_; }
