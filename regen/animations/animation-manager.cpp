/*
 * animation-manager.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <map>
#include <limits.h>

#include <regen/config.h>

#include "animation-manager.h"
using namespace regen;

/**
 * Milliseconds to sleep per loop in idle mode.
 */
#define IDLE_SLEEP_MS 100

// boost adds 100ms to desired interval !?!
//  * with version 1.50.0-2
//  * not known as of 14.08.2012
#define BOOST_SLEEP_BUG

AnimationManager& AnimationManager::get()
{
  static AnimationManager manager;
  return manager;
}

AnimationManager::AnimationManager()
: closeFlag_(GL_FALSE),
  pauseFlag_(GL_TRUE),
  hasNextFrame_(GL_FALSE),
  hasNextStep_(GL_FALSE),
  animationThread_(&AnimationManager::run, this)
{
  time_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastTime_ = time_;
}

AnimationManager::~AnimationManager()
{
  animationLock_.lock(); {
    closeFlag_ = GL_TRUE;
  } animationLock_.unlock();
  nextFrame();
  animationThread_.join();
}

void AnimationManager::addAnimation(Animation *animation)
{
  // queue adding the animation in the animation thread
  animationLock_.lock(); { // lock shared newAnimations_
    if(animation->useAnimation()) {
      newAnimations_.push_back(animation);
    }
    if(animation->useGLAnimation()) {
      removedGLAnimations_.erase(animation);
      glAnimations_.insert(animation);
    }
  } animationLock_.unlock();
}
void AnimationManager::removeAnimation(Animation *animation)
{
  animationLock_.lock(); {
    if(animation->useAnimation()) {
      removedAnimations_.push_back(animation);
    }
    if(animation->useGLAnimation()) {
      removedGLAnimations_.insert(animation);
    }
  } animationLock_.unlock();
}

void AnimationManager::updateGraphics(RenderState *rs, GLdouble dt)
{
#ifdef SYNCHRONIZE_ANIM_AND_RENDER
    nextFrame();
#endif

  // remove animations
  set<Animation*>::iterator it, jt;
  for(it = removedGLAnimations_.begin(); it!=removedGLAnimations_.end(); ++it)
  {
    glAnimations_.erase(*it);
  }
  removedGLAnimations_.clear();
  // update animations
  for(jt=glAnimations_.begin(); jt!=glAnimations_.end(); ++jt)
  {
    (*jt)->glAnimate(rs,dt);
  }

#ifdef SYNCHRONIZE_ANIM_AND_RENDER
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
  // just continue if we already have a new frame
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
  // next wait until a new frame is rendered.
  // just continue if we already have a new frame
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
    animationLock_.lock(); {
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
    } animationLock_.unlock();

    if(pauseFlag_ || animations_.size()==0) {
#ifdef UNIX
      // i have a strange problem with boost::this_thread here.
      // it just adds 100ms to the interval provided :/
      usleep(IDLE_SLEEP_MS * 1000);
#else
      boost::this_thread::sleep(boost::posix_time::milliseconds(IDLE_SLEEP_MS));
#endif
    } else {
      GLdouble milliSeconds = ((GLdouble)(time_ - lastTime_).total_microseconds())/1000.0;
      for(set<Animation*>::iterator it=animations_.begin(); it!=animations_.end(); ++it)
      {
        (*it)->animate(milliSeconds);
      }
      if(milliSeconds<10)
#ifdef UNIX
        usleep((10-milliSeconds) * 1000);
#else
        boost::this_thread::sleep(boost::posix_time::milliseconds(10-milliSeconds));
#endif
    }
    lastTime_ = time_;

#ifdef SYNCHRONIZE_ANIM_AND_RENDER
    nextStep();
    waitForFrame();
#endif
  }
}

void AnimationManager::pause()
{
  pauseFlag_ = GL_TRUE;
}
void AnimationManager::resume()
{
  pauseFlag_ = GL_FALSE;
}
