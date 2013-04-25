/*
 * animation.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef GL_ANIMATION_H_
#define GL_ANIMATION_H_

#include <GL/glew.h>
#include <boost/thread/mutex.hpp>

#include <regen/utility/event-object.h>
#include <regen/gl-types/render-state.h>

namespace regen {

/**
 * \brief Abstract base class for animations.
 */
class Animation : public EventObject
{
public:
  /**
   * The animation state switched to active.
   */
  static GLuint ANIMATION_STARTED;
  /**
   * The animation state switched to inactive.
   */
  static GLuint ANIMATION_STOPPED;

  /**
   * Create and activate an animation.
   * @param useGLAnimation execute with render context.
   * @param useAnimation execute without render context in separate thread.
   */
  Animation(GLboolean useGLAnimation, GLboolean useAnimation);
  virtual ~Animation();

  /**
   * @return true if this animation is active.
   */
  GLboolean isRunning() const;

  /**
   * Activate this animation.
   */
  void startAnimation();
  /**
   * Deactivate this animation.
   */
  void stopAnimation();

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
   * Waits for a while.
   * @param milliseconds number of ms to wait
   */
  void wait(GLuint milliseconds);

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
  GLboolean isRunning_;

  Animation(const Animation&);
  void operator=(const Animation&);
};

} // namespace

#endif /* GL_ANIMATION_H_ */
