#include <map>
#include <time.h>
#include <boost/thread.hpp>

#include <regen/utility/threading.h>
#include <regen/utility/logging.h>
#include "animation-manager.h"

using namespace regen;

AnimationManager &AnimationManager::get() {
	static AnimationManager manager;
	return manager;
}

static void setStagingCopyFlag() {
	// after each frame, we set a flag that indicates a client buffer to staging copy is in progress.
	// this avoids that client buffers are swapped before staging system had a chance
	// to enter the copy.
	// StagingSystem system sets this flag to false each frame after the copy is done.
	StagingSystem::instance().setIsCopyInProgress();
}

template <bool DesiredFlagState>
static void waitOnFlag(const std::atomic_flag &flag) {
	for (int i = 0; flag.test(std::memory_order_acquire) != DesiredFlagState; ++i) {
		if (i < 16) { CPU_PAUSE(); }
		else if (i < 256) { std::this_thread::yield(); }
		else { std::this_thread::sleep_for(std::chrono::microseconds(1)); }
	}
}

template <typename CounterType, CounterType DesiredCount>
static void waitOnCounter(const std::atomic<CounterType> &count) {
	for (int i = 0; count.load(std::memory_order_acquire) != DesiredCount; ++i) {
		if (i < 16) { CPU_PAUSE(); }
		else if (i < 256) { std::this_thread::yield(); }
		else { std::this_thread::sleep_for(std::chrono::microseconds(1)); }
	}
}


AnimationManager::AnimationManager()
		: frameBarrier_(2, setStagingCopyFlag) {
	resetTime();
	cpu_isUpdateActive_.clear(std::memory_order_release);
	gpu_isUpdateActive_.clear(std::memory_order_release);
	cpuUpdateThread_ = boost::thread(&AnimationManager::cpuUpdate, this);
}

AnimationManager::~AnimationManager() {
	// toggle atomic close flag to true
	closeFlag_.test_and_set(std::memory_order_release);
	// indicate arrival at the barrier and drop this thread
	frameBarrier_.arrive_and_drop();
	cpuUpdateThread_.join();
	// Finally also join the dedicated threads
	if (unsynced_numActiveUpdates_.load(std::memory_order_acquire) > 0) {
		waitOnCounter<int,0>(unsynced_numActiveUpdates_);
	}
	for (auto &thread : unsyncedThreads_) {
		thread.join();
	}
}

void AnimationManager::resetTime() {
	time_ = boost::posix_time::ptime(
			boost::posix_time::microsec_clock::local_time());
	lastTime_ = time_;
}

void AnimationManager::setRootState(const ref_ptr<State> &rootState) {
	std::set<Animation *> allAnimations;
	for (auto &anim : cpuAnimations_) {
		allAnimations.insert(anim);
	}
	for (auto &anim : gpuAnimations_) {
		allAnimations.insert(anim);
	}
	for (auto &anim : unsyncedAnimations_) {
		allAnimations.insert(anim);
	}

	if (rootState_.get()) {
		for (auto &anim : allAnimations) {
			anim->disjoinAnimationState(rootState_);
		}
	}
	rootState_ = rootState;
	if (rootState_.get()) {
		for (auto &anim : allAnimations) {
			anim->joinAnimationState(rootState);
		}
	}
}

void AnimationManager::addAnimation(Animation *animation) {
	if (animation->isGPUAnimation()) {
		gpu_commandQueue_.push({ ADD, animation });
	}

	if (animation->isCPUAnimation() && animation->isSynchronized()) {
		cpu_commandQueue_.push({ ADD, animation });
	} else if (animation->isCPUAnimation()) {
		// start a new dedicated thread
		boost::unique_lock lock(unsyncedListLock_);
		unsyncedAnimations_.emplace_back(animation);
		unsyncedThreads_.emplace_back([this, animation]() {
			unsyncedUpdate(animation);
		});
	}
}

