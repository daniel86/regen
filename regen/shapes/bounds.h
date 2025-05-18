#ifndef REGEN_BOUNDS_H
#define REGEN_BOUNDS_H

#include <array>
#include <regen/math/vector.h>

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
		inline bool operator==(const Bounds<T> &other) const {
			return min == other.min && max == other.max;
		}

		/**
		 * @param other The other bounds
		 * @return true if the bounds are not equal
		 */
		inline bool operator!=(const Bounds<T> &other) const {
			return min != other.min || max != other.max;
		}

		/**
		 * @return The size of the bounds
		 */
		inline float size() const {
			return (max - min).length();
		}

		/**
		 * @return The center of the bounds
		 */
		inline T center() const {
			return (min + max) * 0.5f;
		}

		/**
		 * @return The radius of the bounds
		 */
		inline float radius() const {
			return size() * 0.5f;
		}

		/**
		 * Increase the bounds to include the given other bounds.
		 * @param other The other bounds
		 */
		inline void extend(const Bounds<T> &other) {
			min.setMin(other.min);
			max.setMax(other.max);
		}

		/**
		 * Check if the bounds contain the given point.
		 * @param point The point
		 * @return true if the bounds contain the point
		 */
		inline bool contains(const T &point) const {
			return point.x >= min.x && point.x <= max.x &&
				   point.y >= min.y && point.y <= max.y;
		}

		/**
		 * Check if the bounds contain the given other bounds.
		 * @param other The other bounds
		 * @return true if the bounds contain the other bounds
		 */
		inline bool contains(const Bounds<T> &other) const {
			return min.x <= other.min.x && max.x >= other.max.x &&
				   min.y <= other.min.y && max.y >= other.max.y;
		}

		/**
		 * @brief Get the corner points of the bounds
		 * @return The corner points of the bounds
		 */
		inline std::array<Vec3f, 8> cornerPoints() const {
			return {
				Vec3f(min.x, min.y, min.z),
				Vec3f(max.x, min.y, min.z),
				Vec3f(min.x, max.y, min.z),
				Vec3f(max.x, max.y, min.z),
				Vec3f(min.x, min.y, max.z),
				Vec3f(max.x, min.y, max.z),
				Vec3f(min.x, max.y, max.z),
				Vec3f(max.x, max.y, max.z)
			};
		}
	};
}

#endif //REGEN_BOUNDS_H
