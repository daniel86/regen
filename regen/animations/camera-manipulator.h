/*
 * camera-manipulator.h
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#ifndef CAMERA_MANIPULATOR_H_
#define CAMERA_MANIPULATOR_H_

#include <regen/algebra/quaternion.h>
#include <regen/states/camera.h>
#include <regen/animations/animation.h>

namespace regen {
/**
 * \brief Manipulates the view matrix of a Camera.
 */
class CameraManipulator : public Animation
{
public:
  /**
   * @param cam the camera to manipulate
   * @param interval interval for camera manipulation in ms
   */
  CameraManipulator(const ref_ptr<Camera> &cam);

  // override
  void glAnimate(RenderState *rs, GLdouble dt);

protected:
  ref_ptr<Camera> cam_;

  Vec3f position_;
  Vec3f direction_;
  Vec3f velocity_;
  Vec3f lastPosition_;

  GLfloat fov_;
  GLfloat near_;
  GLfloat far_;

  Mat4f view_;
  Mat4f viewInv_;
  Mat4f viewproj_;
  Mat4f viewprojInv_;
};

/**
 * Ego-Perspective camera.
 * Translation is only done in xz-plane.
 */
class EgoCameraManipulator : public CameraManipulator
{
public:
  /**
   * @param cam the camera to manipulate
   */
  EgoCameraManipulator(const ref_ptr<Camera> &cam);

  /**
   * @param v move velocity.
   */
  void set_moveAmount(GLfloat v);

  /**
   * @param v moving forward toggle.
   */
  void moveForward(GLboolean v);
  /**
   * @param v moving backward toggle.
   */
  void moveBackward(GLboolean v);
  /**
   * @param v moving left toggle.
   */
  void moveLeft(GLboolean v);
  /**
   * @param v moving right toggle.
   */
  void moveRight(GLboolean v);

  /**
   * @param v the amount to step forward.
   */
  void stepForward(const GLfloat &v);
  /**
   * @param v the amount to step backward.
   */
  void stepBackward(const GLfloat &v);
  /**
   * @param v the amount to step left.
   */
  void stepLeft(const GLfloat &v);
  /**
   * @param v the amount to step right.
   */
  void stepRight(const GLfloat &v);

  /**
   * @param v the amount to change the position.
   */
  void step(const Vec3f &v);

  /**
   * @param v the amount of camera direction change in up direction.
   */
  void lookUp(GLfloat v);
  /**
   * @param v the amount of camera direction change in down direction.
   */
  void lookDown(GLfloat v);
  /**
   * @param v the amount of camera direction change in left direction.
   */
  void lookLeft(GLfloat v);
  /**
   * @param v the amount of camera direction change in right direction.
   */
  void lookRight(GLfloat v);

  /**
   * @param position the camera position.
   */
  void set_position(const Vec3f &position);
  /**
   * @param position the camera direction.
   */
  void set_direction(const Vec3f &direction);

  // override
  void animate(GLdouble dt);

protected:
  Vec3f pos_;
  Vec3f dir_;
  Vec3f dirXZ_;
  Vec3f dirSidestep_;
  GLfloat moveAmount_;
  Quaternion rot_;

  GLboolean moveForward_;
  GLboolean moveBackward_;
  GLboolean moveLeft_;
  GLboolean moveRight_;
};

/**
 * \brief Camera manipulator that looks at a given position.
 */
class LookAtCameraManipulator : public CameraManipulator
{
public:
  /**
   * @param cam a perspective camera.
   * @param interval update interval.
   */
  LookAtCameraManipulator(const ref_ptr<Camera> &cam, GLint interval);

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
  void animate(GLdouble dt);

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
  GLdouble intervalMiliseconds_;
};

} // namespace

#endif /* CAMERA_MANIPULATOR_H_ */