void AnimationManager::removeAnimation(Animation *animation) {
	// Make sure we won't run this animation anymore
	animation->stopAnimation();

	if (animation->isGPUAnimation()) {
		gpu_commandQueue_.push({ REMOVE, animation });
	}

	if (animation->isCPUAnimation() && animation->isSynchronized()) {
		cpu_commandQueue_.push({ REMOVE, animation });
	} else if (animation->isCPUAnimation()) {
		// remove from list
		boost::unique_lock lock(unsyncedListLock_);
		for (size_t i = 0; i < unsyncedAnimations_.size(); i++) {
			if (unsyncedAnimations_[i] == animation) {
				unsyncedThreads_[i].join();
				unsyncedAnimations_.erase(unsyncedAnimations_.begin() + i);
				unsyncedThreads_.erase(unsyncedThreads_.begin() + i);
				break;
			}
		}
	}
}

void AnimationManager::waitForAnimations() const {
	// Block until all "isUpdateActive" flags are cleared in all *other* threads.
	const auto thisThreadID = boost::this_thread::get_id();
	if (thisThreadID != cpu_threadID_ && cpu_isUpdateActive_.test(std::memory_order_acquire)) {
		waitOnFlag<false>(cpu_isUpdateActive_);
	}
	if (thisThreadID != gpu_threadID_ && gpu_isUpdateActive_.test(std::memory_order_acquire)) {
		waitOnFlag<false>(gpu_isUpdateActive_);
	}
	if (unsynced_numActiveUpdates_.load(std::memory_order_acquire) > 0) {
		waitOnCounter<int,0>(unsynced_numActiveUpdates_);
	}
}

void AnimationManager::shutdown(bool blocking) {
	// toggle atomic close flag to true
	closeFlag_.test_and_set(std::memory_order_release);
	if (blocking) waitForAnimations();
}

void AnimationManager::pause(bool blocking) {
	// toggle atomic pause flag to true
	pauseFlag_.test_and_set(std::memory_order_release);
	if (blocking) waitForAnimations();
}

void AnimationManager::clear() {
	// note: assuming, pause(true) was called before, no animation is running now
	boost::unique_lock lock(unsyncedListLock_);
	cpuAnimations_.clear();
	gpuAnimations_.clear();
	unsyncedAnimations_.clear();
	spatialIndices_.clear();
}

void AnimationManager::resume(bool runOnce) {
	if(runOnce) {
		for (auto anim : cpuAnimations_) {
			if (anim->isRunning()) {
				anim->cpuUpdate(0.0);
			}
		}
	}
	// toggle atomic pause flag to false
	pauseFlag_.clear(std::memory_order_release);
}

void AnimationManager::gpuUpdateStep(double dt) {
	RenderState *rs = RenderState::get();
	// Remember the GPU thread ID
	gpu_threadID_ = boost::this_thread::get_id();

	// Set processing flags, so that other threads can wait for the completion of this loop
	gpu_isUpdateActive_.test_and_set(std::memory_order_acquire);

	// Avoid race condition with close waiting for gpu_isUpdateActive_ to clear
	if (closeFlag_.test(std::memory_order_acquire) || pauseFlag_.test(std::memory_order_acquire)) {
		gpu_isUpdateActive_.clear(std::memory_order_release);
		return;
	}

	// Advance each active GPU animation.
	for (const auto &anim : gpuAnimations_) {
		if (!anim->isRunning()) { continue; }
		auto animState = anim->animationState();
		animState->enable(rs);
		anim->gpuUpdate(rs, dt);
		animState->disable(rs);
	}

	// Clear processing flags
	gpu_isUpdateActive_.clear(std::memory_order_release);

	// Perform pending add/remove operations
	while (!gpu_commandQueue_.empty()) {
		Command &cmd = gpu_commandQueue_.front();
		switch (cmd.type) {
			case ADD:
				gpuAnimations_.push_back(cmd.animation);
				break;
			case REMOVE:
				gpuAnimations_.erase(std::ranges::remove(
					gpuAnimations_, cmd.animation).begin(), gpuAnimations_.end());
				break;
		}
		gpu_commandQueue_.pop();
	}
}

