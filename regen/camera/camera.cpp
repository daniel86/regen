/*
 * camera.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include "camera.h"
using namespace regen;

Camera::Camera(GLboolean initializeMatrices)
: HasInputState(VBO::USAGE_DYNAMIC),
  isAudioListener_(GL_FALSE)
{
  fov_ = ref_ptr<ShaderInput1f>::alloc("fov");
  fov_->setUniformDataUntyped(NULL);
  setInput(fov_);

  aspect_ = ref_ptr<ShaderInput1f>::alloc("aspect");
  aspect_->setUniformDataUntyped(NULL);
  setInput(aspect_);

  near_ = ref_ptr<ShaderInput1f>::alloc("near");
  near_->setUniformDataUntyped(NULL);
  setInput(near_);

  far_ = ref_ptr<ShaderInput1f>::alloc("far");
  far_->setUniformDataUntyped(NULL);
  setInput(far_);

  position_ = ref_ptr<ShaderInput3f>::alloc("cameraPosition");
  position_->setUniformData(Vec3f( 0.0, 1.0, 4.0 ));
  setInput(position_);

  direction_ = ref_ptr<ShaderInput3f>::alloc("cameraDirection");
  direction_->setUniformData(Vec3f( 0, 0, -1 ));
  setInput(direction_);

  vel_ = ref_ptr<ShaderInput3f>::alloc("cameraVelocity");
  vel_->setUniformData(Vec3f(0.0f));
  setInput(vel_);

  view_ = ref_ptr<ShaderInputMat4>::alloc("viewMatrix");
  setInput(view_);
  viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");
  setInput(viewInv_);

  proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
  setInput(proj_);
  projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");
  setInput(projInv_);

  viewproj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
  setInput(viewproj_);
  viewprojInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");
  setInput(viewprojInv_);

  updateFrustum(8.0/6.0, 45.0, 1.0, 200.0, GL_FALSE);
  if(initializeMatrices) {
    view_->setUniformDataUntyped(NULL);
    viewInv_->setUniformDataUntyped(NULL);
    proj_->setUniformDataUntyped(NULL);
    projInv_->setUniformDataUntyped(NULL);
    viewproj_->setUniformDataUntyped(NULL);
    viewprojInv_->setUniformDataUntyped(NULL);

    updateProjection();
    updateLookAt();
    updateViewProjection();
  }
}

void Camera::updateLookAt()
{
  view_->setVertex(0, Mat4f::lookAtMatrix(
      position_->getVertex(0),
      direction_->getVertex(0),
      Vec3f::up()));
  viewInv_->setVertex(0, view_->getVertex(0).lookAtInverse());
}

void Camera::updateProjection()
{
  proj_->setVertex(0, Mat4f::projectionMatrix(
      fov_->getVertex(0),
      aspect_->getVertex(0),
      near_->getVertex(0),
      far_->getVertex(0))
  );
  projInv_->setVertex(0, proj_->getVertex(0).projectionInverse());
}

void Camera::updateViewProjection(GLuint i, GLuint j)
{
  viewproj_->setVertex((i>j?i:j),
      view_->getVertex(j) * proj_->getVertex(i));
  viewprojInv_->setVertex((i>j?i:j),
      projInv_->getVertex(i) * viewInv_->getVertex(j));
}

void Camera::updateFrustum(
    GLfloat aspect,
    GLfloat fov,
    GLfloat near,
    GLfloat far,
    GLboolean updateMatrices)
{
  near_->setVertex(0,near);
  far_->setVertex(0,far);
  fov_->setVertex(0,fov);
  aspect_->setVertex(0,aspect);
  frustum_.set(aspect,fov,near,far);

  if(updateMatrices) {
    updateProjection();
    updateViewProjection();
  }
}

const ref_ptr<ShaderInput1f>& Camera::fov() const
{ return fov_; }
const ref_ptr<ShaderInput1f>& Camera::near() const
{ return near_; }
const ref_ptr<ShaderInput1f>& Camera::far() const
{ return far_; }
const ref_ptr<ShaderInput1f>& Camera::aspect() const
{ return aspect_; }

const Frustum& Camera::frustum() const
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

void Camera::set_isAudioListener(GLboolean isAudioListener)
{
  isAudioListener_ = isAudioListener;
  if(isAudioListener_) {
    AudioListener::set3f(AL_POSITION, position_->getVertex(0));
    AudioListener::set3f(AL_VELOCITY, vel_->getVertex(0));
    AudioListener::set6f(AL_ORIENTATION, Vec6f(direction_->getVertex(0), Vec3f::up()));
  }
}
GLboolean Camera::isAudioListener() const
{ return isAudioListener_; }
