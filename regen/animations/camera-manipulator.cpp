/*
 * camera-manipulator.cpp
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#include <regen/av/audio.h>

#include "camera-manipulator.h"
using namespace regen;

CameraManipulator::CameraManipulator(const ref_ptr<Camera> &cam)
: Animation(GL_TRUE,GL_TRUE),
  cam_(cam)
{
  velocity_ = cam_->velocity()->getVertex3f(0);
  position_ = cam_->position()->getVertex3f(0);
  direction_ = cam_->direction()->getVertex3f(0);
}

void CameraManipulator::glAnimate(RenderState *rs, GLdouble dt)
{
  lock(); {
    cam_->position()->setVertex3f(0,position_);
    cam_->direction()->setVertex3f(0,direction_);
    cam_->velocity()->setVertex3f(0,velocity_);

    cam_->view()->setVertex16f(0,view_);
    cam_->viewInverse()->setVertex16f(0,viewInv_);
    cam_->viewProjection()->setVertex16f(0,viewproj_);
    cam_->viewProjectionInverse()->setVertex16f(0,viewprojInv_);

    if(cam_->isAudioListener()) {
      AudioListener::set3f(AL_POSITION, position_);
      AudioListener::set3f(AL_VELOCITY, velocity_);
      AudioListener::set6f(AL_ORIENTATION, Vec6f(direction_, Vec3f::up()));
    }
  } unlock();
}

////////////////

EgoCameraManipulator::EgoCameraManipulator(const ref_ptr<Camera> &cam)
: CameraManipulator(cam)
{
  pos_ = position_;
  dir_ = direction_;
  dirXZ_ = Vec3f(dir_.x, 0.0f, dir_.z);
  dirXZ_.normalize();
  dirSidestep_ = dirXZ_.cross(Vec3f::up());
  moveAmount_ = 1.0;
  moveForward_ = GL_FALSE;
  moveBackward_ = GL_FALSE;
  moveLeft_ = GL_FALSE;
  moveRight_ = GL_FALSE;
}

void EgoCameraManipulator::set_position(const Vec3f &position)
{ pos_ = position; }
void EgoCameraManipulator::set_direction(const Vec3f &direction)
{ dir_ = direction; }

void EgoCameraManipulator::set_moveAmount(GLfloat moveAmount)
{ moveAmount_ = moveAmount; }

void EgoCameraManipulator::moveForward(GLboolean v)
{ moveForward_ = v; }
void EgoCameraManipulator::moveBackward(GLboolean v)
{ moveBackward_ = v; }
void EgoCameraManipulator::moveLeft(GLboolean v)
{ moveLeft_ = v; }
void EgoCameraManipulator::moveRight(GLboolean v)
{ moveRight_ = v; }

void EgoCameraManipulator::stepForward(const GLfloat &v)
{ step(dirXZ_*v); }
void EgoCameraManipulator::stepBackward(const GLfloat &v)
{ step(dirXZ_*(-v)); }
void EgoCameraManipulator::stepLeft(const GLfloat &v)
{ step(dirSidestep_*(-v)); }
void EgoCameraManipulator::stepRight(const GLfloat &v)
{ step(dirSidestep_*v); }
void EgoCameraManipulator::step(const Vec3f &v)
{ pos_ += v; }

void EgoCameraManipulator::lookUp(GLfloat amount)
{
  rot_.setAxisAngle(dirSidestep_, amount);
  dir_ = rot_.rotate(dir_);

  dirXZ_ = Vec3f(dir_.x, 0.0f, dir_.z);
  dirXZ_.normalize();
  dirSidestep_ = dirXZ_.cross(Vec3f::up());
}
void EgoCameraManipulator::lookDown(GLfloat amount)
{
  rot_.setAxisAngle(dirSidestep_, amount);
  dir_ = rot_.rotate(dir_);

  dirXZ_ = Vec3f(dir_.x, 0.0f, dir_.z);
  dirXZ_.normalize();
  dirSidestep_ = dirXZ_.cross(Vec3f::up());
}
void EgoCameraManipulator::lookLeft(GLfloat amount)
{
  rot_.setAxisAngle(Vec3f::up(), amount);
  dir_ = rot_.rotate(dir_);

  dirXZ_ = Vec3f(dir_.x, 0.0f, dir_.z);
  dirXZ_.normalize();
  dirSidestep_ = dirXZ_.cross(Vec3f::up());
}
void EgoCameraManipulator::lookRight(GLfloat amount)
{
  rot_.setAxisAngle(Vec3f::up(), -amount);
  dir_ = rot_.rotate(dir_);

  dirXZ_ = Vec3f(dir_.x, 0.0f, dir_.z);
  dirXZ_.normalize();
  dirSidestep_ = dirXZ_.cross(Vec3f::up());
}

void EgoCameraManipulator::animate(GLdouble dt)
{
  Mat4f &proj = *(Mat4f*)cam_->projection()->ownedData();
  Mat4f &projInv = *(Mat4f*)cam_->projectionInverse()->ownedData();

  if(moveForward_)       stepForward(moveAmount_*dt);
  else if(moveBackward_) stepBackward(moveAmount_*dt);
  if(moveLeft_)          stepLeft(moveAmount_*dt);
  else if(moveRight_)    stepRight(moveAmount_*dt);

  lock(); {
    position_ = pos_;
    direction_ = dir_;
    // update the camera velocity
    if(dt > 1e-6) {
      velocity_ = (lastPosition_ - position_) / dt;
      lastPosition_ = position_;
    }

    view_ = Mat4f::lookAtMatrix(position_, direction_, Vec3f::up());
    viewInv_ = view_.lookAtInverse();
    viewproj_ = view_ * proj;
    viewprojInv_ = projInv * viewInv_;
  } unlock();
}

////////////////

LookAtCameraManipulator::LookAtCameraManipulator(
    const ref_ptr<Camera> &cam,
    GLint intervalMiliseconds)
: CameraManipulator(cam),
  lookAt_( Vec3f(0.0f, 0.0f, 0.0f) ),
  radius_( 4.0f ),
  height_( 2.0f ),
  deg_(0.0f),
  stepLength_(1.0f),
  intervalMiliseconds_((GLfloat)intervalMiliseconds)
{
}

void LookAtCameraManipulator::set_lookAt(
    const Vec3f &lookAt, const GLdouble &dt)
{ lookAt_.setDestination(lookAt, dt); }
void LookAtCameraManipulator::set_degree(
    GLfloat degree, const GLdouble &dt)
{ deg_.setDestination(degree, dt); }
void LookAtCameraManipulator::set_radius(
    GLfloat radius, const GLdouble &dt)
{ radius_.setDestination(radius, dt); }

void LookAtCameraManipulator::set_height(
    GLfloat height, const GLdouble &dt)
{ height_.setDestination(height, dt); }

void LookAtCameraManipulator::setStepLength(
    GLfloat length, const GLdouble &dt)
{ stepLength_.setDestination(length, dt); }

GLfloat LookAtCameraManipulator::height() const
{ return height_.value(); }
GLfloat LookAtCameraManipulator::radius() const
{ return radius_.value(); }

void LookAtCameraManipulator::animate(GLdouble dt)
{
  const GLdouble &step = stepLength_.value(dt);
  const GLdouble &degStep = step*(dt/intervalMiliseconds_);
  const GLdouble &radius = radius_.value(dt);
  const GLdouble &height = height_.value(dt);
  const Vec3f &lookAt = lookAt_.value(dt);

  deg_.src_ += degStep;
  deg_.dst_ += degStep;
  const GLdouble &deg = deg_.value(dt);

  Mat4f &proj = *(Mat4f*)cam_->projection()->ownedData();
  Mat4f &projInv = *(Mat4f*)cam_->projectionInverse()->ownedData();

  lock(); {
    position_ = lookAt + Vec3f(
        radius*sin(deg), height, radius*cos(deg));
    direction_ = (lookAt - position_);
    direction_.normalize();
    // update the camera velocity
    if(dt > 1e-6) {
      velocity_ = (lastPosition_ - position_) / dt;
      lastPosition_ = position_;
    }

    view_ = Mat4f::lookAtMatrix(position_, direction_, Vec3f::up());
    viewInv_ = view_.lookAtInverse();
    viewproj_ = view_ * proj;
    viewprojInv_ = projInv * viewInv_;
  } unlock();
}
