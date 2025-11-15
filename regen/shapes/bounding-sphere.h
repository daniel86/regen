#ifndef REGEN_BOUNDING_SPHERE_H_
#define REGEN_BOUNDING_SPHERE_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/aabb.h>
#include <regen/shapes/obb.h>
#include <regen/utility/aligned-array.h>
#include "batch-of-shapes.h"

namespace regen {
	/**
	 * @brief Batch structure for sphere shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple spheres in a batched manner.
	 */
	struct BatchOfSpheres : BatchOfShapes {
		BatchOfSpheres() : BatchOfShapes(4) {}
		~BatchOfSpheres() override = default;
		// Queued sphere center position + radius
		AlignedArray<float>& posX() { return soaData_[0]; }
		AlignedArray<float>& posY() { return soaData_[1]; }
		AlignedArray<float>& posZ() { return soaData_[2]; }
		AlignedArray<float>& radius() { return soaData_[3]; }

		const AlignedArray<float>& posX() const { return soaData_[0]; }
		const AlignedArray<float>& posY() const { return soaData_[1]; }
		const AlignedArray<float>& posZ() const { return soaData_[2]; }
		const AlignedArray<float>& radius() const { return soaData_[3]; }
	};

	/**
	 * @brief Bounding sphere
	 */
	class BoundingSphere : public BatchedBoundingShape<BatchOfSpheres> {
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
		BoundingSphere(const Vec3f &basePosition, float radius);

		~BoundingSphere() override = default;

		/**
		 * @brief Get the radius of this sphere
		 * @return The radius
		 */
		float radius() const { return globalBatchData_.radius()[globalIndex_]; }

		/**
		 * @brief Get the squared radius of this sphere
		 * @return The squared radius
		 */
		float radiusSquared() const { return radiusSquared_; }

		/**
		 * @brief Set the radius of this sphere
		 * @param radius The radius
		 */
		void setRadius(float radius) { globalBatchData_.radius()[globalIndex_] = radius; }

		/**
		 * @brief Check if this sphere has intersection with an AABB
		 * @param box The AABB
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithAABB(const AABB &box) const;

		/**
		 * @brief Check if this sphere has intersection with an OBB
		 * @param box The OBB
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithOBB(const OBB &box) const;

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
		float radiusSquared_;

		float computeRadius(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts) const;

		void updateShapeOrigin();
	};
} // namespace

#include "batched-intersection.h"

namespace regen {
	namespace shapes {
		void flush_Sphere_Spheres(BatchedIntersectionCase&);
		void flush_Sphere_AABBs(BatchedIntersectionCase&);
		void flush_Sphere_OBBs(BatchedIntersectionCase&);
		void flush_Sphere_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<BoundingShapeType::SPHERE, BoundingShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_Spheres;
	};

	template<> struct IntersectionTraits<BoundingShapeType::SPHERE, BoundingShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_AABBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_AABBs;
	};

	template<> struct IntersectionTraits<BoundingShapeType::SPHERE, BoundingShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_OBBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_OBBs;
	};

	template<> struct IntersectionTraits<BoundingShapeType::SPHERE, BoundingShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_Frustums;
	};

	/**
	 * @brief Intersection shape data for sphere shapes.
	 */
	struct IntersectionData_Sphere : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};
} // namespace

#endif /* REGEN_BOUNDING_SPHERE_H_ */
