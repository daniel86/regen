/*
 * camera-manipulator.h
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#ifndef CAMERA_MANIPULATOR_H_
#define CAMERA_MANIPULATOR_H_

#include <ogle/states/camera.h>
#include <ogle/animations/animation.h>

/**
 * Base class that modifies camera properties as timeout.
 */
class CameraManipulator : public Animation {
public:
  /**
   * @param cam the camera to manipulate
   * @param intervalMiliseconds interval for camera manipulation
   */
  CameraManipulator(ref_ptr<Camera> cam, int intervalMiliseconds);

  /**
   * Supposed to manipulate the camera.
   */
  virtual void manipulateCamera(const double &milliSeconds) = 0;

protected:
  ref_ptr<Camera> cam_;
  double intervalMiliseconds_;

  // overwrite
  virtual void doAnimate(const double &milliSeconds);
};

/**
 * Linear interpolation of values.
 * Used to get continuous animations for mouse motion
 * events and similar.
 */
template<class T>
class ValueKeyFrame {
public:
  T src_;
  T dst_;
  double dt_;
  ValueKeyFrame(const T &initialValue)
  : src_(initialValue),
    dst_(initialValue),
    dt_(0.0)
  {
  }
  void setDestination(const T &dst, const double &dt) {
    dst_ = dst;
    dt_ = dt;
  }
  const T& value() const {
    return src_;
  }
  const T& value(const double &dt) {
    if(dt > dt_) {
      dt_ = 0.0;
      src_ = dst_;
    } else {
      double factor = (dt/dt_);
      dt_ -= dt;
      src_ += (dst_-src_)*factor;
    }
    return src_;
  }
};

/**
 * Camera manipulator that looks at a given position.
 */
class LookAtCameraManipulator : public CameraManipulator {
public:
  LookAtCameraManipulator(ref_ptr<Camera> cam, int intervalMiliseconds);

  /**
   * the look at position.
   */
  void set_lookAt(const Vec3f &lookAt, const double &dt=0.0) {
    lookAt_.setDestination(lookAt, dt);
  }
  /**
   * Degree of rotation around the position.
   */
  void set_degree(float degree, const double &dt=0.0) {
    deg_.setDestination(degree, dt);
  }
  /**
   * Distance to look at point in xz plane.
   */
  void set_radius(float radius, const double &dt=0.0) {
    radius_.setDestination(radius, dt);
  }

  /**
   * Distance to look at point y direction.
   */
  void set_height(float height, const double &dt=0.0) {
    height_.setDestination(height, dt);
  }

  /**
   * camera will move length units each timestep
   */
  void setStepLength(float length, const double &dt=0.0) {
    stepLength_.setDestination(length, dt);
  }

  float height() const {
    return height_.value();
  }
  float radius() const {
    return radius_.value();
  }

  // override
  virtual void manipulateCamera(const double &milliSeconds);

protected:
  ValueKeyFrame<Vec3f> lookAt_;
  ValueKeyFrame<float> radius_;
  ValueKeyFrame<float> height_;
  ValueKeyFrame<float> deg_;
  ValueKeyFrame<float> stepLength_;
};

/**
 * Interpolate linear between current posiion and destination.
 */
class CameraLinearPositionManipulator : public CameraManipulator {
public:
  CameraLinearPositionManipulator(ref_ptr<Camera> cam, int intervalMiliseconds);

  /**
   * camera will move to this position (not changing direction)
   */
  void setDestinationPosition(const Vec3f &destination);
  /**
   * camera will move length units each timestep
   */
  void setStepLength(float length);

  // override
  virtual void manipulateCamera(const double &milliSeconds);

protected:
  Vec3f destination_;
  float stepLength_;
  bool arrived_;
};

#endif /* CAMERA_MANIPULATOR_H_ */
