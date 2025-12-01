#ifndef REGEN_ANIMATION_CHANNEL_H_
#define REGEN_ANIMATION_CHANNEL_H_

#include <string>
#include <regen/utility/stamped.h>
#include "regen/utility/ref-ptr.h"
#include <regen/compute/vector.h>
#include <regen/compute/quaternion.h>

namespace regen {
	/**
	 * Defines behavior for first or last key frame.
	 */
	enum class AnimationChannelBehavior {
		// The value from the default node transformation is taken.
		DEFAULT = 0x0,
		// The nearest key value is used without interpolation.
		CONSTANT = 0x1,
		// The value of the nearest two keys is linearly extrapolated for the current time value.
		LINEAR = 0x2,
		// The animation is repeated.
		// If the animation key go from n to m and the current time is t, use the value at (t-n) % (|m-n|).
		REPEAT = 0x3
	};

	/**
	 * \brief Each channel affects a single node.
	 */
	struct AnimationChannel {
		/**
		 * The name of the node affected by this animation. The node
		 * must exist and it must be unique.
		 */
		std::string nodeName_;
		/** The index of the node affected by this animation. */
		uint32_t nodeIndex_ = 0;
		/** Indicates whether this channel has any key frames. */
		bool isAnimated = true;
		/**
		 * Defines how the animation behaves after the last key was processed.
		 * The default value is ANIM_BEHAVIOR_DEFAULT
		 * (the original transformation matrix of the affected node is taken).
		 */
		AnimationChannelBehavior postState;
		/**
		 * Defines how the animation behaves before the first key is encountered.
		 * The default value is ANIM_BEHAVIOR_DEFAULT
		 * (the original transformation matrix of the affected node is used).
		 */
		AnimationChannelBehavior preState;
		std::vector<Stamped<Vec3f>> scalingKeys_; /**< Scaling key frames. */
		std::vector<Stamped<Vec3f>> positionKeys_; /**< Position key frames. */
		std::vector<Stamped<Quaternion>> rotationKeys_; /**< Rotation key frames. */
	};

	/**
	 * \brief Static animation data containing all channels for an animation.
	 */
	struct StaticAnimationData {
		std::vector<AnimationChannel> channels;
	};

	std::ostream &operator<<(std::ostream &out, const AnimationChannelBehavior &v);

	std::istream &operator>>(std::istream &in, AnimationChannelBehavior &v);
} // namespace

#endif /* REGEN_ANIMATION_CHANNEL_H_ */
