#ifndef REGEN_BOUNDING_SPHERE_H_
#define REGEN_BOUNDING_SPHERE_H_

#include <regen/shapes/bounding-shape.h>
#include "bounding-box.h"

namespace regen {
	/**
	 * @brief Bounding sphere
	 */
	class BoundingSphere : public BoundingShape {
	public:
		/**
		 * @brief Construct a new Bounding Sphere object
		 * @param mesh The mesh
		 */
		explicit BoundingSphere(const ref_ptr<Mesh> &mesh);

		/**
		 * @brief Construct a new Bounding Sphere object
		 * @param radius The radius of the sphere
		 */
		BoundingSphere(const Vec3f &basePosition, GLfloat radius);

		~BoundingSphere() override = default;

		/**
		 * @brief Get the radius of this sphere
		 * @return The radius
		 */
		auto radius() const { return radius_; }

		/**
		 * @brief Set the radius of this sphere
		 * @param radius The radius
		 */
		void setRadius(GLfloat radius) { radius_ = radius; }

		/**
		 * @brief Check if this sphere has intersection with another shape
		 * @param other The other shape
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithSphere(const BoundingShape &other) const;

		// override BoundingShape::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// override BoundingShape::update
		bool updateTransform(bool forceUpdate) final;

		// BoundingShape interface
		void updateBounds(const Vec3f &min, const Vec3f &max) override;

		// override BoundingShape::getCenterPosition
		Vec3f getCenterPosition() const override;

	protected:
		Vec3f basePosition_;
		GLfloat radius_;

		float computeRadius(const Vec3f &min, const Vec3f &max);
	};
} // namespace

#endif /* REGEN_BOUNDING_SPHERE_H_ */
