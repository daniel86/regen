/*
 * threading.h
 *
 *  Created on: 23.04.2013
 *      Author: daniel
 */

#ifndef THREADING_H_
#define THREADING_H_

#include <regen/config.h>

// i have a strange problem with boost::this_thread here.
// it just adds 100ms to the interval provided :/
#ifdef UNIX
#define usleepRegen(v) usleep(v)
#else
#define usleepRegen(v) boost::this_thread::sleep(boost::posix_time::microseconds(v))
#endif

#endif /* THREADING_H_ */
