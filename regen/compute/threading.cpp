#include "threading.h"
using namespace regen;

static uint32_t getMaxNumJobs() {
	// Note: -2 is for graphics and animation thread.
	// Note: But we may run other dedicated threads.
	const uint32_t numRemainingCores =
			std::max(1u, std::thread::hardware_concurrency() - 2);
	// if we have a lot of cores, be a bit more conservative
	if (numRemainingCores > 8u) {
		return numRemainingCores - 2u;
	} else if (numRemainingCores > 4u) {
		return numRemainingCores - 1u;
	} else {
		return numRemainingCores;
	}
}

JobPool& regen::threading::getJobPool() {
	static JobPool jobPool(getMaxNumJobs());
	return jobPool;
}

