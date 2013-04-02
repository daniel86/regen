/*
 * animation.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <regen/animations/animation-manager.h>
#include <regen/config.h>

#include "animation.h"
using namespace regen;

GLuint Animation::ANIMATION_STARTED = EventObject::registerEvent("animationStarted");
GLuint Animation::ANIMATION_STOPPED = EventObject::registerEvent("animationStopped");

Animation::Animation(GLboolean useGLAnimation, GLboolean useAnimation)
: EventObject(),
  useGLAnimation_(useGLAnimation),
  useAnimation_(useAnimation),
  isRunning_(GL_FALSE)
{
  startAnimation();
}
Animation::~Animation()
{
  stopAnimation();
}

void Animation::startAnimation()
{
  if(isRunning_) return;
  isRunning_ = GL_TRUE;

  queueEmit(ANIMATION_STARTED);
  AnimationManager::get().addAnimation(this);
}
void Animation::stopAnimation()
{
  if(!isRunning_) return;
  isRunning_ = GL_FALSE;

  queueEmit(ANIMATION_STOPPED);
  AnimationManager::get().removeAnimation(this);
}

GLboolean Animation::isRunning() const
{ return isRunning_; }

GLboolean Animation::try_lock()
{ return mutex_.try_lock(); }
void Animation::lock()
{ mutex_.lock(); }
void Animation::unlock()
{ mutex_.unlock(); }

void Animation::wait(GLuint milliseconds)
{
#ifdef UNIX
      usleep(1000*milliseconds);
#else
      boost::this_thread::sleep(
          boost::posix_time::milliseconds(milliseconds));
#endif
}

GLboolean Animation::useGLAnimation() const
{ return useGLAnimation_; }
GLboolean Animation::useAnimation() const
{ return useAnimation_; }
