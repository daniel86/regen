#ifndef REGEN_AABB_H_
#define REGEN_AABB_H_

#include <regen/shapes/bounding-box.h>
#include <regen/utility/aligned-array.h>
#include "batch-of-shapes.h"
#include "batched-intersection.h"

namespace regen {
	/**
	 * @brief Batch structure for axis-aligned bounding box (AABB) shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple AABBs in a batched manner.
	 */
	struct BatchOfAABBs : BatchOfShapes {
		BatchOfAABBs() : BatchOfShapes(6) {}
		~BatchOfAABBs() override = default;
		// Queued AABB min/max points
		AlignedArray<float>& minX() { return soaData_[0]; }
		AlignedArray<float>& minY() { return soaData_[1]; }
		AlignedArray<float>& minZ() { return soaData_[2]; }
		AlignedArray<float>& maxX() { return soaData_[3]; }
		AlignedArray<float>& maxY() { return soaData_[4]; }
		AlignedArray<float>& maxZ() { return soaData_[5]; }

		const AlignedArray<float>& minX() const { return soaData_[0]; }
		const AlignedArray<float>& minY() const { return soaData_[1]; }
		const AlignedArray<float>& minZ() const { return soaData_[2]; }
		const AlignedArray<float>& maxX() const { return soaData_[3]; }
		const AlignedArray<float>& maxY() const { return soaData_[4]; }
		const AlignedArray<float>& maxZ() const { return soaData_[5]; }
	};

	/**
	 * @brief Axis-aligned bounding box
	 */
	class AABB : public BoundingBox<BatchOfAABBs> {
	public:
		/**
		 * @brief Construct a new AABB object
		 * @param mesh The mesh
		 */
		AABB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts);

		/**
		 * @brief Construct a new AABB object
		 * @param bounds The min/max of the AABB
		 */
		explicit AABB(const Bounds<Vec3f> &bounds);

		~AABB() override = default;

		/**
		 * @brief Get the transformed minimum bounds of the AABB
		 * @return The transformed minimum bounds
		 */
		Vec3f tfMinBounds() const { return Vec3f(
			globalBatchData_.minX()[globalIndex_],
			globalBatchData_.minY()[globalIndex_],
			globalBatchData_.minZ()[globalIndex_]); }

		/**
		 * @brief Get the transformed maximum bounds of the AABB
		 * @return The transformed maximum bounds
		 */
		Vec3f tfMaxBounds() const { return Vec3f(
			globalBatchData_.maxX()[globalIndex_],
			globalBatchData_.maxY()[globalIndex_],
			globalBatchData_.maxZ()[globalIndex_]); }

		/**
		 * @brief Get the box axes (constant for AABB)
		 * @return The box axes
		 */
		const Vec3f *boxAxes() const {
			static const Vec3f aabb_axes[3] = { Vec3f::right(), Vec3f::up(), Vec3f::front() };
			return aabb_axes;
		}

		/**
		 * @brief Check if this AABB has intersection with another AABB
		 * @param other The other AABB
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithAABB(const AABB &other) const;

		// override BoundingBox::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// override BoundingBox::update
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
		void updateAABB();
	};

	/**
	 * @brief Shape traits for AABB shapes.
	 */
	template<> struct ShapeTraits<BoundingShapeType::AABB> {
		using BatchType = BatchOfAABBs;
		static constexpr auto NumSoAArrays = 6; // minX, minY, minZ, maxX, maxY, maxZ
	};

	/**
	 * @brief Intersection traits for AABB shapes vs. frustum shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::AABB, BoundingShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::AABB_SPHERES;
		static constexpr auto Test = AABB::batchTest_Spheres;
	};

	/**
	 * @brief Intersection traits for AABB shapes vs. AABB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::AABB, BoundingShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::AABB_AABBs;
		static constexpr auto Test = AABB::batchTest_AABBs;
	};

	/**
	 * @brief Intersection traits for AABB shapes vs. OBB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::AABB, BoundingShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::AABB_OBBs;
		static constexpr auto Test = AABB::batchTest_OBBs;
	};

	/**
	 * @brief Intersection traits for AABB shapes vs. frustum shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::AABB, BoundingShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::AABB_FRUSTUMS;
		static constexpr auto Test = AABB::batchTest_Frustums;
	};

	/**
	 * @brief Batch intersection test of AABB shapes vs. sphere shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::AABB,
			BoundingShapeType::SPHERE>(BatchedIntersectionCase &td) {
		AABB::batchTest_Spheres(td);
	}

	/**
	 * @brief Batch intersection test of AABB shapes vs. AABB shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::AABB,
			BoundingShapeType::AABB>(BatchedIntersectionCase &td) {
		AABB::batchTest_AABBs(td);
	}

	/**
	 * @brief Batch intersection test of AABB shapes vs. OBB shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::AABB,
			BoundingShapeType::OBB>(BatchedIntersectionCase &td) {
		AABB::batchTest_OBBs(td);
	}

	/**
	 * @brief Batch intersection test of AABB shapes vs. frustum shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::AABB,
			BoundingShapeType::FRUSTUM>(BatchedIntersectionCase &td) {
		AABB::batchTest_Frustums(td);
	}

	/**
	 * @brief Intersection shape data for axis-aligned bounding box (AABB) shapes.
	 */
	struct IntersectionData_AABB : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};
} // namespace regen

#endif /* REGEN_AABB_H_ */
