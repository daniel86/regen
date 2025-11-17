#ifndef REGEN_PLANE_H_
#define REGEN_PLANE_H_

#include <regen/math/vector.h>

namespace regen {
	/**
	 * Infinite plane in 3D space.
	 */
	class Plane {
	public:
		// the coefficients (a, b, c, d) of the plane equation ax + by + cz + d = 0.
		Vec4f coefficients;

		Plane() = default;

		/**
		 * Set plane coefficients based on 3 arbitrary points on the plane.
		 */
		void set(const Vec3f &p0, const Vec3f &p1, const Vec3f &p2) {
			coefficients.xyz_() = (p1 - p0).cross(p2 - p0);
			coefficients.xyz_().normalize();
			coefficients.w = coefficients.xyz_().dot(p0);
		}

		/**
		 * @return Normal vector of the plane.
		 */
		const Vec3f& normal() const { return coefficients.xyz_(); }

		/**
		 * @return Minimum distance between this plane and a point.
		 */
		float distance(const Vec3f &p) const {
			return normal().dot(p) - coefficients.w;
		}

		/**
		 * @return Closest point on this plane to a point.
		 */
		Vec3f closestPoint(const Vec3f &p) const {
			return p + normal() * distance(p);
		}
	};
}

#endif /* REGEN_PLANE_H_ */
