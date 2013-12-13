/*
 * camera.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <GL/glew.h>

#include <regen/states/atomic-states.h>
#include <regen/math/vector.h>
#include <regen/math/matrix.h>
#include <regen/av/audio.h>

#include "camera.h"
using namespace regen;

Camera::Camera()
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
  view_->setUniformData(Mat4f::lookAtMatrix(
      position_->getVertex(0),
      direction_->getVertex(0),
      Vec3f::up()));
  setInput(view_);

  viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");
  viewInv_->setUniformData(view_->getVertex(0).lookAtInverse());
  setInput(viewInv_);

  proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
  proj_->setUniformData(Mat4f::projectionMatrix(
      frustum_->fov()->getVertex(0),
      frustum_->aspect()->getVertex(0),
      frustum_->near()->getVertex(0),
      frustum_->far()->getVertex(0))
  );
  setInput(proj_);

  projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");
  projInv_->setUniformData(proj_->getVertex(0).projectionInverse());
  setInput(projInv_);

  viewproj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
  viewproj_->setUniformData(view_->getVertex(0) * proj_->getVertex(0));
  setInput(viewproj_);

  viewprojInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");
  viewprojInv_->setUniformData(projInv_->getVertex(0) * viewInv_->getVertex(0));
  setInput(viewprojInv_);
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

/////////////////////////////
/////////////////////////////

ReflectionCamera::ReflectionCamera(
    const ref_ptr<Camera> &userCamera,
    const ref_ptr<Mesh> &mesh,
    GLuint vertexIndex)
: Camera(),
  userCamera_(userCamera),
  vertexIndex_(vertexIndex),
  projStamp_(userCamera->projection()->stamp()-1),
  camPosStamp_(userCamera->position()->stamp()-1),
  camDirStamp_(userCamera->direction()->stamp()-1),
  cameraChanged_(GL_TRUE),
  isFront_(GL_TRUE),
  hasMesh_(GL_TRUE)
{
  frustum_->setProjection(
      userCamera->frustum()->fov()->getVertex(0),
      userCamera->frustum()->aspect()->getVertex(0),
      userCamera->frustum()->near()->getVertex(0),
      userCamera->frustum()->far()->getVertex(0));

  clipPlane_ = ref_ptr<ShaderInput4f>::alloc("clipPlane");
  clipPlane_->setUniformData(Vec4f(0.0f));
  setInput(clipPlane_);

  pos_ = mesh->positions();
  nor_ = mesh->normals();
  isReflectorValid_ = (pos_.get()!=NULL) && (nor_.get()!=NULL);
  if(isReflectorValid_) {
    posStamp_ = pos_->stamp()-1;
    norStamp_ = nor_->stamp()-1;
  }

  transform_ = mesh->findShaderInput("modelMatrix");
  if(transform_.get()!=NULL) {
    transformStamp_ = transform_->stamp()-1;
  }
}

ReflectionCamera::ReflectionCamera(
    const ref_ptr<Camera> &userCamera,
    const Vec3f &reflectorNormal,
    const Vec3f &reflectorPoint)
: Camera(),
  userCamera_(userCamera),
  projStamp_(userCamera->projection()->stamp()-1),
  camPosStamp_(userCamera->position()->stamp()-1),
  camDirStamp_(userCamera->direction()->stamp()-1),
  cameraChanged_(GL_TRUE),
  isFront_(GL_TRUE),
  hasMesh_(GL_FALSE)
{
  frustum_->setProjection(
      userCamera->frustum()->fov()->getVertex(0),
      userCamera->frustum()->aspect()->getVertex(0),
      userCamera->frustum()->near()->getVertex(0),
      userCamera->frustum()->far()->getVertex(0));

  clipPlane_ = ref_ptr<ShaderInput4f>::alloc("clipPlane");
  clipPlane_->setUniformData(Vec4f(0.0f));
  setInput(clipPlane_);

  vertexIndex_ = 0;
  transformStamp_ = 0;
  posStamp_ = 0;
  norStamp_ = 0;
  posWorld_ = reflectorPoint;
  norWorld_ = reflectorNormal;
  isReflectorValid_ = GL_TRUE;

  clipPlane_->setVertex(0, Vec4f(
      norWorld_.x,norWorld_.y,norWorld_.z,
      norWorld_.dot(posWorld_)));
  reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_,norWorld_);
}

void ReflectionCamera::enable(RenderState *rs)
{
  if(!isReflectorValid_) {
    REGEN_WARN("Reflector has no position/normal attribute.");
  }
  else if(!isHidden()) {
    updateReflection();
    Camera::enable(rs);
  }
}

void ReflectionCamera::updateReflection()
{
  GLboolean reflectorChanged = GL_FALSE;
  if(hasMesh_) {
    if(transform_.get()!=NULL && transform_->stamp()!=transformStamp_) {
      reflectorChanged = GL_TRUE;
      transformStamp_ = transform_->stamp();
    }
    if(nor_->stamp()!=norStamp_) {
      reflectorChanged = GL_TRUE;
      norStamp_ = nor_->stamp();
    }
    if(pos_->stamp()!=posStamp_) {
      reflectorChanged = GL_TRUE;
      posStamp_ = pos_->stamp();
    }
    // Compute plane parameters...
    if(reflectorChanged) {
      if(!pos_->hasClientData()) pos_->readServerData();
      if(!nor_->hasClientData()) nor_->readServerData();
      posWorld_ = ((Vec3f*)pos_->clientData())[vertexIndex_];
      norWorld_ = ((Vec3f*)nor_->clientData())[vertexIndex_];

      if(transform_.get()!=NULL) {
        if(!transform_->hasClientData()) transform_->readServerData();
        Mat4f transform = (((Mat4f*)transform_->clientData())[0]).transpose();
        posWorld_ = transform.transformVector(posWorld_);
        norWorld_ = transform.rotateVector(norWorld_);
        norWorld_.normalize();
      }
    }
  }

  // Switch normal if viewer is behind reflector.
  // TODO: also allow to skip computation in this case.
  const Vec3f &camPos = userCamera_->position()->getVertex(0);
  GLboolean isFront = norWorld_.dot(camPos-posWorld_)>0.0;
  if(isFront != isFront_) {
    isFront_ = isFront;
    reflectorChanged = GL_TRUE;
  }
  // Compute reflection matrix...
  if(reflectorChanged) {
    if(isFront_) {
      clipPlane_->setVertex(0, Vec4f(
          norWorld_.x,norWorld_.y,norWorld_.z,
          norWorld_.dot(posWorld_)));
      reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_,norWorld_);
    }
    else {
      // flip reflector normal
      Vec3f n = -norWorld_;
      clipPlane_->setVertex(0, Vec4f(n.x,n.y,n.z, n.dot(posWorld_)));
      reflectionMatrix_ = Mat4f::reflectionMatrix(posWorld_,n);
    }
  }

  // Compute reflection camera direction
  if(reflectorChanged || userCamera_->direction()->stamp() != camDirStamp_) {
    camDirStamp_ = userCamera_->direction()->stamp();
    Vec3f dir = reflectionMatrix_.rotateVector(userCamera_->direction()->getVertex(0));
    dir.normalize();
    direction_->setVertex(0,dir);

    reflectorChanged = GL_TRUE;
  }
  // Compute reflection camera position
  if(reflectorChanged || userCamera_->position()->stamp() != camPosStamp_) {
    camPosStamp_ = userCamera_->position()->stamp();
    Vec3f reflected = reflectionMatrix_.transformVector(
        userCamera_->position()->getVertex(0));
    position_->setVertex(0, reflected);

    reflectorChanged = GL_TRUE;
  }

  // Compute view matrix
  if(reflectorChanged) {
    view_->setUniformData(Mat4f::lookAtMatrix(
        position_->getVertex(0),
        direction_->getVertex(0),
        Vec3f::up()));
    viewInv_->setUniformData(view_->getVertex(0).lookAtInverse());
    cameraChanged_ = GL_TRUE;
  }

  // Compute projection matrix
  if(userCamera_->projection()->stamp() != projStamp_) {
    projStamp_ = userCamera_->projection()->stamp();
    proj_->setUniformData(
        userCamera_->projection()->getVertex(0));
    projInv_->setUniformData(
        userCamera_->projectionInverse()->getVertex(0));
    cameraChanged_ = GL_TRUE;
  }

  // Compute view-projection matrix
  if(cameraChanged_) {
    viewproj_->setVertex(0,
        view_->getVertex(0) * proj_->getVertex(0));
    viewprojInv_->setVertex(0,
        projInv_->getVertex(0) * viewInv_->getVertex(0));
    cameraChanged_ = GL_FALSE;
  }
}
