#include <map>
#include <time.h>
#include <boost/thread.hpp>

#include <regen/utility/threading.h>
#include <regen/utility/logging.h>
#include "animation-manager.h"

using namespace regen;

// Microseconds to sleep per loop in idle mode.
#define IDLE_SLEEP 100000

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

AnimationManager::AnimationManager()
		: frameBarrier_(2, setStagingCopyFlag),
		  animInProgress_(false),
		  glInProgress_(false),
		  removeInProgress_(false),
		  addInProgress_(false),
		  animChangedDuringLoop_(false),
		  glChangedDuringLoop_(false),
		  closeFlag_(false),
		  pauseFlag_(true) {
	resetTime();
	thread_ = boost::thread(&AnimationManager::run, this);
}

AnimationManager::~AnimationManager() {
	closeFlag_ = true;
	frameBarrier_.arrive_and_drop();
	thread_.join();
}

void AnimationManager::resetTime() {
	time_ = boost::posix_time::ptime(
			boost::posix_time::microsec_clock::local_time());
	lastTime_ = time_;
}

void AnimationManager::setRootState(const ref_ptr<State> &rootState) {
	std::set<Animation *> allAnimations;
	for (auto &anim : synchronizedAnimations_) {
		allAnimations.insert(anim);
	}
	for (auto &anim : unsynchronizedAnimations_) {
		allAnimations.insert(anim);
	}
	for (auto &anim : gpuAnimations_) {
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

void AnimationManager::setSpatialIndices(const std::map<std::string, ref_ptr<SpatialIndex>> &indices) {
	spatialIndices_ = indices;
}

void AnimationManager::addAnimation(Animation *animation) {
	// Don't add while removing
	while (removeInProgress_) usleepRegen(1000);

	addThreadID_ = boost::this_thread::get_id();
	addInProgress_ = true;

	if (animation->isGPUAnimation()) {
		if (glInProgress_ && addThreadID_ == glThreadID_) {
			// Called from glAnimate().
			glChangedDuringLoop_ = true;
			gpuAnimations_.insert(animation);
		} else {
			// Wait for the current loop to finish.
			if (glInProgress_) {
				addInProgress_ = false;
				while (glInProgress_) usleepRegen(1000);
				addInProgress_ = true;
			}
			// save to remove from set
			// FIXME: there is a race condition here if add/remove are called from different
			//        non GL threads they could insert/remove simultaneously
			gpuAnimations_.insert(animation);
		}
	}

	if (animation->isCPUAnimation()) {
		if (animation->isSynchronized()) {
			if (animInProgress_ && addThreadID_ == animationThreadID_) {
				// Called from animate().
				animChangedDuringLoop_ = true;
				synchronizedAnimations_.emplace_back(animation);
			} else {
				// Wait for the current loop to finish.
				while (animInProgress_) usleepRegen(1000);
				// save to remove from set
				synchronizedAnimations_.emplace_back(animation);
			}
		} else {
			// start a new dedicated thread
			boost::unique_lock<boost::mutex> lock(unsynchronizedMut_);
			unsynchronizedAnimations_.emplace_back(animation);
			unsynchronizedThreads_.emplace_back([this, animation]()
				{ runUnsynchronized(animation); });
		}
	}

	addInProgress_ = false;
}

void AnimationManager::removeAnimation(Animation *animation) {
	// Don't remove while adding
	while (addInProgress_) usleepRegen(1000);

	removeThreadID_ = boost::this_thread::get_id();
	removeInProgress_ = true;

	if (animation->isGPUAnimation()) {
		if (glInProgress_ && removeThreadID_ == glThreadID_) {
			// Called from glAnimate().
			glChangedDuringLoop_ = true;
			gpuAnimations_.erase(animation);
		} else {
			// Wait for the current loop to finish.
			if (glInProgress_) {
				removeInProgress_ = false;
				while (glInProgress_) usleepRegen(1000);
				removeInProgress_ = true;
			}
			// save to remove from set
			gpuAnimations_.erase(animation);
		}
	}

	if (animation->isCPUAnimation()) {
		if (animation->isSynchronized()) {
			if (animInProgress_ && removeThreadID_ == animationThreadID_) {
				// Called from animate().
				animChangedDuringLoop_ = true;
			} else {
				// Wait for the current loop to finish.
				while (animInProgress_) usleepRegen(1000);
				// save to remove from set
			}
			// remove from list
			auto it = synchronizedAnimations_.begin();
			while (it != synchronizedAnimations_.end()) {
				if (*it == animation) {
					synchronizedAnimations_.erase(it);
					break;
				}
				++it;
			}
		}
		else {
			// remove from list
			boost::unique_lock<boost::mutex> lock(unsynchronizedMut_);
			animation->isRunning_ = false;
			for (size_t i = 0; i < unsynchronizedAnimations_.size(); i++) {
				if (unsynchronizedAnimations_[i] == animation) {
					unsynchronizedThreads_[i].join();
					unsynchronizedAnimations_.erase(unsynchronizedAnimations_.begin() + i);
					unsynchronizedThreads_.erase(unsynchronizedThreads_.begin() + i);
					break;
				}
			}
		}
	}

	removeInProgress_ = false;
}

void AnimationManager::updateGraphics(RenderState *_, GLdouble dt) {
	if (pauseFlag_) { return; }
	glThreadID_ = boost::this_thread::get_id();

	// wait for remove/remove to return
	while (removeInProgress_) usleepRegen(1000);
	while (addInProgress_) usleepRegen(1000);

	// Set processing flags, so that other threads can wait
	// for the completion of this loop
	glInProgress_ = true;
	std::set<Animation *> processed;
	bool animationsRemaining = true;
	while (animationsRemaining && !pauseFlag_) {
		animationsRemaining = false;
		for (auto it = gpuAnimations_.begin(); it != gpuAnimations_.end(); ++it) {
			Animation *anim = *it;
			processed.insert(anim);
			if (anim->isRunning()) {
				auto animState = anim->animationState();
				animState->enable(RenderState::get());
				anim->glAnimate(RenderState::get(), dt);
				animState->disable(RenderState::get());
				// Animation was removed in glAnimate call.
				// We have to restart the loop because iterator is invalid.
				if (glChangedDuringLoop_) {
					glChangedDuringLoop_ = false;
					animationsRemaining = true;
					break;
				}
			}
		}
	}
	glInProgress_ = false;
}

void AnimationManager::flushGraphics() {
	frameBarrier_.arrive_and_wait();
}

void AnimationManager::runUnsynchronized(Animation *animation) const {
	using Clock = std::chrono::steady_clock;
	using ms = std::chrono::duration<double, std::milli>;

	const double targetMs = 1000.0 / animation->desiredFrameRate();
	const auto d_frameDuration = ms(targetMs);
	const auto frameDuration = std::chrono::duration_cast<Clock::duration>(d_frameDuration);
	auto nextFrame = Clock::now();

	while (!closeFlag_ && animation->isRunning()) {
		if (pauseFlag_) {
			usleepRegen(IDLE_SLEEP);  // or sleep_for()
			nextFrame += std::chrono::microseconds(IDLE_SLEEP);
			continue;
		}

		auto frameStart = Clock::now();
		double dt = std::chrono::duration<double, std::milli>(frameStart - nextFrame + frameDuration).count();

		// Run the animation logic
		animation->animate(dt);

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
	if (closeFlag_) return;
	auto &staging = StagingSystem::instance();
	// Wait for the staging system to finish copying client data for this frame.
	while (staging.isCopyInProgress()) {
		CPU_PAUSE();
		if (closeFlag_) return;
	}
	staging.swapClientData();
}

void AnimationManager::updateAnimations_cpu(double dt) {
	if (synchronizedAnimations_.empty()) return;

	bool areAnimationsRemaining = true;
	std::set<Animation *> processed;
	while (areAnimationsRemaining) {
		areAnimationsRemaining = false;
		for (auto anim: synchronizedAnimations_) {
			processed.insert(anim);
			if (anim->isRunning()) {
				anim->animate(dt);
				// Animation was removed in animate call.
				// We have to restart the loop because iterator is invalid.
				if (animChangedDuringLoop_) {
					animChangedDuringLoop_ = false;
					areAnimationsRemaining = true;
					break;
				}
			}
		}
	}
}

void AnimationManager::run() {
	animationThreadID_ = boost::this_thread::get_id();
	resetTime();

	while (!closeFlag_) {
		time_ = boost::posix_time::ptime(
				boost::posix_time::microsec_clock::local_time());

		if (!pauseFlag_) {
			double dt = ((GLdouble) (time_ - lastTime_).total_microseconds()) / 1000.0;
			// wait for remove/add to return
			while (removeInProgress_) usleepRegen(1000);
			while (addInProgress_) usleepRegen(1000);
			animInProgress_ = true;

			// Advance each CPU animation.
			// Main point is writing shader data that will be added to
			// staging next frame.
			updateAnimations_cpu(dt);
			// Update visibility using spatial indices.
			// Note: this might be computationally heavy!
			for (auto &index : spatialIndices_) {
				index.second->update(static_cast<float>(dt));
			}
#ifdef REGEN_STAGING_ANIMATION_THREAD_SWAPS_CLIENT
			// make client buffers we just wrote available for the next frame in the staging system.
			swapClientData();
#endif

			animInProgress_ = false;
		}
		lastTime_ = time_;
		frameBarrier_.arrive_and_wait();
	}
}

void AnimationManager::close(bool blocking) {
	closeFlag_ = true;
	if (blocking) {
		boost::thread::id callingThread = boost::this_thread::get_id();
		if (callingThread != animationThreadID_)
			while (animInProgress_) usleepRegen(1000);
		if (callingThread != glThreadID_)
			while (glInProgress_) usleepRegen(1000);
	}
}

void AnimationManager::pause(bool blocking) {
	pauseFlag_ = true;
	if (blocking) {
		boost::thread::id callingThread = boost::this_thread::get_id();
		if (callingThread != animationThreadID_)
			while (animInProgress_) usleepRegen(1000);
		if (callingThread != glThreadID_)
			while (glInProgress_) usleepRegen(1000);
	}
}

void AnimationManager::clear() {
	synchronizedAnimations_.clear();
	unsynchronizedAnimations_.clear();
	gpuAnimations_.clear();
	spatialIndices_.clear();
}

void AnimationManager::resume(bool runOnce) {
	if(runOnce) {
		for (auto anim : synchronizedAnimations_) {
			if (anim->isRunning()) {
				anim->animate(0.0);
			}
		}
	}
	pauseFlag_ = false;
}
