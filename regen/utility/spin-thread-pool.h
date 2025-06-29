#ifndef REGEN_SPIN_THREAD_POOL_H
#define REGEN_SPIN_THREAD_POOL_H

#include <thread>
#include <atomic>
#include <vector>
#include <functional>

namespace regen {
	/**
	 * \brief A simple thread pool that uses spin-waiting.
	 * This is a simple thread pool implementation that uses spin-waiting
	 * for task assignment.
	 */
	class SpinThreadPool {
	public:
		/**
		 * \brief A worker thread in the thread pool.
		 */
		struct Worker {
			std::thread thread;
			std::atomic<bool> has_work{false};
			std::function<void()> job;
		};

		/**
		 * \brief Constructor for the thread pool.
		 * @param numThreads the number of threads in the pool.
		 */
		explicit SpinThreadPool(unsigned int numThreads);

		unsigned int numThreads() const { return workers.size(); }

		/**
		 * Assign a job to a worker thread.
		 * @param i the index of the worker thread.
		 * @param f the job to be executed.
		 */
		void assignJob(int i, std::function<void()> f);

		/**
		 * Wait for all worker threads to finish their jobs.
		 */
		void waitAll();

		/**
		 * Shutdown the thread pool.
		 */
		void shutdownPool();

	private:
		std::vector<Worker> workers;
		std::atomic<bool> shutdown;
	};

} // regen

#endif //REGEN_SPIN_THREAD_POOL_H
