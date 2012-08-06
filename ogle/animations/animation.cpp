/*
 * animation.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <list>

#include "animation.h"

Animation::Animation()
{
}

void Animation::set_elapsedTime(const double &time)
{
  elapsedTime_ = time;
}
const double& Animation::elapsedTime() const
{
  return elapsedTime_;
}

bool Animation::try_lock()
{
  return mutex_.try_lock();
}
void Animation::lock()
{
  mutex_.lock();
}
void Animation::unlock()
{
  mutex_.unlock();
}

void Animation::animate(
    const double &milliSeconds)
{
  while(!try_lock()); { // make sure data stays mapped
    doAnimate(milliSeconds);
    elapsedTime_ += milliSeconds;
  } unlock();
}
