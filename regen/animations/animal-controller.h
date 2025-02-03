#ifndef REGEN_ANIMAL_CONTROLLER_H_
#define REGEN_ANIMAL_CONTROLLER_H_

#include <regen/math/vector.h>
#include <regen/gl-types/texture.h>
#include <regen/animations/animation-controller.h>
#include "animation-node.h"
#include "regen/shapes/bounds.h"
#include "regen/states/model-transformation.h"
#include "regen/math/bezier.h"

namespace regen {
	/**
	 * A controller for animal animations.
	 */
	class AnimalController : public AnimationController {
	public:
		/**
		 * The behavior of the animal.
		 */
		enum Behavior {
			BEHAVIOR_WALK = 0,
			BEHAVIOR_RUN,
			BEHAVIOR_IDLE,
			BEHAVIOR_SPECIAL,
			BEHAVIOR_SMELL,
			BEHAVIOR_ATTACK,
			BEHAVIOR_LAST
		};

		/**
		 * Constructor.
		 * @param tf the model transformation.
		 * @param animation the node animation.
		 * @param ranges the animation ranges.
		 */
		AnimalController(
			const ref_ptr<ModelTransformation> &tf,
			const ref_ptr<NodeAnimation> &animation,
			const std::vector<scene::AnimRange> &ranges);

		/**
		 * Set the walk speed.
		 * @param speed the speed.
		 */
		void setWalkSpeed(float speed) { walkSpeed_ = speed; }

		/**
		 * Set the run speed.
		 * @param speed the speed.
		 */
		void setRunSpeed(float speed) { runSpeed_ = speed; }

		/**
		 * Set the laziness.
		 * @param laziness the laziness.
		 */
		void setLaziness(float laziness) { laziness_ = laziness; }

		/**
		 * Set the max height.
		 * @param maxHeight the max height.
		 */
		void setMaxHeight(float maxHeight) { maxHeight_ = maxHeight; }

		/**
		 * Set the min height.
		 * @param minHeight the min height.
		 */
		void setMinHeight(float minHeight) { minHeight_ = minHeight; }

		/**
		 * Set the base orientation.
		 * @param baseOrientation the base orientation.
		 */
		void setBaseOrientation(float baseOrientation) { baseOrientation_ = baseOrientation; }

		/**
		 * Set the floor height.
		 * @param floorHeight the floor height.
		 */
		void setFloorHeight(float floorHeight) { floorHeight_ = floorHeight; }

		/**
		 * Add a special animation.
		 * @param special the special animation.
		 */
		void addSpecial(std::string_view special);

		/**
		 * Set the territory bounds.
		 * @param center the center.
		 * @param size the size.
		 */
		void setTerritoryBounds(const Vec2f &center, const Vec2f &size);

		/**
		 * Set the height map.
		 * @param heightMap the height map.
		 * @param heightMapCenter the center.
		 * @param heightMapSize the size.
		 * @param heightMapFactor the factor.
		 */
		void setHeightMap(
				const ref_ptr<Texture2D> &heightMap,
				const Vec2f &heightMapCenter,
				const Vec2f &heightMapSize,
				float heightMapFactor);

		// override
		void updateController(double dt) override;

		// override
		void updatePose(const TransformKeyFrame &currentFrame, double t) override;

	protected:
		Bounds<Vec2f> territoryBounds_;
		float walkSpeed_;
		float runSpeed_;
		float laziness_;
		float maxHeight_;
		float minHeight_;
		// base orientation of the mesh around y axis
		float baseOrientation_;
		float floorHeight_;

		math::Bezier<Vec2f> bezierPath_;

		ref_ptr<Texture2D> heightMap_;
		Bounds<Vec2f> heightMapBounds_;
		float heightMapFactor_ = 8.0f;

		Behavior behavior_ = BEHAVIOR_IDLE;
		std::map<Behavior, std::vector<const scene::AnimRange*> > behaviorRanges_;
		bool isLastAnimationMovement_ = false;

		void activateRandom();

		void activateMovement();

		Behavior selectNextBehavior();

		float getHeight(const Vec2f &pos);

	};
} // namespace

#endif /* REGEN_ANIMAL_CONTROLLER_H_ */
