/*
 * animation-manager.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <map>
#include <limits.h>

#include "animation-manager.h"

/**
 * Milliseconds to sleep per loop in idle mode.
 */
#define IDLE_SLEEP_MS 5

AnimationManager& AnimationManager::get()
{
  static AnimationManager manager;
  return manager;
}

AnimationManager::AnimationManager()
: closeFlag_(false),
  pauseFlag_(false),
  hasNextFrame_(false)
{
  time_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastTime_ = time_;

  animationThread_ = boost::thread(&AnimationManager::run, this);
}

AnimationManager::~AnimationManager()
{
  animationLock_.lock(); {
    closeFlag_ = true;
  } animationLock_.unlock();
  nextFrame();
  animationThread_.join();

  for(AnimationBuffers::iterator it = animationBuffers_.begin();
      it != animationBuffers_.end(); ++it)
  {
    delete it->second;
  }
  animationBuffers_.clear();
}

void AnimationManager::addAnimation(ref_ptr<Animation> animation, GLenum bufferAccess)
{
  // queue adding the animation in the animation thread
  animationLock_.lock(); { // lock shared newAnimations_
    newAnimations_.push_back(animation);
  } animationLock_.unlock();

  VBOAnimation *vboAnimation = dynamic_cast<VBOAnimation*>(animation.get());
  if(vboAnimation != NULL) {
    GLuint vboID = vboAnimation->destinationBuffer();
    AnimationBuffers::iterator it = animationBuffers_.find(vboID);

    // add to animation buffer,
    // animation buffer is handled in mainthread
    if(it == animationBuffers_.end()) {
      AnimationBuffer *abuff = new AnimationBuffer(bufferAccess);

      AnimationBuffers::iterator jt = animationBuffers_.insert(
              pair<GLuint,AnimationBuffer*>(vboID,abuff)).first;
      AnimationIterator kt = abuff->add(vboAnimation, vboID);
      animationToBuffer_[animation.get()] = kt;
    } else {
      AnimationIterator jt = it->second->add(vboAnimation, vboID);
      animationToBuffer_[animation.get()] = jt;
    }
  }
}
void AnimationManager::removeAnimation(ref_ptr<Animation> animation)
{
  VBOAnimation *vboAnimation = dynamic_cast<VBOAnimation*>(animation.get());
  if(vboAnimation != NULL) {
    GLuint vboID = vboAnimation->destinationBuffer();
    AnimationBuffers::iterator it = animationBuffers_.find(vboID);
    map< Animation*, AnimationIterator >::iterator jt = animationToBuffer_.find(animation.get());

    if(it != animationBuffers_.end() && jt != animationToBuffer_.end()) {
      it->second->remove(jt->second, vboID);
      if(it->second->isEmpty()) {
        delete it->second;
        animationBuffers_.erase(it);
      }
    }
  }

  animationLock_.lock(); {
    removedAnimations_.push_back(animation);
  } animationLock_.unlock();
}

void AnimationManager::updateGraphics(GLdouble dt)
{
  animationLock_.lock();
  for(AnimationBuffers::iterator
      it = animationBuffers_.begin(); it != animationBuffers_.end(); ++it)
  {
    // copy animation buffer content to drawing buffer.
    if(it->second->bufferChanged()) {
      it->second->lock(); { // avoid animations while updating
        // unmap the animation buffer data
        // because we want GL to copy the data to the primitive data vbo
        it->second->unmap();
        it->second->copy(it->first);
        // map the data again, animations can continue
        it->second->map();
      } it->second->unlock();
    }
  }
  animationLock_.unlock();

  for(list< ref_ptr<Animation> >::iterator it = animations_.begin();
      it != animations_.end(); ++it)
  {
    it->get()->updateGraphics(dt);
  }
}

void AnimationManager::nextFrame()
{
  // set the next frame condition to true
  // and notify waitForFrame if it is waiting.
  // waitForStep waits only if it was faster to render
  // a new frame then calculating the next animation step
  {
    boost::lock_guard<boost::mutex> lock(frameMut_);
    hasNextFrame_ = true;
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
    hasNextStep_ = true;
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
    hasNextFrame_ = false;
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
    hasNextStep_ = false;
  }
}

void AnimationManager::run()
{
  while(true) {
    time_ = boost::posix_time::ptime(
        boost::posix_time::microsec_clock::local_time());

    // break loop and close thread if requested.
    if(closeFlag_) break;

    // handle added/removed animations
    animationLock_.lock(); {
      // remove animations
      list< ref_ptr<Animation> >::iterator it, jt;
      for(it = removedAnimations_.begin(); it!=removedAnimations_.end(); ++it)
      {
        for(jt = animations_.begin(); jt!=animations_.end(); ++jt)
        {
          if(it->get() == jt->get()) {
            animations_.erase(jt);
            break;
          }
        }
      }
      removedAnimations_.clear();

      // and add animations
      for(it = newAnimations_.begin(); it!=newAnimations_.end(); ++it)
      {
        animations_.push_back(*it);
      }
      newAnimations_.clear();
    } animationLock_.unlock();

    if(pauseFlag_ || animations_.size()==0) {
      boost::this_thread::sleep(boost::posix_time::milliseconds( IDLE_SLEEP_MS ));
    } else {
      GLdouble milliSeconds = ((GLdouble)(time_ - lastTime_).total_microseconds())/1000.0;
      for(list< ref_ptr<Animation> >::iterator it = animations_.begin();
          it != animations_.end(); ++it)
      {
        it->get()->animate(milliSeconds);
      }
    }
    lastTime_ = time_;

    nextStep();
    waitForFrame();
  }
}

void AnimationManager::pause()
{
  pauseFlag_ = true;
}
void AnimationManager::resume()
{
  pauseFlag_ = false;
}
