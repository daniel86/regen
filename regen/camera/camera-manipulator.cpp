/*
 * camera-manipulator.cpp
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#include <regen/av/audio.h>

#include "camera-manipulator.h"
using namespace regen;

CameraUpdater::CameraUpdater(const ref_ptr<Camera> &cam)
: cam_(cam)
{
}

void CameraUpdater::computeMatrices(const Vec3f &pos, const Vec3f &dir)
{
  Mat4f &proj = *(Mat4f*)cam_->projection()->ownedClientData();
  Mat4f &projInv = *(Mat4f*)cam_->projectionInverse()->ownedClientData();
  view_ = Mat4f::lookAtMatrix(pos, dir, Vec3f::up());
  viewInv_ = view_.lookAtInverse();
  viewproj_ = view_ * proj;
  viewprojInv_ = projInv * viewInv_;
}

void CameraUpdater::updateCamera(const Vec3f &pos, const Vec3f &dir, GLdouble dt)
{
  cam_->position()->setVertex(0, pos);
  cam_->direction()->setVertex(0, dir);

  velocity_ = (lastPosition_ - pos) / dt;
  cam_->velocity()->setVertex(0,velocity_);
  lastPosition_ = pos;

  cam_->view()->setVertex(0,view_);
  cam_->viewInverse()->setVertex(0,viewInv_);
  cam_->viewProjection()->setVertex(0,viewproj_);
  cam_->viewProjectionInverse()->setVertex(0,viewprojInv_);

  if(cam_->isAudioListener()) {
    AudioListener::set3f(AL_POSITION, pos);
    AudioListener::set3f(AL_VELOCITY, velocity_);
    AudioListener::set6f(AL_ORIENTATION, Vec6f(dir, Vec3f::up()));
  }
}

////////////////
////////////////
////////////////

KeyFrameCameraTransform::KeyFrameCameraTransform(const ref_ptr<Camera> &cam)
: Animation(GL_TRUE,GL_TRUE),
  CameraUpdater(cam)
{
  camPos_ = cam->position()->getVertex(0);
  camDir_ = cam->direction()->getVertex(0);
  it_ = frames_.end();
  lastFrame_.pos = camPos_;
  lastFrame_.dir = camDir_;
  lastFrame_.dt  = 0.0;
  dt_ = 0.0;
}

void KeyFrameCameraTransform::push_back(const Vec3f &pos, const Vec3f &dir, GLdouble dt)
{
  CameraKeyFrame f;
  f.pos = pos;
  f.dir = dir;
  f.dt = dt;
  frames_.push_back(f);
  if(frames_.size()==1) {
    it_ = frames_.begin();
  }
}

Vec3f KeyFrameCameraTransform::interpolate(const Vec3f &v0, const Vec3f &v1, GLdouble t)
{
  return math::mix(v0,v1,t);
}

void KeyFrameCameraTransform::animate(GLdouble dt)
{
  if(it_ == frames_.end()) {
    it_ = frames_.begin();
    dt_ = 0.0;
  }
  CameraKeyFrame &currentFrame = *it_;

  dt_ += dt/1000.0;
  if(dt_ >= currentFrame.dt) {
    ++it_;
    lastFrame_ = currentFrame;
    GLdouble dt__ = dt_-currentFrame.dt;
    dt_ = 0.0;
    animate(dt__);
  }
  else {
    Vec3f &pos0 = lastFrame_.pos;
    Vec3f &pos1 = currentFrame.pos;
    Vec3f dir0 = lastFrame_.dir;
    Vec3f dir1 = currentFrame.dir;
    GLdouble t = currentFrame.dt>0.0 ? dt_/currentFrame.dt : 1.0;
    dir0.normalize();
    dir1.normalize();

    lock(); {
      camPos_ = interpolate(pos0, pos1, t);
      camDir_ = interpolate(dir0, dir1, t);
      camDir_.normalize();
      computeMatrices(camPos_, camDir_);
    } unlock();
  }
}

void KeyFrameCameraTransform::glAnimate(RenderState *rs, GLdouble dt)
{
  lock(); {
    updateCamera(camPos_, camDir_, dt);
  } unlock();
}

////////////////
////////////////
////////////////

FirstPersonTransform::FirstPersonTransform(const ref_ptr<ShaderInputMat4> &mat)
: Animation(GL_TRUE,GL_TRUE),
  mat_(mat)
{
  horizontalOrientation_ = 0.0;
  moveAmount_ = 1.0;
  moveForward_ = GL_FALSE;
  moveBackward_ = GL_FALSE;
  moveLeft_ = GL_FALSE;
  moveRight_ = GL_FALSE;
  matVal_ = Mat4f::identity();
}

void FirstPersonTransform::set_moveAmount(GLfloat moveAmount)
{ moveAmount_ = moveAmount; }

void FirstPersonTransform::moveForward(GLboolean v)
{ moveForward_ = v; }
void FirstPersonTransform::moveBackward(GLboolean v)
{ moveBackward_ = v; }
void FirstPersonTransform::moveLeft(GLboolean v)
{ moveLeft_ = v; }
void FirstPersonTransform::moveRight(GLboolean v)
{ moveRight_ = v; }

void FirstPersonTransform::stepForward(const GLfloat &v)
{ step(dirXZ_*v); }
void FirstPersonTransform::stepBackward(const GLfloat &v)
{ step(dirXZ_*(-v)); }
void FirstPersonTransform::stepLeft(const GLfloat &v)
{ step(dirSidestep_*(-v)); }
void FirstPersonTransform::stepRight(const GLfloat &v)
{ step(dirSidestep_*v); }
void FirstPersonTransform::step(const Vec3f &v)
{ pos_ += v; }

void FirstPersonTransform::lookLeft(GLdouble amount)
{ horizontalOrientation_ = fmod(horizontalOrientation_+amount, 2.0*M_PI); }
void FirstPersonTransform::lookRight(GLdouble amount)
{ horizontalOrientation_ = fmod(horizontalOrientation_-amount, 2.0*M_PI); }

void FirstPersonTransform::animate(GLdouble dt)
{
  rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_);
  Vec3f d = rot_.rotate(Vec3f::front());

  dirXZ_ = Vec3f(d.x, 0.0f, d.z);
  dirXZ_.normalize();
  dirSidestep_ = dirXZ_.cross(Vec3f::up());

  lock(); {
    if(moveForward_)       stepForward(moveAmount_*dt);
    else if(moveBackward_) stepBackward(moveAmount_*dt);
    if(moveLeft_)          stepLeft(moveAmount_*dt);
    else if(moveRight_)    stepRight(moveAmount_*dt);
  } unlock();
}

void FirstPersonTransform::glAnimate(RenderState *rs, GLdouble dt)
{
  lock(); {
    // Simple rotation matrix around up vector (0,1,0)
    GLdouble cy = cos(horizontalOrientation_), sy = sin(horizontalOrientation_);
    matVal_.x[0 ] =  cy;
    matVal_.x[2 ] =  sy;
    matVal_.x[8 ] = -sy;
    matVal_.x[10] =  cy;
    // Translate to mesh position
    const Mat4f &m = mat_->getVertex(0);
    matVal_.x[12] = m.x[12] - pos_.x;
    matVal_.x[13] = m.x[13] - pos_.y;
    matVal_.x[14] = m.x[14] - pos_.z;
    pos_ = Vec3f(0.0f);

    mat_->setVertex(0, matVal_);
  } unlock();
}

////////////////
////////////////
////////////////

FirstPersonCameraTransform::FirstPersonCameraTransform(
    const ref_ptr<Camera> &cam,
    const ref_ptr<Mesh> &mesh,
    const ref_ptr<ModelTransformation> &transform,
    const Vec3f &meshEyeOffset,
    GLdouble meshHorizontalOrientation)
: FirstPersonTransform(transform->get()),
  CameraUpdater(cam),
  mesh_(mesh),
  mat_(transform->get()),
  meshEyeOffset_(meshEyeOffset)
{
  verticalOrientation_ = 0.0;
  meshHorizontalOrientation_ = meshHorizontalOrientation;
  pos_ = Vec3f(0.0f);
}

FirstPersonCameraTransform::FirstPersonCameraTransform(
    const ref_ptr<Camera> &cam)
: FirstPersonTransform(cam->view()),
  CameraUpdater(cam)
{
  verticalOrientation_ = 0.0;
  meshHorizontalOrientation_ = 0.0;
  pos_ = cam->position()->getVertex(0);
}

void FirstPersonCameraTransform::updateCameraPosition()
{
  if(mat_.get()) {
    camPos_ = meshEyeOffset_;
    if(mesh_.get()) {
      camPos_ += mesh_->centerPosition();
    }
    camPos_ = (mat_->getVertex(0) ^ Vec4f(camPos_,1.0)).xyz_();
  }
  else {
    camPos_ = pos_;
  }
}

void FirstPersonCameraTransform::updateCameraOrientation()
{
  rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_+meshHorizontalOrientation_);
  camDir_ = rot_.rotate(Vec3f::front());
  rot_.setAxisAngle(dirSidestep_, verticalOrientation_);
  camDir_ = rot_.rotate(camDir_);
}

#define __ORIENT_THRESHOLD__ 0.1

void FirstPersonCameraTransform::lookUp(GLdouble amount)
{ verticalOrientation_ = math::clamp(verticalOrientation_+amount,
    -0.5*M_PI + __ORIENT_THRESHOLD__, 0.5*M_PI - __ORIENT_THRESHOLD__); }
void FirstPersonCameraTransform::lookDown(GLdouble amount)
{ verticalOrientation_ = math::clamp(verticalOrientation_-amount,
    -0.5*M_PI + __ORIENT_THRESHOLD__, 0.5*M_PI - __ORIENT_THRESHOLD__); }

void FirstPersonCameraTransform::zoomIn(GLdouble amount)
{  }
void FirstPersonCameraTransform::zoomOut(GLdouble amount)
{  }

void FirstPersonCameraTransform::animate(GLdouble dt)
{
  FirstPersonTransform::animate(dt);
  lock(); {
    updateCameraPosition();
    updateCameraOrientation();
    computeMatrices(camPos_, camDir_);
  } unlock();
}

void FirstPersonCameraTransform::glAnimate(RenderState *rs, GLdouble dt)
{
  if(mat_.get()) {
    FirstPersonTransform::glAnimate(rs,dt);
  }
  lock(); {
    updateCamera(camPos_, camDir_, dt);
  } unlock();
}

////////////////
////////////////
////////////////

ThirdPersonCameraTransform::ThirdPersonCameraTransform(
    const ref_ptr<Camera> &cam,
    const ref_ptr<Mesh> &mesh,
    const ref_ptr<ModelTransformation> &transform,
    const Vec3f &eyeOffset,
    GLfloat eyeOrientation)
: FirstPersonCameraTransform(cam,mesh,transform,eyeOffset,eyeOrientation),
  meshDistance_(10.0f)
{
}

void ThirdPersonCameraTransform::updateCameraPosition()
{
  FirstPersonCameraTransform::updateCameraPosition();
  meshPos_ = camPos_;

  rot_.setAxisAngle(Vec3f::up(), horizontalOrientation_+meshHorizontalOrientation_);
  Vec3f dir = rot_.rotate(Vec3f::front());

  rot_.setAxisAngle(dir.cross(Vec3f::up()), verticalOrientation_);
  camPos_ -= rot_.rotate(dir*meshDistance_);
}

void ThirdPersonCameraTransform::updateCameraOrientation()
{
  camDir_ = meshPos_ - camPos_;
  camDir_.normalize();
}

void ThirdPersonCameraTransform::zoomIn(GLdouble amount)
{ meshDistance_ = math::clamp(meshDistance_-amount, 0.0, 100.0); }
void ThirdPersonCameraTransform::zoomOut(GLdouble amount)
{ meshDistance_ = math::clamp(meshDistance_+amount, 0.0, 100.0); }
