#ifndef REGEN_AABB_H_
#define REGEN_AABB_H_

#include <regen/shapes/bounding-box.h>
#include <regen/utility/aligned-array.h>
#include "batch-of-shapes.h"

namespace regen {
	/**
	 * @brief Batch structure for axis-aligned bounding box (AABB) shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple AABBs in a batched manner.
	 */
	struct BatchOfAABBs : BatchOfShapes {
		BatchOfAABBs() : BatchOfShapes() {
			resizeFun = &BatchOfAABBs::doResize;
			pushFun = &BatchOfAABBs::doPush;
		}
		~BatchOfAABBs() override = default;
		// Queued AABB min/max points
		AlignedArray<float> minX;
		AlignedArray<float> minY;
		AlignedArray<float> minZ;
		AlignedArray<float> maxX;
		AlignedArray<float> maxY;
		AlignedArray<float> maxZ;

	protected:
		static void doResize(BatchOfShapes &self, uint32_t newCapacity, bool preserveData);
		static void doPush(BatchOfShapes &self, const BoundingShape &shape, uint32_t index);
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
			globalBatchData_.minX[globalIndex_],
			globalBatchData_.minY[globalIndex_],
			globalBatchData_.minZ[globalIndex_]); }

		/**
		 * @brief Get the transformed maximum bounds of the AABB
		 * @return The transformed maximum bounds
		 */
		Vec3f tfMaxBounds() const { return Vec3f(
			globalBatchData_.maxX[globalIndex_],
			globalBatchData_.maxY[globalIndex_],
			globalBatchData_.maxZ[globalIndex_]); }

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

	protected:
		void updateAABB();

		void setVertices();
	};
} // namespace

#include "batched-intersection.h"

namespace regen {
	namespace shapes {
		void flush_AABB_Spheres(BatchedIntersectionCase&);
		void flush_AABB_AABBs(BatchedIntersectionCase&);
		void flush_AABB_OBBs(BatchedIntersectionCase&);
		void flush_AABB_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<BoundingShapeType::AABB, BoundingShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::AABB_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_AABB_Spheres;
	};

	template<> struct IntersectionTraits<BoundingShapeType::AABB, BoundingShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::AABB_AABBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_AABB_AABBs;
	};

	template<> struct IntersectionTraits<BoundingShapeType::AABB, BoundingShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::AABB_OBBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_AABB_OBBs;
	};

	template<> struct IntersectionTraits<BoundingShapeType::AABB, BoundingShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::AABB_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_AABB_Frustums;
	};

	/**
	 * @brief Intersection shape data for axis-aligned bounding box (AABB) shapes.
	 */
	struct IntersectionData_AABB : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};
} // namespace

#endif /* REGEN_AABB_H_ */
