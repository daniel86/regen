#ifndef REGEN_THREADING_H_
#define REGEN_THREADING_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <thread>

namespace regen {
	/**
	 * \brief A simple spin lock implementation.
	 * This is a low-level lock that can be used for short critical sections.
	 * It is not suitable for long operations as it can lead to high CPU usage.
	 */
	class SpinLock {
		std::atomic_flag flag = ATOMIC_FLAG_INIT;
	public:
		void lock() {
			while (flag.test_and_set(std::memory_order_acquire)) {
				// Optional: pause to reduce contention
				// std::this_thread::yield(); or _mm_pause();
			}
		}

		void unlock() {
			flag.clear(std::memory_order_release);
		}
	};

	/**
	 * \brief An adaptive lock that spins for a while before blocking.
	 * This is useful for situations where contention is expected to be low.
	 */
	class AdaptiveLock {
		std::atomic_flag flag = ATOMIC_FLAG_INIT;
	public:
		void lock() {
			int spins = 0;
			while (flag.test_and_set(std::memory_order_acquire)) {
				if (++spins > 1000)
					std::this_thread::sleep_for(std::chrono::nanoseconds(50));
			}
		}
		void unlock() {
			flag.clear(std::memory_order_release);
		}
	};

	// i have a strange problem with boost::this_thread here.
	// it just adds 100ms to the interval provided :/
#ifdef UNIX
#define usleepRegen(v) usleep(v)
#else
#define usleepRegen(v) boost::this_thread::sleep(boost::posix_time::microseconds(v))
#endif
} // namespace

#endif /* REGEN_THREADING_H_ */
