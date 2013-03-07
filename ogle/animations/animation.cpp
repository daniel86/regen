/*
 * animation.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include "animation.h"
using namespace ogle;

GLboolean Animation::try_lock()
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
