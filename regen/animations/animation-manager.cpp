/*
 * animation-manager.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <map>
#include <limits.h>

#include <regen/config.h>
#include <regen/utility/threading.h>
#include <regen/utility/logging.h>

#include "animation-manager.h"
using namespace regen;

// Microseconds to sleep per loop in idle mode.
#define IDLE_SLEEP 100000
// Synchronize animation and render thread.
#define SYNCHRONIZE_THREADS

AnimationManager& AnimationManager::get()
{
  static AnimationManager manager;
  return manager;
}

AnimationManager::AnimationManager()
: Thread(),
  animationInProgress_(GL_FALSE),
  closeFlag_(GL_FALSE),
  pauseFlag_(GL_TRUE),
  hasNextFrame_(GL_FALSE),
  hasNextStep_(GL_FALSE)
{
  time_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastTime_ = time_;
}

AnimationManager::~AnimationManager()
{
  threadLock_.lock(); {
    closeFlag_ = GL_TRUE;
  } threadLock_.unlock();
  nextFrame();
  thread_.join();
}

void AnimationManager::addAnimation(Animation *animation)
{
  // queue adding the animation in the animation thread
  threadLock_.lock(); { // lock shared newAnimations_
    if(animation->useAnimation()) {
      for(list<Animation*>::iterator
          it=removedAnimations_.begin(); it!=removedAnimations_.end(); ++it) {
        if(*it == animation) {
          removedAnimations_.erase(it);
          break;
        }
      }
      newAnimations_.push_back(animation);
    }
  } threadLock_.unlock();

  graphicsLock_.lock(); {
    if(animation->useGLAnimation()) {
      removedGLAnimations_.erase(animation);
      glAnimations_.insert(animation);
    }
  } graphicsLock_.unlock();
}
void AnimationManager::removeAnimation(Animation *animation)
{
  threadLock_.lock(); {
    if(animation->useAnimation()) {
      for(list<Animation*>::iterator
          it=newAnimations_.begin(); it!=newAnimations_.end(); ++it) {
        if(*it == animation) {
          newAnimations_.erase(it);
          break;
        }
      }
      removedAnimations_.push_back(animation);
    }
  } threadLock_.unlock();
  if(animation->useAnimation()) {
    while(animationInProgress_) usleepRegen(1000);
  }

  graphicsLock_.lock(); {
    if(animation->useGLAnimation()) {
      glAnimations_.erase(animation);
    }
  } graphicsLock_.unlock();
}

void AnimationManager::updateGraphics(RenderState *rs, GLdouble dt)
{
  if(pauseFlag_) return;

#ifdef SYNCHRONIZE_THREADS
  nextFrame();
#endif

  graphicsLock_.lock(); {
    // remove animations
    set<Animation*>::iterator it, jt;
    for(it = removedGLAnimations_.begin(); it!=removedGLAnimations_.end(); ++it)
    {
      glAnimations_.erase(*it);
    }
    removedGLAnimations_.clear();
  } graphicsLock_.unlock();

  // update animations
  GLboolean reloadAnimations_ = GL_TRUE;
  set<Animation*> anims, processed;
  for(GLboolean animationsProcessed=GL_FALSE; !animationsProcessed;) {
    if(reloadAnimations_) {
      anims.clear();
      graphicsLock_.lock(); {
        anims.insert(glAnimations_.begin(), glAnimations_.end());
      } graphicsLock_.unlock();
      reloadAnimations_ = GL_FALSE;
    }

    animationsProcessed = GL_TRUE;
    for(set<Animation*>::iterator
        it=anims.begin(); it!=anims.end(); ++it) {
      Animation *anim = *it;
      if(processed.count(anim)>0 ||
        glAnimations_.count(anim)==0) continue;
      processed.insert(anim);

      if(anim->isRunning()) anim->glAnimate(rs,dt);

      if(anims.size()!=glAnimations_.size()) {
        // glAnimate modified the list of active animations
        reloadAnimations_ = GL_TRUE;
        break;
      }
    }
  }

#ifdef SYNCHRONIZE_THREADS
  waitForStep();
#endif
}

void AnimationManager::nextFrame()
{
  // set the next frame condition to true
  // and notify waitForFrame if it is waiting.
  // waitForStep waits only if it was faster to render
  // a new frame then calculating the next animation step
  {
    boost::lock_guard<boost::mutex> lock(frameMut_);
    hasNextFrame_ = GL_TRUE;
  }
  frameCond_.notify_all();
}

void AnimationManager::nextStep()
{
  // set the next step condition to true
  // and notify waitForStep if it is waiting.
  // waitForStep waits only if it was faster to render
  // a new frame then calculating the next animation step
  {
    boost::lock_guard<boost::mutex> lock(stepMut_);
    hasNextStep_ = GL_TRUE;
  }
  stepCond_.notify_all();
}

void AnimationManager::waitForFrame()
{
  // wait until a new frame is rendered.
  {
    boost::unique_lock<boost::mutex> lock(frameMut_);
    while(!hasNextFrame_) {
      frameCond_.wait(lock);
    }
  }
  // toggle hasNextFrame_ to false
  {
    boost::lock_guard<boost::mutex> lock(frameMut_);
    hasNextFrame_ = GL_FALSE;
  }
}

void AnimationManager::waitForStep()
{
  // wait until a new animation step was calculated.
  {
    boost::unique_lock<boost::mutex> lock(stepMut_);
    while(!hasNextStep_) {
      stepCond_.wait(lock);
    }
  }
  {
    boost::lock_guard<boost::mutex> lock(stepMut_);
    hasNextStep_ = GL_FALSE;
  }
}

void AnimationManager::run()
{
  while(GL_TRUE) {
    time_ = boost::posix_time::ptime(
        boost::posix_time::microsec_clock::local_time());

    // break loop and close thread if requested.
    if(closeFlag_) break;

    // handle added/removed animations
    threadLock_.lock(); {
      // remove animations
      list<Animation*>::iterator it;
      set<Animation*>::iterator jt;
      for(it = removedAnimations_.begin(); it!=removedAnimations_.end(); ++it)
      {
        for(jt = animations_.begin(); jt!=animations_.end(); ++jt)
        {
          if(*it == *jt) {
            animations_.erase(jt);
            break;
          }
        }
      }
      removedAnimations__.insert(removedAnimations__.end(),
          removedAnimations_.begin(), removedAnimations_.end());
      removedAnimations_.clear();

      // and add animations
      for(it = newAnimations_.begin(); it!=newAnimations_.end(); ++it)
      {
        animations_.insert(*it);
      }
      newAnimations_.clear();
    } threadLock_.unlock();

    if(pauseFlag_ || animations_.size()==0) {
#ifndef SYNCHRONIZE_THREADS
      usleepRegen(IDLE_SLEEP);
#endif // SYNCHRONIZE_THREADS
    } else {
      animationInProgress_ = GL_TRUE;
      GLdouble dt = ((GLdouble)(time_ - lastTime_).total_microseconds())/1000.0;
      for(set<Animation*>::iterator it=animations_.begin(); it!=animations_.end(); ++it)
      {
        Animation *anim = *it;
        if(anim->isRunning()) anim->animate(dt);
      }
#ifndef SYNCHRONIZE_THREADS
      if(dt<10) usleepRegen((10-dt) * 1000);
#endif // SYNCHRONIZE_THREADS
      animationInProgress_ = GL_FALSE;
    }
    lastTime_ = time_;

#ifdef SYNCHRONIZE_THREADS
    nextStep();
    waitForFrame();
#endif // SYNCHRONIZE_THREADS
  }
}

void AnimationManager::pause(GLboolean blocking)
{
  pauseFlag_ = GL_TRUE;
  if(blocking) {
    while(animationInProgress_) usleepRegen(1000);
  }
}
void AnimationManager::resume()
{
  pauseFlag_ = GL_FALSE;
}
