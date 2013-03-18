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

namespace ogle {

/**
 * \brief Abstract base class for animations.
 */
class Animation : public EventObject
{
public:
  /**
   * @param useGLAnimation execute with render context.
   * @param useAnimation execute without render context in separate thread.
   */
  Animation(GLboolean useGLAnimation, GLboolean useAnimation);
  virtual ~Animation() {}

  /**
   * Mutex lock for data access.
   * @return false if not successful.
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
   * @return true if the animation implements glAnimate().
   */
  GLboolean useGLAnimation() const;
  /**
   * @return true if the animation implements animate().
   */
  GLboolean useAnimation() const;

  /**
   * Make the next animation step.
   * This should be called each frame.
   * @param dt time difference to last call in milliseconds.
   */
  virtual void animate(GLdouble dt){}
  /**
   * Upload animation data to GL.
   * This should be called each frame in a thread
   * with a GL context.
   * @param rs the render state.
   * @param dt time difference to last call in milliseconds.
   */
  virtual void glAnimate(RenderState *rs, GLdouble dt){}

private:
  boost::mutex mutex_;
  GLboolean useGLAnimation_;
  GLboolean useAnimation_;
};

} // end ogle namespace

#endif /* GL_ANIMATION_H_ */
