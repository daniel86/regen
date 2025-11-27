#ifndef REGEN_THREADING_H_
#define REGEN_THREADING_H_

#include <boost/thread/thread.hpp>
#include <thread>
#include <atomic>
#include <vector>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #include <immintrin.h>
    #define CPU_PAUSE() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
    #define CPU_PAUSE() asm volatile("yield" ::: "memory")
#else
    #define CPU_PAUSE() ((void)0)
#endif

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
				CPU_PAUSE();
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
				if (++spins < 200) {
					CPU_PAUSE();
				} else {
					std::this_thread::yield();
				}
			}
		}
		void unlock() {
			flag.clear(std::memory_order_release);
		}
	};

	/**
	 * \brief A job structure representing a unit of work.
	 */
	struct Job {
		void (*fn)(void*) = nullptr;
		void *arg = nullptr;
	};

	/**
	 * \brief A lock-free job queue using a circular buffer.
	 */
	template <class JobType>
	class JobQueue {
		std::vector<JobType> buffer;
		// Producer position, padded to avoid false sharing
		/** alignas(64) **/ std::atomic<uint32_t> head{0};
		// Consumer position, padded to avoid false sharing
		/** alignas(64) **/ std::atomic<uint32_t> tail{0};
	public:
		JobQueue() : buffer(1024) {
			assert((buffer.size() & mask()) == 0 && "JobQueue size must be power of two");
		}

		inline uint32_t mask() const {
			return static_cast<uint32_t>(buffer.size() - 1);
		}

		/**
		 * Grow the job queue to double its current size.
		 * This is not thread-safe and should only be called by the owner thread
		 * when no frame is active.
		 */
		void grow() {
			const size_t oldSize = buffer.size();
			const size_t newSize = oldSize * 2;
			const uint32_t t = tail.load(std::memory_order_acquire);
			const uint32_t h = head.load(std::memory_order_acquire);
			const uint32_t count = (h - t) & (oldSize - 1);

			std::vector<JobType> newBuffer(newSize);
			for (uint32_t i = 0; i < count; ++i) {
				newBuffer[i] = buffer[(t + i) & (oldSize - 1)];
			}
			head.store(count, std::memory_order_release);
			tail.store(0, std::memory_order_release);
			buffer = std::move(newBuffer);
		}

		/**
		 * Producer non-blocking
		 * @param j The job to push.
		 * @return True if the job was pushed, false if the queue is full.
		 */
		bool push(const JobType &j) {
			const uint32_t mask = this->mask();
			while (true) {
				// Load current head and tail
				uint32_t h = head.load(std::memory_order_relaxed);
				const uint32_t t = tail.load(std::memory_order_acquire);
				if ((h - t) == mask) return false; // full
				// try to claim slot h by incrementing head -> h+1
				// on success we "own" index h and can safely write to buffer[h & mask]
				if (head.compare_exchange_weak(h, h+1,
							std::memory_order_acq_rel,
							std::memory_order_relaxed)) {
					buffer[h & mask] = j;
					return true;
				}
			}
		}

		/**
		 * Consumer non-blocking
		 * @param out The job to pop.
		 * @return True if a job was popped, false if the queue is empty.
		 */
		bool try_pop(JobType &out) {
			const uint32_t mask = this->mask();
			while (true) {
				// Load current tail and head
				const uint32_t h = head.load(std::memory_order_acquire);
				uint32_t t = tail.load(std::memory_order_relaxed);
				if (t == h) return false; // empty
				// try to claim slot t by incrementing tail -> t+1
				// on success we "own" index t and can safely read buffer[t & mask]
				if (tail.compare_exchange_weak(t, t+1,
							std::memory_order_acq_rel,
							std::memory_order_relaxed)) {
					out = buffer[t & mask];
					return true;
				}
			}
		}

		/**
		 * Checks if there are unassigned jobs in the job queue.
		 * @return True if there are unassigned jobs, false otherwise.
		 */
		bool hasUnassignedJobs() const {
			const uint32_t t = tail.load(std::memory_order_acquire);
			const uint32_t h = head.load(std::memory_order_acquire);
			return t != h;
		}
	};

	/**
	 * \brief A structure representing a frame of jobs.
	 */
	struct JobFrame {
		// number of jobs remaining in the current frame
		std::atomic<uint32_t> numJobsRemaining{0u};
		// number of jobs pushed in the current frame
		uint32_t numPushed = 0u;
	};

	/**
	 * \brief A job associated with a job frame.
	 */
	struct FramedJob : Job {
		JobFrame *frame = nullptr;
	};

	/**
	 * \brief A job pool for managing and executing jobs across multiple threads.
	 */
	class JobPool {
	public:
		/**
		 * \brief Constructor for the job pool.
		 * @param numThreads The number of worker threads in the pool.
		 */
		explicit JobPool(uint32_t numThreads) : numThreads_(numThreads) {
			threads_.reserve(numThreads_);
			for (uint32_t i = 0; i < numThreads_; ++i) {
				threads_.emplace_back(&JobPool::workerLoop, this);
			}
		}

		/**
		 * \brief Destructor for the job pool.
		 * Stops all worker threads and cleans up resources.
		 */
		~JobPool() {
			stopThreads();
		}

		/**
		 * @return The number of worker threads in the pool.
		 */
		uint32_t numThreads() const {
			return numThreads_;
		}

		/**
		 * Stops all worker threads in the pool.
		 */
		void stopThreads() {
			running_.store(false);
			workSem_.release(static_cast<int>(numThreads_));
			for (auto &thread : threads_) {
				if (thread.joinable()) {
					thread.join();
				}
			}
		}

		/**
		 * Adds a job to the job queue before beginFrame() was called
		 * to fill the job queue for each frame.
		 * @param jobFrame The job frame to add the job to.
		 * @param job The job to add.
		 */
		void addJobPreFrame(JobFrame &jobFrame, const Job &job) {
			if (jobQueue_.push({{job.fn, job.arg}, &jobFrame})) {
				jobFrame.numPushed++;
			} else {
				// job queue full, grow and push
				jobQueue_.grow();
				jobQueue_.push({{job.fn, job.arg}, &jobFrame});
				jobFrame.numPushed++;
			}
		}

		/**
		 * Adds a job to the job queue within a frame.
		 * @param jobFrame The job frame to add the job to.
		 * @param job The job to add.
		 */
		void addJobWithinFrame(JobFrame &jobFrame, const Job &job) {
			if (jobQueue_.push({{job.fn, job.arg}, &jobFrame})) {
				jobFrame.numJobsRemaining.fetch_add(1u, std::memory_order_acq_rel);
				workSem_.release(1);
			} else {
				// job queue full, execute directly
				job.fn(job.arg);
			}
		}

		/**
		 * Begins a new frame for job processing.
		 * This should be called by the owner thread after all jobs for the frame have been added.
		 * This is a non-blocking call, endFrame() must be called to ensure all jobs are completed.
		 * @param jobFrame The job frame to begin.
		 * @param numLocalJobs The number of local jobs to be executed by the owner thread.
		 */
		void beginFrame(JobFrame &jobFrame, uint32_t numLocalJobs) {
			jobFrame.numJobsRemaining.store(jobFrame.numPushed+numLocalJobs, std::memory_order_relaxed);
			workSem_.release(jobFrame.numPushed);
		}

		/**
		 * Performs a job from the job queue.
		 * This can be called by worker threads to execute a job.
		 * @param job The job to perform.
		 */
		static void performJob(FramedJob &job) {
			job.fn(job.arg);
			job.frame->numJobsRemaining.fetch_sub(1u, std::memory_order_acq_rel);
		}

		/**
		 * Checks if there are unassigned jobs in the job queue.
		 * @return True if there are unassigned jobs, false otherwise.
		 */
		bool hasUnassignedJobs() const {
			return jobQueue_.hasUnassignedJobs();
		}

		/**
		 * Steals an unassigned job from the job queue.
		 * This can be called by the owner thread or worker threads to get a job to execute.
		 * @param out The job to steal.
		 * @return True if a job was stolen, false otherwise.
		 */
		bool stealJob(FramedJob &out) {
			return jobQueue_.try_pop(out);
		}

		/**
		 * Ends the current frame for job processing.
		 * This should be called by the owner thread to wait for all jobs to complete.
		 * This is a blocking call.
		 */
		static void endFrame(JobFrame &jobFrame) {
#if 0
			int spins = 0;
			while (numJobsRemaining_.load(std::memory_order_acquire) > 0u) {
				if (++spins < 1000) _mm_pause();
				else std::this_thread::yield();
			}
#else
			while (jobFrame.numJobsRemaining.load(std::memory_order_acquire) > 0u) {
				CPU_PAUSE();
			}
#endif
			jobFrame.numPushed = 0u;
		}

		/**
		 * Worker thread loop for processing jobs.
		 * This should be run by each worker thread in the pool.
		 */
		void workerLoop() {
			FramedJob job;
			while (running_.load()) {
				workSem_.acquire();
				if (jobQueue_.try_pop(job)) {
					performJob(job);
				}
			}
		}

	protected:
		// number of worker threads
		const uint32_t numThreads_;
		// worker threads
		std::vector<std::thread> threads_;

		// job queue
		JobQueue<FramedJob> jobQueue_;
		// semaphore to signal available work
		std::counting_semaphore<> workSem_{0};
		// running flag
		std::atomic<bool> running_{true};
	};

	/**
	 * \brief A thread-safe queue implementation.
	 *
	 * This is supposed to be used in producer-consumer scenarios.
	 * Currently only a single producer and a single consumer is supported.
	 * E.g. GUI thread producing tasks, CPU thread consuming tasks.
	 *
	 * Internally, the queue is composed of segments of fixed size (SEG_SIZE),
	 * this is done to allow dynamic growth without requiring complex memory management.
	 * Producers write to the head segment, while consumers read from the tail segment.
	 * When a segment is full, a new segment is allocated and linked to the previous one.
	 * When a segment is fully consumed, it is deleted to free memory.
	 *
	 * At the moment, this implementation is not entirely lock-free, as it uses a spin lock
	 * to manage a pool of pre-allocated segments for reuse. However, often this lock
	 * won't be used as one segment is usually sufficient for most workloads.
	 */
	template <typename T, size_t SEG_SIZE = 512>
	class ThreadSafeQueue {
		/** \brief A segment of the lock-free queue. */
	    struct Segment {
	    	// The cursor for writing items into the segment.
	    	// Only modified by the producer.
	    	// Conceptually the write cursor is "behind" the read cursor,
	    	// and stands still when reaching the read cursor.
	        std::atomic<size_t> writeCursor{0};
	    	// The cursor for reading items from the segment.
	    	// Only modified by the consumer.
	        std::atomic<size_t> readCursor{0};
	    	// Segments are linked in a singly linked list.
	    	// Only modified by the producer.
	        std::atomic<Segment*> next{nullptr};
	    	// The items in the segment.
	        T items[SEG_SIZE];
	    };

	    // Producer writes to head_
	    std::atomic<Segment*> head_;
	    // Consumer reads from tail_
	    std::atomic<Segment*> tail_;

		// A pool of pre-allocated segments
		std::vector<Segment*> segmentPool_;

	public:
		/**
		 * \brief Constructs an empty lock-free queue.
		 */
		ThreadSafeQueue() {
			// Initially, create a single empty segment used for both head and tail.
	        auto* s = acquireSegment();
	        head_.store(s, std::memory_order_relaxed);
	        tail_.store(s, std::memory_order_relaxed);
	    }

		/**
		 * \brief Destructor for the lock-free queue.
		 */
		~ThreadSafeQueue() {
	        // Free all remaining segments, for this we traverse the list structure.
	        Segment* seg = tail_.load(std::memory_order_relaxed);
	        while (seg) {
	            Segment* next = seg->next.load(std::memory_order_relaxed);
	            delete seg;
	            seg = next;
	        }
			// Free segments in the pool
			for (Segment* s : segmentPool_) {
				delete s;
			}
	    }

		/**
		 * \brief Pushes an item onto the queue.
		 * @param item The item to push.
		 */
	    void push(const T& item) {
			// Load the current head segment which we will write to.
	        Segment* seg = head_.load(std::memory_order_acquire);
			// Load the cursor positions for this segment.
	        const size_t w = seg->writeCursor.load(std::memory_order_relaxed);
	        const size_t r = seg->readCursor.load(std::memory_order_acquire);

	        if (w - r < SEG_SIZE) {
	            // there is enough space in current segment.
	            seg->items[w % SEG_SIZE] = item;
	            seg->writeCursor.store(w + 1, std::memory_order_release);
	            return;
	        }

	        // Current segment overflow -> allocate a new one.
			// This means the consumer is at least SEG_SIZE items behind.
			// In this case, we create a new segment and link it to previous segment.
			// NOTE: The consumer can release the old segment when it is done with it.
	        auto* newSeg = acquireSegment();
	        newSeg->items[0] = item;
	        newSeg->writeCursor.store(1, std::memory_order_release);

			// Link new segment to current head segment, and update head_ to point to new segment.
	        seg->next.store(newSeg, std::memory_order_release);
	        head_.store(newSeg, std::memory_order_release);
	    }

		/**
		 * \brief Checks if the queue is empty.
		 * @return True if the queue is empty, false otherwise.
		 */
	    bool empty() const {
	        const Segment* seg = tail_.load(std::memory_order_acquire);
	        const size_t w = seg->writeCursor.load(std::memory_order_acquire);
	        const size_t r = seg->readCursor.load(std::memory_order_relaxed);
			// if cursors are at different positions, queue is not empty
	        if (r != w) return false;
			// also if there is a next segment, queue is not empty
	        return seg->next.load(std::memory_order_acquire) == nullptr;
	    }

		/**
		 * \brief Returns a reference to the front item in the queue.
		 * This is only safe to call if the queue is not empty.
		 * @return A reference to the front item.
		 */
		T& front() {
	        Segment* seg = tail_.load(std::memory_order_acquire);
	        const size_t r = seg->readCursor.load(std::memory_order_relaxed);
	        const size_t w = seg->writeCursor.load(std::memory_order_acquire);

	        if (r == w) {
				// Advance to next segment if we hit the write cursor.
	            Segment* next = seg->next.load(std::memory_order_acquire);
	            const size_t r_next = next->readCursor.load(std::memory_order_relaxed);
	            tail_.store(next, std::memory_order_release);
	            // Old segment can now be released
	        	releaseSegment(seg);
				return next->items[r_next % SEG_SIZE];
	        } else {
				return seg->items[r % SEG_SIZE];
	        }
	    }

		/**
		 * \brief Pops the front item from the queue.
		 * This is only safe to call if the queue is not empty.
		 */
		void pop() {
	        Segment* seg = tail_.load(std::memory_order_acquire);
	        const size_t r = seg->readCursor.load(std::memory_order_relaxed) + 1;

			// Advance read cursor
	        seg->readCursor.store(r, std::memory_order_release);

	        // If segment fully consumed and next exists -> move to next
	        size_t w = seg->writeCursor.load(std::memory_order_acquire);
	        if (r == w) {
	            Segment* next = seg->next.load(std::memory_order_acquire);
	            if (next != nullptr) {
	                tail_.store(next, std::memory_order_release);
	            	// Old segment can now be released
	            	releaseSegment(seg);
	            }
	        }
	    }

	private:
		AdaptiveLock poolLock_;

		Segment* acquireSegment() {
			poolLock_.lock();
			if (!segmentPool_.empty()) {
				Segment* seg = segmentPool_.back();
				segmentPool_.pop_back();
				poolLock_.unlock();

				seg->writeCursor.store(0, std::memory_order_relaxed);
				seg->readCursor.store(0, std::memory_order_relaxed);
				seg->next.store(nullptr, std::memory_order_relaxed);

				return seg;
			} else {
				poolLock_.unlock();
				return new Segment();
			}
		}

		void releaseSegment(Segment* seg) {
			poolLock_.lock();
			segmentPool_.push_back(seg);
			poolLock_.unlock();
		}
	};

	namespace threading {
		/**
		 * @return the job pool.
		 */
		JobPool& getJobPool();
	}

	// i have a strange problem with boost::this_thread here.
	// it just adds 100ms to the interval provided :/
#ifdef UNIX
#define usleepRegen(v) usleep(v)
#else
#define usleepRegen(v) boost::this_thread::sleep(boost::posix_time::microseconds(v))
#endif
} // namespace

#endif /* REGEN_THREADING_H_ */
