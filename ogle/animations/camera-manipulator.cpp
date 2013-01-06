/*
 * camera-manipulator.cpp
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#include "camera-manipulator.h"

CameraManipulator::CameraManipulator(
    const ref_ptr<PerspectiveCamera> &cam,
    GLint intervalMiliseconds)
: Animation(),
  cam_(cam),
  intervalMiliseconds_((GLfloat)intervalMiliseconds)
{
}

void CameraManipulator::animate(GLdouble dt)
{
  manipulateCamera(dt);
}
void CameraManipulator::glAnimate(GLdouble dt)
{
  cam_->update(dt);
}
GLboolean CameraManipulator::useGLAnimation() const
{
  return GL_TRUE;
}
GLboolean CameraManipulator::useAnimation() const
{
  return GL_TRUE;
}

////////////////

CameraLinearPositionManipulator::CameraLinearPositionManipulator(
    const ref_ptr<PerspectiveCamera> &cam,
    GLint intervalMiliseconds)
: CameraManipulator(cam,intervalMiliseconds),
  destination_(0.0f),
  stepLength_(1.0f),
  arrived_(true)
{
}

void CameraLinearPositionManipulator::setDestinationPosition(const Vec3f &destination)
{
  destination_ = destination;
  arrived_ = false;
}

void CameraLinearPositionManipulator::setStepLength(GLdouble length)
{
  stepLength_ = length;
}

void CameraLinearPositionManipulator::manipulateCamera(const GLdouble &dt)
{
  if(arrived_) return;

  const Vec3f &start = cam_->position();
  Vec3f diff = destination_ - start;
  GLfloat step = stepLength_*(dt/intervalMiliseconds_);
  if( diff.length() < step ) {
    arrived_ = true;
    cam_->set_position( destination_ );
  } else {
    diff.normalize();
    cam_->set_position( start + diff*step );
  }
  cam_->updatePerspective(dt);
}

////////////////

LookAtCameraManipulator::LookAtCameraManipulator(
    const ref_ptr<PerspectiveCamera> &cam,
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

void LookAtCameraManipulator::manipulateCamera(const GLdouble &dt)
{
  const GLdouble &step = stepLength_.value(dt);
  const GLdouble &degStep = step*(dt/intervalMiliseconds_);
  const GLdouble &radius = radius_.value(dt);
  const GLdouble &height = height_.value(dt);
  const Vec3f &lookAt = lookAt_.value(dt);

  deg_.src_ += degStep;
  deg_.dst_ += degStep;

  const GLdouble &deg = deg_.value(dt);

  Vec3f pos = lookAt + Vec3f(
      radius*sin(deg), height, radius*cos(deg));

  cam_->set_position(pos);

  Vec3f direction = (lookAt - pos);
  direction.normalize();
  cam_->set_direction(direction);
  cam_->updatePerspective(dt);
}
