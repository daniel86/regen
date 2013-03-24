/*
 * camera-manipulator.cpp
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#include <regen/av/audio.h>

#include "camera-manipulator.h"
using namespace ogle;

CameraManipulator::CameraManipulator(
    const ref_ptr<Camera> &cam,
    GLint intervalMiliseconds)
: Animation(GL_TRUE,GL_TRUE),
  cam_(cam),
  intervalMiliseconds_((GLfloat)intervalMiliseconds)
{
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
      AudioSystem &audio = AudioSystem::get();
      audio.set_listenerPosition( position_ );
      audio.set_listenerVelocity( velocity_ );
      audio.set_listenerOrientation( direction_, Vec3f::up() );
    }
  } unlock();
}

////////////////

LookAtCameraManipulator::LookAtCameraManipulator(
    const ref_ptr<Camera> &cam,
    GLint intervalMiliseconds)
: CameraManipulator(cam,intervalMiliseconds),
  lookAt_( Vec3f(0.0f, 0.0f, 0.0f) ),
  radius_( 4.0f ),
  height_( 2.0f ),
  deg_(0.0f),
  stepLength_(1.0f)
{
}

void LookAtCameraManipulator::set_lookAt(
    const Vec3f &lookAt, const GLdouble &dt)
{
  lookAt_.setDestination(lookAt, dt);
}
void LookAtCameraManipulator::set_degree(
    GLfloat degree, const GLdouble &dt)
{
  deg_.setDestination(degree, dt);
}
void LookAtCameraManipulator::set_radius(
    GLfloat radius, const GLdouble &dt)
{
  radius_.setDestination(radius, dt);
}

void LookAtCameraManipulator::set_height(
    GLfloat height, const GLdouble &dt)
{
  height_.setDestination(height, dt);
}

void LookAtCameraManipulator::setStepLength(
    GLfloat length, const GLdouble &dt)
{
  stepLength_.setDestination(length, dt);
}

GLfloat LookAtCameraManipulator::height() const
{
  return height_.value();
}
GLfloat LookAtCameraManipulator::radius() const
{
  return radius_.value();
}

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
