#ifndef REGEN_FRUSTUM_H_
#define REGEN_FRUSTUM_H_

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
		double fov = 60.0;
		/** The aspect ratio. */
		double aspect = 8.0 / 6.0;
		/** The near plane distance. */
		double near = 0.1;
		/** The far plane distance. */
		double far = 100.0;
		/** Near plane size. */
		Vec2f nearPlaneHalfSize;
		/** Far plane size. */
		Vec2f farPlaneHalfSize;
		/** Bounds of parallel projection */
		Bounds<Vec2f> orthoBounds;
		/** The 8 frustum points. 0-3 are the near plane points, 4-7 far plane. */
		Vec3f points[8];
		/** The 6 frustum planes. */
		Plane planes[6];

		Frustum();

		/**
		 * Set projection parameters and compute near- and far-plane.
		 */
		void setPerspective(double aspect, double fov, double near, double far);

		/**
		 * Set projection parameters and compute near- and far-plane.
		 */
		void setOrtho(double left, double right, double bottom, double top, double near, double far);

		/**
		 * Update frustum points based on view point and direction.
		 */
		void update(const Vec3f &pos, const Vec3f &dir);

		/**
		 * Split this frustum along the view ray.
		 */
		void split(double splitWeight, std::vector<Frustum> &frustumSplit) const;

		/**
		 * @return true if the sphere intersects with this frustum.
		 */
		bool hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) const;

		/**
		 * @return true if the box intersects with this frustum.
		 */
		bool hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) const;

		/**
		 * @return true if the box intersects with this frustum.
		 */
		bool hasIntersectionWithBox(const BoundingBox &box) const;

		/**
		 * @return true if the sphere intersects with this frustum.
		 */
		bool hasIntersectionWithSphere(const BoundingSphere &sphere) const;

		/**
		 * @return true if the frustum intersects with this frustum.
		 */
		bool hasIntersectionWithFrustum(const Frustum &other) const;

		/**
		 * @brief Get the direction of this frustum
		 * @return The direction
		 */
		Vec3f direction() const;

		// override BoundingShape::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// override BoundingShape::update
		bool updateTransform(bool forceUpdate) final;

		// BoundingShape interface
		void updateBaseBounds(const Vec3f &min, const Vec3f &max) override { }

	protected:
		ref_ptr<ShaderInput3f> direction_;
		unsigned int lastDirectionStamp_ = 0;

		unsigned int directionStamp() const;

		void updatePointsPerspective(const Vec3f &pos, const Vec3f &dir);
		void updatePointsOrthogonal(const Vec3f &pos, const Vec3f &dir);
	};
} // namespace

#endif /* REGEN_FRUSTUM_H_ */
