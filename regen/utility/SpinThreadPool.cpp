#include "SpinThreadPool.h"
#include <regen/utility/logging.h>

using namespace regen;

SpinThreadPool::SpinThreadPool(unsigned int numThreads) :
		workers(numThreads),
		shutdown(false) {
	REGEN_INFO("num threads: " << numThreads);
	for (unsigned int i = 0; i < numThreads; ++i) {
		workers[i].thread = std::thread([this, i] {
			while (!shutdown.load()) {
				if (workers[i].has_work.load(std::memory_order_acquire)) {
					workers[i].job();  // Run job
					workers[i].has_work.store(false, std::memory_order_release);
				} else {
					std::this_thread::yield(); // spin-yield loop
				}
			}
		});
	}
}

void SpinThreadPool::assignJob(int i, std::function<void()> f) {
	workers[i].job = f;
	workers[i].has_work.store(true, std::memory_order_release);
}

void SpinThreadPool::waitAll() {
	for (auto &w: workers) {
		while (w.has_work.load(std::memory_order_acquire)) {
			std::this_thread::yield();
		}
	}
}

void SpinThreadPool::shutdownPool() {
	shutdown.store(true);
	for (auto &w: workers) {
		if (w.thread.joinable())
			w.thread.join();
	}
}
