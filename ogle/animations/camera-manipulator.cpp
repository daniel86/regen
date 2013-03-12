/*
 * camera-manipulator.cpp
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#include "camera-manipulator.h"
using namespace ogle;

CameraManipulator::CameraManipulator(
    const ref_ptr<Camera> &cam,
    GLint intervalMiliseconds)
: Animation(),
  cam_(cam),
  intervalMiliseconds_((GLfloat)intervalMiliseconds)
{
}

void CameraManipulator::glAnimate(RenderState *rs, GLdouble dt)
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

  Vec3f pos = lookAt + Vec3f(
      radius*sin(deg), height, radius*cos(deg));

  cam_->set_position(pos);

  Vec3f direction = (lookAt - pos);
  direction.normalize();
  cam_->set_direction(direction);
  cam_->updatePerspective(dt);
}
