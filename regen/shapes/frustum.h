/*
 * frustum.h
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#ifndef FRUSTUM_H_
#define FRUSTUM_H_

#include <vector>
#include <regen/math/vector.h>
#include <regen/math/plane.h>
#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/bounding-box.h>
#include <regen/shapes/bounding-sphere.h>

namespace regen {
	/**
	 * A portion of a solid pyramid that lies between two parallel planes cutting it.
	 */
	class Frustum : public BoundingShape {
	public:
		/** The field of view angle. */
		double fov;
		/** The aspect ratio. */
		double aspect;
		/** The near plane distance. */
		double near;
		/** The far plane distance. */
		double far;
		/** Near plane size. */
		Vec2f nearPlane;
		/** Far plane size. */
		Vec2f farPlane;
		/** The 8 frustum points. */
		Vec3f points[8];
		/** The 6 frustum planes. */
		Plane planes[6];

		Frustum();

		/**
		 * Set projection parameters and compute near- and far-plane.
		 */
		void set(double aspect, double fov, double near, double far);

		/**
		 * Update frustum points based on view point and direction.
		 */
		void update(const Vec3f &pos, const Vec3f &dir);

		/**
		 * Split this frustum along the view ray.
		 */
		std::vector<Frustum *> split(GLuint count, GLdouble splitWeight) const;

		/**
		 * @return true if the sphere intersects with this frustum.
		 */
		bool hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) const;

		/**
		 * @return true if the box intersects with this frustum.
		 */
		bool hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) const;

		/**
		 * @return true if the sphere intersects with this frustum.
		 */
		bool hasIntersectionWithFrustum(const BoundingSphere &sphere) const;

		/**
		 * @return true if the box intersects with this frustum.
		 */
		bool hasIntersectionWithFrustum(const BoundingBox &box) const;

		/**
		 * @return true if the frustum intersects with this frustum.
		 */
		bool hasIntersectionWithFrustum(const Frustum &other) const;

		/**
		 * @brief Get the direction of this frustum
		 * @return The direction
		 */
		const Vec3f &direction() const;

		// override BoundingShape::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// override BoundingShape::update
		bool update() final;

		// override BoundingShape::getCenterPosition
		Vec3f getCenterPosition() const override;

	protected:
		ref_ptr<ShaderInput3f> direction_;
		unsigned int lastDirectionStamp_ = 0;

		unsigned int directionStamp() const;
	};
} // namespace

#endif /* FRUSTUM_H_ */
