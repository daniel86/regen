/*
 * threading.cpp
 *
 *  Created on: 04.05.2013
 *      Author: daniel
 */

#include <regen/config.h>

#include "threading.h"

using namespace regen;

Thread::Thread()
		: thread_(&Thread::run, this) {
}
