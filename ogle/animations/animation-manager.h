/*
 * animation-manager.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef GL_ANIMATION_MANAGER_H_
#define GL_ANIMATION_MANAGER_H_

#include <list>
#include <set>
using namespace std;

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <ogle/animations/animation.h>

/**
 * Singleton AnmationManager.
 * Uses a separate thread to manipulate the data.
 */
class AnimationManager
{
public:
  static AnimationManager& get();

  /**
   * Adds an animation.
   */
  void addAnimation(ref_ptr<Animation> animation,
          GLenum bufferAccess=GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);
  /**
   * Removes previously added animation.
   */
  void removeAnimation(ref_ptr<Animation> animation);

  /**
   * Copy animation buffer to primitive buffer if needed.
   */
  void updateGraphics(GLdouble dt);

  /**
   * Wait until next step was calculated in animation thread.
   */
  void waitForStep();
  /**
   * Wake up the animation thread if it is waiting for
   * the next frame to finish.
   */
  void nextFrame();

  /**
   * Pause animations.
   * Can be resumed by call to resume().
   */
  void pause();
  /**
   * Resumes previously paused animations.
   */
  void resume();

private:
  ///// main thread only
  boost::thread animationThread_;
  list< ref_ptr<Animation> > removedAnimations__;

  ///// animation thread only
  boost::posix_time::ptime time_;
  boost::posix_time::ptime lastTime_;
  list< ref_ptr<Animation> > animations_;

  ///// shared
  list< ref_ptr<Animation> > newAnimations_;
  list< ref_ptr<Animation> > removedAnimations_;
  boost::mutex animationLock_;
  GLboolean closeFlag_;
  GLboolean pauseFlag_;

  boost::mutex stepMut_;
  boost::mutex frameMut_;
  boost::condition_variable stepCond_;
  boost::condition_variable frameCond_;
  GLboolean hasNextFrame_;
  GLboolean hasNextStep_;

  AnimationManager();
  ~AnimationManager();

  void run();

  void nextStep();
  void waitForFrame();
};

#endif /* GL_ANIMATION_MANAGER_H_ */
