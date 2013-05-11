/*
 * threading.cpp
 *
 *  Created on: 04.05.2013
 *      Author: daniel
 */

#include "threading.h"
using namespace regen;

Thread::Thread()
: thread_(&Thread::__run, this)
{
}

void Thread::__run()
{
  run();
}

void Thread::usleep(unsigned int microSeconds)
{
  // i have a strange problem with boost::this_thread here.
  // it just adds 100ms to the interval provided :/
#ifdef UNIX
  usleep(microSeconds);
#else
  boost::this_thread::sleep(boost::posix_time::microseconds(microSeconds))
#endif
}
