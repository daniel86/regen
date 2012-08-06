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
#include <ogle/animations/vbo-animation.h>
#include <ogle/animations/animation-buffer.h>

/**
 * Singleton AnmationManager.
 * Uses a separate thread to manipulate the data.
 */
class AnimationManager
{
public:
  static AnimationManager& get();

  /**
   * AnimationManager keeps a reference on the animation.
   */
  void addAnimation(ref_ptr<Animation> animation,
          GLenum bufferAccess=GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);
  void removeAnimation(ref_ptr<Animation> animation);

  /**
   * Copy animation buffer to primitive buffer if needed.
   */
  void updateGraphics(const double &dt, list<GLuint> buffers);

  void waitForStep();
  void nextFrame();

  void pauseAllAnimations();
  void resumeAllAnimations();

private:
  typedef map<GLuint, AnimationBuffer*> AnimationBuffers;

  ///// main thread only
  boost::thread animationThread_;
  AnimationBuffers animationBuffers_;
  map< Animation*, AnimationIterator > animationToBuffer_;

  ///// animation thread only
  boost::posix_time::ptime time_;
  boost::posix_time::ptime lastTime_;
  list< ref_ptr<Animation> > animations_;

  ///// shared
  list< ref_ptr<Animation> > newAnimations_;
  list< ref_ptr<Animation> > removedAnimations_;
  boost::mutex animationLock_;
  bool closeFlag_;
  bool pauseFlag_;

  boost::mutex stepMut_;
  boost::mutex frameMut_;
  boost::condition_variable stepCond_;
  boost::condition_variable frameCond_;
  bool hasNextFrame_;
  bool hasNextStep_;

  AnimationManager();
  ~AnimationManager();

  void run();

  void nextStep();
  void waitForFrame();
};

#endif /* GL_ANIMATION_MANAGER_H_ */
