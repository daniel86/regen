#ifndef REGEN_BOUNDING_SPHERE_H_
#define REGEN_BOUNDING_SPHERE_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/aabb.h>
#include <regen/shapes/obb.h>
#include <regen/memory/aligned-array.h>
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
		 * @brief Set the base position of this sphere (without transformation)
		 * @param position The base position
		 */
		void setBasePosition(const Vec3f &position) { basePosition_ = position; }

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

		/**
		 * @brief Batch intersection test of this shape against a batch of spheres.
		 */
		static void batchTest_Spheres(BatchedIntersectionCase&);

		/**
		 * @brief Batch intersection test of this shape against a batch of AABBs.
		 */
		static void batchTest_AABBs(BatchedIntersectionCase&);

		/**
		 * @brief Batch intersection test of this shape against a batch of OBBs.
		 */
		static void batchTest_OBBs(BatchedIntersectionCase&);

		/**
		 * @brief Batch intersection test of this shape against a batch of frustums.
		 */
		static void batchTest_Frustums(BatchedIntersectionCase&);

	protected:
		Vec3f basePosition_;
		float radiusSquared_;

		float computeRadius(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts) const;

		void updateShapeOrigin();
	};

	/**
	 * @brief Shape traits for sphere shapes.
	 */
	template<> struct ShapeTraits<BoundingShapeType::SPHERE> {
		using BatchType = BatchOfSpheres;
		static constexpr auto NumSoAArrays = 4u; // posX, posY, posZ, radius
	};

	/**
	 * @brief Intersection traits for sphere shapes vs. frustum shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::SPHERE, BoundingShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_SPHERES;
		static constexpr auto Test = BoundingSphere::batchTest_Spheres;
	};

	/**
	 * @brief Intersection traits for sphere shapes vs. AABB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::SPHERE, BoundingShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_AABBs;
		static constexpr auto Test = BoundingSphere::batchTest_AABBs;
	};

	/**
	 * @brief Intersection traits for sphere shapes vs. OBB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::SPHERE, BoundingShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_OBBs;
		static constexpr auto Test = BoundingSphere::batchTest_OBBs;
	};

	/**
	 * @brief Intersection traits for sphere shapes vs. frustum shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::SPHERE, BoundingShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_FRUSTUMS;
		static constexpr auto Test = BoundingSphere::batchTest_Frustums;
	};

	/**
	 * @brief Batch intersection test of SPHERE shapes vs. sphere shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::SPHERE,
			BoundingShapeType::SPHERE>(BatchedIntersectionCase &td) {
		BoundingSphere::batchTest_Spheres(td);
	}

	/**
	 * @brief Batch intersection test of SPHERE shapes vs. AABB shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::SPHERE,
			BoundingShapeType::AABB>(BatchedIntersectionCase &td) {
		BoundingSphere::batchTest_AABBs(td);
	}

	/**
	 * @brief Batch intersection test of SPHERE shapes vs. OBB shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::SPHERE,
			BoundingShapeType::OBB>(BatchedIntersectionCase &td) {
		BoundingSphere::batchTest_OBBs(td);
	}

	/**
	 * @brief Batch intersection test of SPHERE shapes vs. frustum shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::SPHERE,
			BoundingShapeType::FRUSTUM>(BatchedIntersectionCase &td) {
		BoundingSphere::batchTest_Frustums(td);
	}

	/**
	 * @brief Intersection shape data for sphere shapes.
	 */
	struct IntersectionData_Sphere : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};
} // namespace regen

#endif /* REGEN_BOUNDING_SPHERE_H_ */
