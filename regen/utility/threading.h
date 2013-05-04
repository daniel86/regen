/*
 * threading.h
 *
 *  Created on: 23.04.2013
 *      Author: daniel
 */

#ifndef __THREADING_H_
#define __THREADING_H_

#include <regen/config.h>

namespace regen {
  // i have a strange problem with boost::this_thread here.
  // it just adds 100ms to the interval provided :/
  #ifdef UNIX
  #define usleepRegen(v) usleep(v)
  #else
  #define usleepRegen(v) boost::this_thread::sleep(boost::posix_time::microseconds(v))
  #endif
} // namespace

#endif /* __THREADING_H_ */
