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

namespace ogle {
/**
 * \brief Manipulates the view/projection matrix of a camera.
 */
class CameraManipulator : public Animation
{
public:
  /**
   * @param cam the camera to manipulate
   * @param interval interval for camera manipulation in ms
   */
  CameraManipulator(const ref_ptr<PerspectiveCamera> &cam, GLint interval);

  // override
  virtual void glAnimate(RenderState *rs, GLdouble dt);
  virtual GLboolean useGLAnimation() const;
  virtual GLboolean useAnimation() const;

protected:
  ref_ptr<PerspectiveCamera> cam_;
  GLdouble intervalMiliseconds_;
};

/**
 * \brief Camera manipulator that looks at a given position.
 */
class LookAtCameraManipulator : public CameraManipulator
{
public:
  LookAtCameraManipulator(const ref_ptr<PerspectiveCamera> &cam, GLint interval);

  /**
   * @param lookAt the look at position.
   * @param dt time difference to last call in milliseconds.
   */
  void set_lookAt(const Vec3f &lookAt, const GLdouble &dt=0.0);
  /**
   * @param degree degree of rotation around the position.
   * @param dt time difference to last call in milliseconds.
   */
  void set_degree(GLfloat degree, const GLdouble &dt=0.0);
  /**
   * @param radius distance to look at point in xz plane.
   * @param dt time difference to last call in milliseconds.
   */
  void set_radius(GLfloat radius, const GLdouble &dt=0.0);

  /**
   * @param height distance to look at point y direction.
   * @param dt time difference to last call in milliseconds.
   */
  void set_height(GLfloat height, const GLdouble &dt=0.0);

  /**
   * @param length animation step size.
   * @param dt time difference to last call in milliseconds.
   */
  void setStepLength(GLfloat length, const GLdouble &dt=0.0);

  /**
   * @return the camera height.
   */
  GLfloat height() const;
  /**
   * @return the camera radius.
   */
  GLfloat radius() const;

  // override
  virtual void animate(GLdouble dt);

protected:
  template<class T> class KeyFrame
  {
  public:
    T src_;
    T dst_;
    GLdouble dt_;
    KeyFrame(const T &initialValue)
    : src_(initialValue), dst_(initialValue), dt_(0.0) {}

    void setDestination(const T &dst, const GLdouble &dt)
    {
      dst_ = dst;
      dt_ = dt;
    }
    const T& value() const
    {
      return src_;
    }
    const T& value(const GLdouble &dt)
    {
      if(dt > dt_) {
        dt_ = 0.0;
        src_ = dst_;
      } else {
        GLdouble factor = (dt/dt_);
        dt_ -= dt;
        src_ += (dst_-src_)*factor;
      }
      return src_;
    }
  };

  KeyFrame<Vec3f> lookAt_;
  KeyFrame<GLdouble> radius_;
  KeyFrame<GLdouble> height_;
  KeyFrame<GLdouble> deg_;
  KeyFrame<GLdouble> stepLength_;
};

} // end ogle namespace

#endif /* CAMERA_MANIPULATOR_H_ */
