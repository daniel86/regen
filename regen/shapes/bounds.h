#ifndef REGEN_BOUNDS_H
#define REGEN_BOUNDS_H

namespace regen {
	/**
	 * @brief Bounds
	 * @tparam T The type of the bounds
	 */
	template<typename T>
	struct Bounds {
		T min;
		T max;

		/**
		 * @brief Construct a new Bounds object
		 * @param min The minimum bounds
		 * @param max The maximum bounds
		 */
		Bounds(const T &min, const T &max) : min(min), max(max) {}

		float size() const {
			return (max - min).length();
		}
	};
}

#endif //REGEN_BOUNDS_H
