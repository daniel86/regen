/*
 * threading.h
 *
 *  Created on: 23.04.2013
 *      Author: daniel
 */

#ifndef __THREADING_H_
#define __THREADING_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace regen {
	// i have a strange problem with boost::this_thread here.
	// it just adds 100ms to the interval provided :/
#ifdef UNIX
#define usleepRegen(v) usleep(v)
#else
#define usleepRegen(v) boost::this_thread::sleep(boost::posix_time::microseconds(v))
#endif

	/**
	 * \brief Simple thread class using boost::thread.
	 */
	class Thread {
	public:
		Thread();

		virtual ~Thread() = default;

	protected:
		boost::thread thread_;
		boost::mutex threadLock_;

		virtual void run() = 0;
	};
} // namespace

#endif /* __THREADING_H_ */
