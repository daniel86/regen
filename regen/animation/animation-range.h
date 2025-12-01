#ifndef REGEN_ANIMATION_RANGE_H_
#define REGEN_ANIMATION_RANGE_H_

#include <string>
#include <string_view>
#include <regen/compute/vector.h>

namespace regen {
	/**
	 * A named animation range.
	 * Unit might be ticks.
	 */
	struct AnimationRange {
		/**
		 * Default constructor.
		 */
		AnimationRange() : range(Vec2d(0.0, 0.0)) {}

		/**
		 * Constructor with name and range.
		 * @param n The range name.
		 * @param r The range value.
		 * @param i The track index.
		 */
		AnimationRange(std::string_view n, const Vec2d &r, uint32_t i = 0)
				: name(n), range(r), trackIndex(i) {}

		/**
		 * Constructor with name, range and channel name.
		 * @param n The range name.
		 * @param r The range value.
		 * @param trackName The track name.
		 */
		AnimationRange(std::string_view n, const Vec2d &r, const std::string &trackName)
				: name(n), range(r), trackName(trackName) {}

		/** The range name. */
		std::string name;
		/** The range value. */
		Vec2d range;
		/** The track index. */
		uint32_t trackIndex = 0;
		/** The track name. */
		std::string trackName;
	};
} // namespace

#endif /* REGEN_ANIMATION_RANGE_H_ */
