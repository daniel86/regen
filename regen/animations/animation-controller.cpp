#include "animation-controller.h"

using namespace regen;

AnimationController::AnimationController(
			const ref_ptr<ModelTransformation> &tf,
			const ref_ptr<NodeAnimation> &animalAnimation,
			const std::vector<scene::AnimRange> &ranges) :
		TransformAnimation(tf->modelMat()),
		animation_(animalAnimation),
		animationRanges_(ranges),
		tf_(tf) {
}

void AnimationController::setTarget(
			const Vec3f &target,
			const std::optional<Vec3f> &rotation,
			GLdouble dt) {
	frames_.clear();
	TransformAnimation::push_back(target, rotation, dt);
	if (!isRunning()) { startAnimation(); }
}

void AnimationController::animate(GLdouble dt) {
	updateController(dt);
	if (it_ != frames_.end()) {
		TransformAnimation::animate(dt);
	}
}
