#ifndef REGEN_BOUNDING_SPHERE_H_
#define REGEN_BOUNDING_SPHERE_H_

#include <regen/shapes/bounding-shape.h>

namespace regen {
	/**
	 * @brief Bounding sphere
	 */
	class BoundingSphere : public BoundingShape {
	public:
		/**
		 * @brief Construct a new Bounding Sphere object
		 * @param mesh The mesh
		 * @param parts The parts of the mesh
		 * @param radius The radius of the sphere (if 0, it will be computed from the mesh)
		 */
		BoundingSphere(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts, float radius = 0.0f);

		/**
		 * @brief Construct a new Bounding Sphere object
		 * @param basePosition The base position of the sphere (without transformation)
		 * @param radius The radius of the sphere
		 */
		BoundingSphere(const Vec3f &basePosition, GLfloat radius);

		~BoundingSphere() override = default;

		/**
		 * @brief Get the radius of this sphere
		 * @return The radius
		 */
		float radius() const { return radius_; }

		/**
		 * @brief Get the squared radius of this sphere
		 * @return The squared radius
		 */
		float radiusSquared() const { return radiusSquared_; }

		/**
		 * @brief Set the radius of this sphere
		 * @param radius The radius
		 */
		void setRadius(float radius) { radius_ = radius; }

		/**
		 * @brief Check if this sphere has intersection with another shape
		 * @param other The other shape
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithShape(const BoundingShape &other) const;

		/**
		 * @brief Check if this sphere has intersection with another sphere
		 * @param other The other sphere
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithSphere(const BoundingSphere &other) const;

		// override BoundingShape::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// override BoundingShape::update
		bool updateTransform(bool forceUpdate) final;

		// BoundingShape interface
		void updateBaseBounds(const Vec3f &min, const Vec3f &max) override;

	protected:
		Vec3f basePosition_;
		float radius_;
		float radiusSquared_;

		float computeRadius(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts) const;

		void updateShapeOrigin();
	};
} // namespace

#endif /* REGEN_BOUNDING_SPHERE_H_ */
