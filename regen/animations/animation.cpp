/*
 * animation.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <regen/animations/animation-manager.h>
#include <regen/utility/threading.h>
#include <regen/utility/logging.h>
#include <regen/config.h>

#include "animation.h"

using namespace regen;

GLuint Animation::ANIMATION_STARTED = EventObject::registerEvent("animationStarted");
GLuint Animation::ANIMATION_STOPPED = EventObject::registerEvent("animationStopped");

Animation::Animation(bool isGPUAnimation, bool isCPUAnimation)
		: EventObject(),
		  isGPUAnimation_(isGPUAnimation),
		  isCPUAnimation_(isCPUAnimation),
		  isRunning_(false) {
	animationState_ = ref_ptr<State>::alloc();
	if (AnimationManager::get().rootState().get()) {
		animationState_->joinStates(AnimationManager::get().rootState());
	}
}

Animation::~Animation() {
	if (!try_lock()) {
		REGEN_WARN("Destroying Animation during it's locked.");
	} else {
		unlock();
	}
	if (!try_lock_gl()) {
		REGEN_WARN("Destroying GL Animation during it's locked.");
	} else {
		unlock_gl();
	}
	stopAnimation();
}

void Animation::startAnimation() {
	if (isRunning_) return;
	isRunning_ = true;

	unqueueEmit(ANIMATION_STOPPED);
	queueEmit(ANIMATION_STARTED);
	AnimationManager::get().addAnimation(this);
}

void Animation::stopAnimation() {
	if (!isRunning_) return;

	unqueueEmit(ANIMATION_STARTED);
	queueEmit(ANIMATION_STOPPED);
	AnimationManager::get().removeAnimation(this);
	isRunning_ = false;
}

GLboolean Animation::try_lock() { return mutex_.try_lock(); }

void Animation::lock() { mutex_.lock(); }

void Animation::unlock() { mutex_.unlock(); }

GLboolean Animation::try_lock_gl() { return mutex_gl_.try_lock(); }

void Animation::lock_gl() { mutex_gl_.lock(); }

void Animation::unlock_gl() { mutex_gl_.unlock(); }

void Animation::wait(GLuint milliseconds) {
	usleepRegen(1000 * milliseconds);
}

void Animation::joinAnimationState(const ref_ptr<State> &state) {
	animationState_->joinStates(state);
}

void Animation::disjoinAnimationState(const ref_ptr<State> &rootState) {
	animationState_->disjoinStates(rootState);
}
