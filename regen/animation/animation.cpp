#include <regen/animation/animation-manager.h>

#include "animation.h"

using namespace regen;

uint32_t Animation::ANIMATION_STOPPED = EventObject::registerEvent("animationStopped");

Animation::Animation(bool isGPUAnimation, bool isCPUAnimation)
		: EventObject(),
		  isGPUAnimation_(isGPUAnimation),
		  isCPUAnimation_(isCPUAnimation) {
	animationState_ = ref_ptr<State>::alloc();
	if (AnimationManager::get().rootState().get()) {
		animationState_->joinStates(AnimationManager::get().rootState());
	}
}

Animation::~Animation() {
	isRunning_.clear(std::memory_order_release);
	unQueueEmit(ANIMATION_STOPPED);
	AnimationManager::get().removeAnimation(this);
}

void Animation::startAnimation() {
	if (isRunning()) return;
	isRunning_.test_and_set(std::memory_order_acquire);
	unQueueEmit(ANIMATION_STOPPED);
	AnimationManager::get().addAnimation(this);
}

void Animation::doStopAnimation() {
	if (!isRunning()) return;
	isRunning_.clear(std::memory_order_release);
	queueEmit(ANIMATION_STOPPED);
	AnimationManager::get().removeAnimation(this);
}

void Animation::joinAnimationState(const ref_ptr<State> &state) {
	animationState_->joinStates(state);
}

void Animation::disjoinAnimationState(const ref_ptr<State> &rootState) {
	animationState_->disjoinStates(rootState);
}
