/*
 * animation.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef GL_ANIMATION_H_
#define GL_ANIMATION_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>

using namespace std;

#include <ogle/animations/buffer-data.h>
#include <ogle/utility/event-object.h>

/**
 * Abstract baseclass for animations.
 * doAnimate(dt) must be implemented by subclasses.
 *
 * NOTE: you cannot use gl* methods in doAnimate because
 * the thread this method is called in does not have a gl context.
 * You are just operating on data pointer acquired in the main thread.
 */
class Animation : public EventObject {
public:
  Animation();

  /**
   * Time of last step.
   */
  const double& elapsedTime() const;
  void set_elapsedTime(const double &time);

  /**
   * Mutex lock for data access.
   * Returns false if not successful.
   */
  bool try_lock();
  /**
   * Mutex lock for data access.
   * Blocks until lock can be acquired.
   */
  void lock();
  /**
   * Mutex lock for data access.
   * Unlocks a previously acquired lock.
   */
  void unlock();

  virtual void animate(const double &milliSeconds);
  virtual void updateAnimationGraphics(const double &dt) {}
private:
  double elapsedTime_;
  GLuint isRemoved_;
  boost::mutex mutex_;

  /**
   * Manipulate the data.
   * @return true if anything changed
   */
  virtual void doAnimate(const double &milliSeconds) = 0;
};

#endif /* GL_ANIMATION_H_ */
