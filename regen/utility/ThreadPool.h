/*
 * Copyright (c) 2022, Daniel Beßler
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_THREAD_POOL_H_
#define KNOWROB_THREAD_POOL_H_

#include <queue>
#include <mutex>
#include <string>
#include <list>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <thread>

namespace regen {
	/**
	 * A pool of worker threads waiting on tasks to be pushed
	 * into a work queue.
	 */
	class ThreadPool {
	public:
		// forward declarations
		class Runner;

		using ExceptionHandler = std::function<void(const std::exception &)>;

		/**
		 * @param maxNumThreads the maximum number of worker threads
		 */
		explicit ThreadPool(uint32_t maxNumThreads);

		virtual ~ThreadPool();

		/**
		 * Cannot be copy-assigned.
		 */
		ThreadPool(const ThreadPool &) = delete;

		/**
		 * @return the maximum number of worker threads
		 */
		uint32_t maxNumThreads() const { return maxNumThreads_; }

		/**
		 * Shutdown the thread pool.
		 * Joining all the worker threads. If one of them is busy, this
		 * call will block until the worker has finished its work, and can
		 * gracefully exit.
		 */
		void shutdown();

		/**
		 * Pushes a goal for a worker.
		 * The goal is assigned to a worker thread when one is available.
		 * @param goal the work goal
		 * @param exceptionHandler an exception handler
		 */
		void pushWork(const std::shared_ptr<ThreadPool::Runner> &goal, ThreadPool::ExceptionHandler exceptionHandler);

		/**
		 * @return the number of work goals that are currently queued.
		 */
		int numQueuedWork() const { return workQueue_.size(); }

		/**
		 * @return the number of worker threads that are currently active.
		 */
		int numActiveWorker() const { return numActiveWorker_; }

		/**
		 * A worker thread that pulls work goals from the work queue of a thread pool.
		 */
		class Worker {
		public:
			explicit Worker(ThreadPool *thread_pool);

			~Worker();

			/**
			 * Cannot be copy-assigned.
			 */
			Worker(const Worker &) = delete;

		protected:
			ThreadPool *threadPool_;

			std::atomic<bool> isTerminated_;
			std::atomic<bool> hasTerminateRequest_;

			std::thread thread_;

			void run();

			friend class ThreadPool;
		};

		/**
		 * An object that provides a run function which is evaluated
		 * in a worker thread.
		 */
		class Runner {
		public:
			Runner();

			virtual ~Runner();

			/**
			 * Cannot be copy-assigned.
			 */
			Runner(const Runner &) = delete;

			/**
			 * Wait until run function has exited.
			 */
			void join();

			/**
			 * Run the computation in a worker thread.
			 */
			virtual void run() = 0;

			/**
			 * Stop the runner.
			 * @param wait call blocks until runner exited if true.
			 */
			void stop(bool wait);

			/**
			 * @return true if the runner was requested to stop.
			 */
			[[nodiscard]] bool hasStopRequest() const { return hasStopRequest_; }

			/**
			 * @return true if the runner is still active.
			 */
			[[nodiscard]] bool isTerminated() const { return isTerminated_; }

			/**
			 * @return true if the runner is currently running.
			 */
			[[nodiscard]] bool isRunning() const { return isRunning_; }

		protected:
			std::atomic<bool> isTerminated_;
			std::atomic<bool> isRunning_;
			std::atomic<bool> hasStopRequest_;
			std::mutex mutex_;
			std::condition_variable finishedCV_;
			ExceptionHandler exceptionHandler_;

			void runInternal();

			void setExceptionHandler(ExceptionHandler exceptionHandler) { exceptionHandler_ = exceptionHandler; }

			friend class ThreadPool::Worker;

			friend class ThreadPool;
		};

		/**
		 * A runner that executes a lambda function.
		 */
		class LambdaRunner : public Runner {
		public:
			using StopChecker = std::function<bool()>;

			/**
			 * @param fn the lambda function to be executed
			 */
			explicit LambdaRunner(const std::function<void(const StopChecker&)> &fn) : fn_(fn) {}

			void run() override { fn_([&]{ return hasStopRequest(); }); }

		protected:
			std::function<void(const StopChecker&)> fn_;
		};

	protected:
		// is called to finalize each worker thread
		// Note: a virtual method is avoided as these cannot be called in destructors.
		std::function<void()> finalizeWorker_;

	private:
		// list of threads doing work
		std::list<Worker *> workerThreads_;
		// currently queued work that has not been associated to a worker yet
		std::queue<std::shared_ptr<ThreadPool::Runner>> workQueue_;
		// condition variable used to wake up worker after new work was queued
		std::condition_variable workCV_;
		mutable std::mutex workMutex_;
		// limit to this number of worker threads
		uint32_t maxNumThreads_;
		// number of terminated threads that are still in workerThreads_ list
		std::atomic_uint32_t numFinishedThreads_;
		// number of currently active workers
		std::atomic_uint32_t numActiveWorker_;

		// get work from queue
		std::shared_ptr<ThreadPool::Runner> popWork();

		// is called initially in each worker thread
		virtual bool initializeWorker() { return true; }
	};

	/**
	 * @return a default thread pool.
	 */
	std::shared_ptr<ThreadPool> DefaultThreadPool();
}

#endif //KNOWROB_THREAD_POOL_H_
