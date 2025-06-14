#include <map>

#include <regen/utility/threading.h>
#include <regen/utility/logging.h>
#include "animation-manager.h"

using namespace regen;

// Microseconds to sleep per loop in idle mode.
#define IDLE_SLEEP 100000
// Synchronize animation and render thread.
#define SYNCHRONIZE_THREADS

AnimationManager &AnimationManager::get() {
	static AnimationManager manager;
	return manager;
}

AnimationManager::AnimationManager()
		: frameBarrier_(2),
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
#ifdef SYNCHRONIZE_THREADS
	frameBarrier_.arrive_and_wait();
#endif
}

void AnimationManager::runUnsynchronized(Animation *animation) const {
	// call animate until the animation is stopped
	// try to reach a dedicated frame rate.
	const auto goalTime = (1.0 / animation->desiredFrameRate()) * 1000.0;
	boost::posix_time::ptime time0, time1;
	time0 = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
	time1 = time0;
	double dt;
	int delta_micros;

	while (animation->isRunning()) {
		if (pauseFlag_) {
			usleepRegen(IDLE_SLEEP);
			time0 += boost::posix_time::microseconds(IDLE_SLEEP);
			time1 += boost::posix_time::microseconds(IDLE_SLEEP);
			continue;
		}

		// make a step
		dt = static_cast<double>((time1 - time0).total_microseconds()) / 1000.0;
		time0 = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
		animation->animate(static_cast<double>(dt));
		time1 = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());

		// synchronize to the desired frame rate
		dt = static_cast<double>((time1 - time0).total_microseconds()) / 1000.0;
		if (dt < goalTime) {
			delta_micros = static_cast<int>((goalTime - dt) * 1000);
			usleepRegen(delta_micros);
			time1 += boost::posix_time::microseconds(delta_micros);
		}
	}
}

void AnimationManager::run() {
	animationThreadID_ = boost::this_thread::get_id();
	resetTime();

	while (!closeFlag_) {
		time_ = boost::posix_time::ptime(
				boost::posix_time::microsec_clock::local_time());

		if (pauseFlag_ || synchronizedAnimations_.empty()) {
#ifndef SYNCHRONIZE_THREADS
			usleepRegen(IDLE_SLEEP);
#endif // SYNCHRONIZE_THREADS
		} else {
			double dt = ((GLdouble) (time_ - lastTime_).total_microseconds()) / 1000.0;

			// wait for remove/add to return
			while (removeInProgress_) usleepRegen(1000);
			while (addInProgress_) usleepRegen(1000);

			animInProgress_ = true;
			bool animsRemaining = true;
			std::set<Animation *> processed;
			while (animsRemaining) {
				animsRemaining = false;
				for (auto anim : synchronizedAnimations_) {
					processed.insert(anim);
					if (anim->isRunning()) {
						anim->animate(dt);
						// Animation was removed in animate call.
						// We have to restart the loop because iterator is invalid.
						if (animChangedDuringLoop_) {
							animChangedDuringLoop_ = false;
							animsRemaining = true;
							break;
						}
					}
				}
			}
			for (auto &index : spatialIndices_) {
				index.second->update(static_cast<float>(dt));
			}
			animInProgress_ = false;
#ifndef SYNCHRONIZE_THREADS
			if(dt<10) usleepRegen((10-dt) * 1000);
#endif // SYNCHRONIZE_THREADS
		}
		lastTime_ = time_;

#ifdef SYNCHRONIZE_THREADS
		frameBarrier_.arrive_and_wait();
#endif // SYNCHRONIZE_THREADS
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
