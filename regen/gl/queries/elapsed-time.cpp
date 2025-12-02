#include "elapsed-time.h"

using namespace regen;

ElapsedTimeDebugger::ElapsedTimeDebugger(std::string_view sessionName, uint32_t numFrames, Mode mode)
		: sessionName_(sessionName),
		  numFrames_(numFrames),
		  mode_(mode) {
	cpuLastTime_ = std::chrono::high_resolution_clock::now();
	if (mode_ == CPU_AND_GPU) {
		query_ = ref_ptr<TimeElapsedQuery>::alloc();
	}
}

ElapsedTimeDebugger::~ElapsedTimeDebugger() {
	pushTime("Finalize");
	printResults();
}

void ElapsedTimeDebugger::push(std::string_view name) {
	pushTime(name);
	cpuLastTime_ = std::chrono::high_resolution_clock::now();
	if (mode_ == CPU_AND_GPU) {
		query_->begin();
	}
}

void ElapsedTimeDebugger::pushTime(std::string_view name) {
	if (gpuTimes_.size() <= pushIdx_) {
		gpuTimes_.resize(pushIdx_ + 1);
		cpuTimes_.resize(pushIdx_ + 1);
		pushNames_.resize(pushIdx_ + 1);
		pushCounts_.resize(pushIdx_ + 1);
		pushNames_[pushIdx_] = name;
		pushCounts_[pushIdx_] = 0u;
		cpuTimes_[pushIdx_] = 0.0f;
		gpuTimes_[pushIdx_] = 0.0f;
	}
	if (mode_ == CPU_AND_GPU) {
		gpuTimes_[pushIdx_] += query_->end();
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> ms = t2 - cpuLastTime_;
	cpuTimes_[pushIdx_] += ms.count();
	pushCounts_[pushIdx_] += 1u;
	cpuLastTime_ = t2;
	pushIdx_ += 1u;
}

void ElapsedTimeDebugger::beginFrame() {
	if (pushIdx_ != 0u) {
		frameIdx_ += 1u;
		if (frameIdx_ >= numFrames_) {
			printResults();
			// reset the frame index if we reached the number of frames
			frameIdx_ = 0u;
			for (uint32_t i = 0; i < cpuTimes_.size(); ++i) {
				cpuTimes_[i] = 0.0f;
				gpuTimes_[i] = 0.0f;
				pushCounts_[i] = 0u;
			}
		}
		pushIdx_ = 0u;
	}
	if (mode_ == CPU_AND_GPU) {
		query_->begin();
	}
	cpuLastTime_ = std::chrono::high_resolution_clock::now();
}

void ElapsedTimeDebugger::endFrame() {
	if (mode_ == CPU_AND_GPU) {
		query_->end();
	}
}

void ElapsedTimeDebugger::printResults() {
	float cpuSum = 0.0f;
	float gpuSum = 0.0f;
	for (uint32_t i = 0; i < pushIdx_; ++i) {
		// compute the average elapsed time
		cpuTimes_[i] = cpuTimes_[i] / static_cast<float>(pushCounts_[i]);
		gpuTimes_[i] = gpuTimes_[i] / static_cast<float>(pushCounts_[i]);
		gpuSum += gpuTimes_[i];
		cpuSum += cpuTimes_[i];
	}

	if (mode_ == CPU_AND_GPU) {
		REGEN_INFO("Elapsed time in " << sessionName_ << ": "
			<< std::fixed << std::setprecision(5)
			<< cpuSum << " ms (CPU), "
			<< gpuSum << " ms (GPU)");
		for (uint32_t i = 0; i < pushIdx_; ++i) {
			REGEN_INFO("\t" << std::fixed << std::setprecision(5)
					<< cpuTimes_[i] << " ms (CPU) + "
					<< gpuTimes_[i] << " ms (GPU) "
					<< " in " << pushNames_[i]);
		}
	} else {
		REGEN_INFO("Elapsed time in " << sessionName_ << ": "
			<< std::fixed << std::setprecision(5)
			<< cpuSum << " ms (CPU)");
		for (uint32_t i = 0; i < pushIdx_; ++i) {
			REGEN_INFO("\t" << std::fixed << std::setprecision(5)
					<< cpuTimes_[i] << " ms (CPU) "
					<< " in " << pushNames_[i]);
		}
	}
}
