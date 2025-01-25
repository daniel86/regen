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

		/**
		 * @param other The other bounds
		 * @return true if the bounds are equal
		 */
		bool operator==(const Bounds<T> &other) const {
			return min == other.min && max == other.max;
		}

		/**
		 * @param other The other bounds
		 * @return true if the bounds are not equal
		 */
		bool operator!=(const Bounds<T> &other) const {
			return min != other.min || max != other.max;
		}

		/**
		 * @return The size of the bounds
		 */
		float size() const {
			return (max - min).length();
		}

		/**
		 * Increase the bounds to include the given other bounds.
		 * @param other The other bounds
		 */
		void extend(const Bounds<T> &other) {
			min.setMin(other.min);
			max.setMax(other.max);
		}
	};
}

#endif //REGEN_BOUNDS_H
