/*
 * plane.h
 *
 *  Created on: Oct 16, 2014
 *      Author: daniel
 */

#ifndef PLANE_H_
#define PLANE_H_

#include <regen/math/vector.h>

namespace regen {
	/**
	 * Infinite plane in 3D space.
	 */
	class Plane {
	public:
		Vec3f point;
		Vec3f normal;

		Plane();

		Plane(const Vec3f &p, const Vec3f &n);

		/**
		 * @return the coefficients (a, b, c, d) of the plane equation ax + by + cz + d = 0.
		 */
		Vec4f equation() const;

		/**
		 * Set plane coefficients based on 3 arbitrary points on the plane.
		 */
		void set(const Vec3f &p0, const Vec3f &p1, const Vec3f &p2);

		/**
		 * @return Minimum distance between this plane and a point.
		 */
		inline float distance(const Vec3f &p) const {
			return normal.dot(point - p);
		}

		/**
		 * @return Closest point on this plane to a point.
		 */
		inline Vec3f closestPoint(const Vec3f &p) const {
			return p - normal * distance(p);
		}
	};
}


#endif /* PLANE_H_ */
