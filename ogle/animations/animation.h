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
#include <boost/thread/mutex.hpp>

#include <ogle/utility/event-object.h>
#include <ogle/gl-types/render-state.h>

/**
 * Abstract base class for animations.
 */
class Animation : public EventObject
{
public:
  Animation();
  virtual ~Animation() {}

  /**
   * Mutex lock for data access.
   * Returns false if not successful.
   */
  GLboolean try_lock();
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

  /**
   * Make the next animation step.
   * This should be called each frame.
   */
  virtual void animate(GLdouble dt) = 0;
  /**
   * Upload animation data to GL.
   * This should be called each frame in a thread
   * with a GL context.
   */
  virtual void glAnimate(RenderState *rs, GLdouble dt) = 0;
  virtual GLboolean useGLAnimation() const = 0;
  virtual GLboolean useAnimation() const = 0;
private:
  boost::mutex mutex_;
};

#endif /* GL_ANIMATION_H_ */