void AnimationManager::cpuUpdateStep() {
	// Compute dt in milliseconds
	double dt = static_cast<double>(
		(time_ - lastTime_).total_microseconds()) / 1000.0;

	// Set processing flags, so that other threads can wait for the completion of this loop
	cpu_isUpdateActive_.test_and_set(std::memory_order_acquire);

	// Avoid race conditions with close/pause waiting for cpu_isUpdateActive_ to clear
	if (closeFlag_.test(std::memory_order_acquire) || pauseFlag_.test(std::memory_order_acquire)) {
		cpu_isUpdateActive_.clear(std::memory_order_release);
		return;
	}

	// Advance each active CPU animation.
	for (const auto &anim : cpuAnimations_) {
		if (!anim->isRunning()) { continue; }
		anim->cpuUpdate(dt);
	}

	// Update visibility using spatial indices.
	// Note: this might be computationally heavy!
	for (auto &index : spatialIndices_) {
		index->update(static_cast<float>(dt));
	}

	if constexpr (ANIMATION_THREAD_SWAPS_CLIENT_BUFFERS) {
		swapClientData();
	}

	// Clear processing flags
	cpu_isUpdateActive_.clear(std::memory_order_release);

	// Perform pending add/remove operations
	while (!cpu_commandQueue_.empty()) {
		Command &cmd = cpu_commandQueue_.front();
		switch (cmd.type) {
			case ADD:
				cpuAnimations_.push_back(cmd.animation);
				break;
			case REMOVE:
				cpuAnimations_.erase(std::ranges::remove(
					cpuAnimations_, cmd.animation).begin(), cpuAnimations_.end());
				break;
		}
		cpu_commandQueue_.pop();
	}
}

void AnimationManager::cpuUpdate() {
	cpu_threadID_ = boost::this_thread::get_id();
	resetTime();

	while (closeFlag_.test(std::memory_order_acquire) == false) {
		time_ = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
		cpuUpdateStep();
		lastTime_ = time_;
		frameBarrier_.arrive_and_wait();
	}
}

void AnimationManager::unsyncedUpdate(Animation *animation) {
	using Clock = std::chrono::steady_clock;
	using ms = std::chrono::duration<double, std::milli>;

	const double targetMs = 1000.0 / animation->desiredFrameRate();
	const auto d_frameDuration = ms(targetMs);
	const auto frameDuration = std::chrono::duration_cast<Clock::duration>(d_frameDuration);
	auto nextFrame = Clock::now();

	while (closeFlag_.test(std::memory_order_acquire) == false && animation->isRunning()) {
		// Increase update counter atomic
		unsynced_numActiveUpdates_.fetch_add(1, std::memory_order_acquire);

		// Avoid race condition on close
		if (closeFlag_.test(std::memory_order_acquire)) {
			unsynced_numActiveUpdates_.fetch_sub(1, std::memory_order_release);
			break;
		}

		// Spin until we can continue
		if (pauseFlag_.test(std::memory_order_acquire)) {
			unsynced_numActiveUpdates_.fetch_sub(1, std::memory_order_release);
			waitOnFlag<false>(pauseFlag_);
			continue;
		}

		auto frameStart = Clock::now();
		double dt = std::chrono::duration<double, std::milli>(frameStart - nextFrame + frameDuration).count();

		// Run the animation logic
		animation->cpuUpdate(dt);

		// decrease update counter atomic
		unsynced_numActiveUpdates_.fetch_sub(1, std::memory_order_release);

		// Schedule next frame
		nextFrame += frameDuration;

		auto now = Clock::now();
		if (now < nextFrame) {
			std::this_thread::sleep_until(nextFrame);
		} else {
			// Missed the frame deadline: resync
			nextFrame = now;
		}
	}
}

void AnimationManager::swapClientData() {
	if (closeFlag_.test(std::memory_order_acquire)) return;
	auto &staging = StagingSystem::instance();
	// Wait for the staging system to finish copying client data for this frame.
	while (staging.isCopyInProgress() &&
		!closeFlag_.test(std::memory_order_acquire)) {
		CPU_PAUSE();
	}
	staging.swapClientData();
}
