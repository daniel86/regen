#ifndef REGEN_ANIMATION_CONTROLLER_H_
#define REGEN_ANIMATION_CONTROLLER_H_

#include "animation-node.h"
#include "regen/states/model-transformation.h"
#include "transform-animation.h"

namespace regen {
	/**
	 * A controller of a mesh animation.
	 * Has access to the mesh transform and node animation to
	 * control the bone animations.
	 */
	class AnimationController : public TransformAnimation {
	public:
		AnimationController(
			const ref_ptr<ModelTransformation> &tf,
			const ref_ptr<NodeAnimation> &animalAnimation,
			const std::vector<scene::AnimRange> &ranges);

		/**
		 * Set the target for movement.
		 * @param target the target position.
		 * @param rotation the target rotation.
		 * @param dt the time difference.
		 */
		void setTarget(
					const Vec3f &target,
					const std::optional<Vec3f> &rotation,
					GLdouble dt);

		/**
		 * Update animation and movement target.
		 * @param dt the time difference.
		 */
		virtual void updateController(double dt) = 0;

		// override
		void animate(GLdouble dt) override;

	protected:
		ref_ptr<NodeAnimation> animation_;
		std::vector<scene::AnimRange> animationRanges_;
		ref_ptr<ModelTransformation> tf_;
		const scene::AnimRange *lastRange_ = nullptr;
	};
} // namespace

#endif /* REGEN_ANIMATION_CONTROLLER_H_ */
