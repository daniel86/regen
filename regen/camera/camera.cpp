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
  position_ = ref_ptr<ShaderInput3f>::alloc("cameraPosition");
  position_->setUniformData(Vec3f( 0.0, 1.0, 4.0 ));
  setInput(position_);

  direction_ = ref_ptr<ShaderInput3f>::alloc("cameraDirection");
  direction_->setUniformData(Vec3f( 0, 0, -1 ));
  setInput(direction_);

  vel_ = ref_ptr<ShaderInput3f>::alloc("cameraVelocity");
  vel_->setUniformData(Vec3f(0.0f));
  setInput(vel_);

  const GLfloat fov    = 45.0;
  const GLfloat aspect = 8.0/6.0;
  const GLfloat near   = 1.0;
  const GLfloat far    = 200.0;
  frustum_ = ref_ptr<Frustum>::alloc();
  frustum_->setProjection(fov, aspect, near, far);
  setInput(frustum_->fov());
  setInput(frustum_->near());
  setInput(frustum_->far());
  setInput(frustum_->aspect());

  view_ = ref_ptr<ShaderInputMat4>::alloc("viewMatrix");
  viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");
  proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
  projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");
  viewproj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
  viewprojInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");
  setInput(view_);
  setInput(viewInv_);
  setInput(proj_);
  setInput(projInv_);
  setInput(viewproj_);
  setInput(viewprojInv_);

  if(initializeMatrices) {
    view_->setUniformData(Mat4f::lookAtMatrix(
        position_->getVertex(0),
        direction_->getVertex(0),
        Vec3f::up()));
    proj_->setUniformData(Mat4f::projectionMatrix(
        frustum_->fov()->getVertex(0),
        frustum_->aspect()->getVertex(0),
        frustum_->near()->getVertex(0),
        frustum_->far()->getVertex(0))
    );
    viewInv_->setUniformData(view_->getVertex(0).lookAtInverse());
    projInv_->setUniformData(proj_->getVertex(0).projectionInverse());
    viewproj_->setUniformData(view_->getVertex(0) * proj_->getVertex(0));
    viewprojInv_->setUniformData(projInv_->getVertex(0) * viewInv_->getVertex(0));
  }
}

void Camera::updateFrustum(
    const Vec2i viewport,
    GLfloat fov,
    GLfloat near,
    GLfloat far)
{
  frustum_->setProjection(
      fov,
      ((GLfloat)viewport.x)/((GLfloat)viewport.y),
      near,
      far);
  proj_->setVertex(0, Mat4f::projectionMatrix(
      fov, frustum_->aspect()->getVertex(0), near, far));
  projInv_->setVertex(0,
      proj_->getVertex(0).projectionInverse());
  viewproj_->setVertex(0,
      view_->getVertex(0) * proj_->getVertex(0));
  viewprojInv_->setVertex(0,
      projInv_->getVertex(0) * viewInv_->getVertex(0));
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
