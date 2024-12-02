/*
 * animation-manager.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <map>
#include <limits.h>

#include <regen/config.h>
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
		: Thread(),
		  animInProgress_(GL_FALSE),
		  glInProgress_(GL_FALSE),
		  removeInProgress_(GL_FALSE),
		  addInProgress_(GL_FALSE),
		  animChangedDuringLoop_(GL_FALSE),
		  glChangedDuringLoop_(GL_FALSE),
		  closeFlag_(GL_FALSE),
		  pauseFlag_(GL_TRUE),
		  hasNextFrame_(GL_FALSE),
		  hasNextStep_(GL_FALSE) {
	resetTime();
}

AnimationManager::~AnimationManager() {
	closeFlag_ = GL_TRUE;
	nextFrame();
	thread_.join();
}

void AnimationManager::resetTime() {
	time_ = boost::posix_time::ptime(
			boost::posix_time::microsec_clock::local_time());
	lastTime_ = time_;
}

void AnimationManager::setRootState(const ref_ptr<State> &rootState) {
	if (rootState_.get()) {
		for (auto &anim : animations_) {
			anim->disjoinAnimationState(rootState_);
		}
	}
	rootState_ = rootState;
	if (rootState_.get()) {
		for (auto &anim : animations_) {
			anim->joinAnimationState(rootState);
		}
	}
}

void AnimationManager::addAnimation(Animation *animation) {
	// Don't add while removing
	while (removeInProgress_) usleepRegen(1000);

	addThreadID_ = boost::this_thread::get_id();
	addInProgress_ = GL_TRUE;

	if (animation->useGLAnimation()) {
		if (rootState_.get() != nullptr) {
			animation->joinAnimationState(rootState_);
		}
		if (glInProgress_ && addThreadID_ == glThreadID_) {
			// Called from glAnimate().
			glChangedDuringLoop_ = GL_TRUE;
			glAnimations_.insert(animation);
		} else {
			// Wait for the current loop to finish.
			while (glInProgress_) usleepRegen(1000);
			// save to remove from set
			glAnimations_.insert(animation);
		}
	}

	if (animation->useAnimation()) {
		if (animInProgress_ && addThreadID_ == animationThreadID_) {
			// Called from animate().
			animChangedDuringLoop_ = GL_TRUE;
			animations_.emplace_back(animation);
		} else {
			// Wait for the current loop to finish.
			while (animInProgress_) usleepRegen(1000);
			// save to remove from set
			animations_.emplace_back(animation);
		}
	}

	addInProgress_ = GL_FALSE;
}

void AnimationManager::removeAnimation(Animation *animation) {
	// Don't remove while adding
	while (addInProgress_) usleepRegen(1000);

	removeThreadID_ = boost::this_thread::get_id();
	removeInProgress_ = GL_TRUE;

	if (animation->useGLAnimation()) {
		if (glInProgress_ && removeThreadID_ == glThreadID_) {
			// Called from glAnimate().
			glChangedDuringLoop_ = GL_TRUE;
			glAnimations_.erase(animation);
		} else {
			// Wait for the current loop to finish.
			while (glInProgress_) usleepRegen(1000);
			// save to remove from set
			glAnimations_.erase(animation);
		}
	}

	if (animation->useAnimation()) {
		if (animInProgress_ && removeThreadID_ == animationThreadID_) {
			// Called from animate().
			animChangedDuringLoop_ = GL_TRUE;
		} else {
			// Wait for the current loop to finish.
			while (animInProgress_) usleepRegen(1000);
			// save to remove from set
		}

		// remove from list
		auto it = animations_.begin();
		while (it != animations_.end()) {
			if (*it == animation) {
				animations_.erase(it);
				break;
			}
			++it;
		}
	}

	removeInProgress_ = GL_FALSE;
}

void AnimationManager::nextFrame() {
	// set the next frame condition to true
	// and notify waitForFrame if it is waiting.
	// waitForStep waits only if it was faster to render
	// a new frame then calculating the next animation step
	{
		boost::lock_guard<boost::mutex> lock(frameMut_);
		hasNextFrame_ = GL_TRUE;
	}
	frameCond_.notify_all();
}

void AnimationManager::nextStep() {
	// set the next step condition to true
	// and notify waitForStep if it is waiting.
	// waitForStep waits only if it was faster to render
	// a new frame then calculating the next animation step
	{
		boost::lock_guard<boost::mutex> lock(stepMut_);
		hasNextStep_ = GL_TRUE;
	}
	stepCond_.notify_all();
}

void AnimationManager::waitForFrame() {
	// wait until a new frame is rendered.
	{
		boost::unique_lock<boost::mutex> lock(frameMut_);
		while (!hasNextFrame_) {
			frameCond_.wait(lock);
		}
	}
	// toggle hasNextFrame_ to false
	{
		boost::lock_guard<boost::mutex> lock(frameMut_);
		hasNextFrame_ = GL_FALSE;
	}
}

void AnimationManager::waitForStep() {
	// wait until a new animation step was calculated.
	{
		boost::unique_lock<boost::mutex> lock(stepMut_);
		while (!hasNextStep_) {
			stepCond_.wait(lock);
		}
	}
	{
		boost::lock_guard<boost::mutex> lock(stepMut_);
		hasNextStep_ = GL_FALSE;
	}
}

void AnimationManager::updateGraphics(RenderState *_, GLdouble dt) {
	if (pauseFlag_) return;
	glThreadID_ = boost::this_thread::get_id();

#ifdef SYNCHRONIZE_THREADS
	nextFrame();
#endif
	// wait for remove/remove to return
	while (removeInProgress_) usleepRegen(1000);
	while (addInProgress_) usleepRegen(1000);

	// Set processing flags, so that other threads can wait
	// for the completion of this loop
	glInProgress_ = GL_TRUE;
	std::set<Animation *> processed;
	GLboolean animsRemaining = GL_TRUE;
	while (animsRemaining && !pauseFlag_) {
		animsRemaining = GL_FALSE;
		for (auto it = glAnimations_.begin(); it != glAnimations_.end(); ++it) {
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
					glChangedDuringLoop_ = GL_FALSE;
					animsRemaining = GL_TRUE;
					break;
				}
			}
		}
	}
	glInProgress_ = GL_FALSE;

#ifdef SYNCHRONIZE_THREADS
	waitForStep();
#endif
}

void AnimationManager::run() {
	animationThreadID_ = boost::this_thread::get_id();
	resetTime();

	while (!closeFlag_) {
		time_ = boost::posix_time::ptime(
				boost::posix_time::microsec_clock::local_time());

		if (pauseFlag_ || animations_.empty()) {
#ifndef SYNCHRONIZE_THREADS
			usleepRegen(IDLE_SLEEP);
#endif // SYNCHRONIZE_THREADS
		} else {
			GLdouble dt = ((GLdouble) (time_ - lastTime_).total_microseconds()) / 1000.0;

			// wait for remove/add to return
			while (removeInProgress_) usleepRegen(1000);
			while (addInProgress_) usleepRegen(1000);

			animInProgress_ = GL_TRUE;
			GLboolean animsRemaining = GL_TRUE;
			std::set<Animation *> processed;
			while (animsRemaining) {
				animsRemaining = GL_FALSE;
				for (auto anim : animations_) {
					processed.insert(anim);
					if (anim->isRunning()) {
						anim->animate(dt);
						// Animation was removed in animate call.
						// We have to restart the loop because iterator is invalid.
						if (animChangedDuringLoop_) {
							animChangedDuringLoop_ = GL_FALSE;
							animsRemaining = GL_TRUE;
							break;
						}
					}
				}
			}
			animInProgress_ = GL_FALSE;
#ifndef SYNCHRONIZE_THREADS
			if(dt<10) usleepRegen((10-dt) * 1000);
#endif // SYNCHRONIZE_THREADS
		}
		lastTime_ = time_;

#ifdef SYNCHRONIZE_THREADS
		nextStep();
		waitForFrame();
#endif // SYNCHRONIZE_THREADS
	}
}

void AnimationManager::close(GLboolean blocking) {
	closeFlag_ = GL_TRUE;
	if (blocking) {
		boost::thread::id callingThread = boost::this_thread::get_id();
		if (callingThread != animationThreadID_)
			while (animInProgress_) usleepRegen(1000); // TODO: rather use signals
		if (callingThread != glThreadID_)
			while (glInProgress_) usleepRegen(1000);
	}
}

void AnimationManager::pause(GLboolean blocking) {
	pauseFlag_ = GL_TRUE;
	if (blocking) {
		boost::thread::id callingThread = boost::this_thread::get_id();
		if (callingThread != animationThreadID_)
			while (animInProgress_) usleepRegen(1000);
		if (callingThread != glThreadID_)
			while (glInProgress_) usleepRegen(1000);
	}
}

void AnimationManager::clear() {
	animations_.clear();
	glAnimations_.clear();
}

void AnimationManager::resume() {
	pauseFlag_ = GL_FALSE;
}
