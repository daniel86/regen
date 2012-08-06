/*
 * camera-manipulator.cpp
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#include "camera-manipulator.h"

CameraManipulator::CameraManipulator(
    ref_ptr<Camera> cam,
    int intervalMiliseconds)
: Animation(),
  cam_(cam),
  intervalMiliseconds_((double)intervalMiliseconds)
{
}

void CameraManipulator::doAnimate(
    const double &milliSeconds)
{
  manipulateCamera(milliSeconds);
}

////////////////

CameraLinearPositionManipulator::CameraLinearPositionManipulator(
    ref_ptr<Camera> cam,
    int intervalMiliseconds)
: CameraManipulator(cam,intervalMiliseconds),
  arrived_(true),
  destination_( (Vec3f) {0.0f, 0.0f, 0.0f} ),
  stepLength_(1.0f)
{
}

void CameraLinearPositionManipulator::setDestinationPosition(const Vec3f &destination)
{
  destination_ = destination;
  arrived_ = false;
}

void CameraLinearPositionManipulator::setStepLength(float length)
{
  stepLength_ = length;
}

void CameraLinearPositionManipulator::manipulateCamera(const double &dt)
{
  if(arrived_) return;

  const Vec3f &start = cam_->position();
  Vec3f diff = destination_ - start;
  double step = stepLength_*(dt/intervalMiliseconds_);
  if( length(diff) < step ) {
    arrived_ = true;
    cam_->set_position( destination_ );
  } else {
    normalize(diff);
    cam_->set_position( start + diff*step );
  }
  cam_->updateMatrix(dt);
}

////////////////

LookAtCameraManipulator::LookAtCameraManipulator(
    ref_ptr<Camera> cam,
    int intervalMiliseconds)
: CameraManipulator(cam,intervalMiliseconds),
  lookAt_( (Vec3f) {0.0f, 0.0f, 0.0f} ),
  radius_( 4.0f ),
  height_( 2.0f ),
  stepLength_(1.0f),
  deg_(0.0f)
{
}

void LookAtCameraManipulator::manipulateCamera(const double &dt)
{
  const float &step = stepLength_.value(dt);
  const float &degStep = step*(dt/intervalMiliseconds_);
  const float &radius = radius_.value(dt);
  const float &height = height_.value(dt);
  const Vec3f &lookAt = lookAt_.value(dt);

  deg_.src_ += degStep;
  deg_.dst_ += degStep;

  const float &deg = deg_.value(dt);

  Vec3f pos = lookAt + Vec3f(
      radius*sin(deg), height, radius*cos(deg));

  cam_->set_position(pos);

  Vec3f direction = (lookAt - pos);
  normalize(direction);
  cam_->set_direction(direction);
  cam_->updateMatrix(dt);
}
