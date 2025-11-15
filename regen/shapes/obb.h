#ifndef REGEN_OBB_H_
#define REGEN_OBB_H_

#include <regen/shapes/bounding-box.h>
#include <regen/shapes/aabb.h>
#include <regen/utility/aligned-array.h>
#include "batch-of-shapes.h"

namespace regen {
	/**
	 * @brief Batch structure for oriented bounding box (OBB) shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple OBBs in a batched manner.
	 */
	struct BatchOfOBBs : BatchOfShapes {
		// NOTE: Frustum shapes do not support batched intersection tests yet.
		BatchOfOBBs() : BatchOfShapes() {
			resizeFun = &BatchOfOBBs::doResize;
			pushFun = &BatchOfOBBs::doPush;
		}
		~BatchOfOBBs() override = default;
		AlignedArray<float> centerX;
		AlignedArray<float> centerY;
		AlignedArray<float> centerZ;
		AlignedArray<float> halfSizeX;
		AlignedArray<float> halfSizeY;
		AlignedArray<float> halfSizeZ;
		struct AxisBatch {
			AlignedArray<float> x;
			AlignedArray<float> y;
			AlignedArray<float> z;
		};
		std::array<AxisBatch, 3> axes;

	protected:
		static void doResize(BatchOfShapes&, uint32_t, bool);
		static void doPush(BatchOfShapes&, const BoundingShape&, uint32_t);
	};

	/**
	 * @brief Oriented bounding box
	 */
	class OBB : public BoundingBox<BatchOfOBBs> {
	public:
		/**
		 * @brief Construct a new OBB object
		 * @param mesh The mesh
		 */
		OBB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts);

		/**
		 * @brief Construct a new OBB object
		 * @param bounds The min/max of the OBB
		 */
		explicit OBB(const Bounds<Vec3f> &bounds);

		~OBB() override = default;

		/**
		 * @brief Get the box axes
		 * @return The box axes
		 */
		//const Vec3f *boxAxes() const { return boxAxes_; }

		/**
		 * @brief Check if this OBB has intersection with an AABB
		 * @param other The AABB
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithAABB(const AABB &other) const;

		/**
		 * @brief Check if this OBB has intersection with another OBB
		 * @param other The other OBB
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithOBB(const OBB &other) const;

		// override BoundingBox::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// BoundingShape interface
		void updateBaseBounds(const Vec3f &min, const Vec3f &max) override;

		// BoundingShape interface
		bool updateTransform(bool forceUpdate) final;

	protected:
		void updateOBB();
		void applyTransform(const Mat4f &tf);
	};
} // namespace

#include "batched-intersection.h"

namespace regen {
	namespace shapes {
		void flush_OBB_Spheres(BatchedIntersectionCase&);
		void flush_OBB_AABBs(BatchedIntersectionCase&);
		void flush_OBB_OBBs(BatchedIntersectionCase&);
		void flush_OBB_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<BoundingShapeType::OBB, BoundingShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::OBB_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_Spheres;
	};

	template<> struct IntersectionTraits<BoundingShapeType::OBB, BoundingShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::OBB_AABBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_AABBs;
	};

	template<> struct IntersectionTraits<BoundingShapeType::OBB, BoundingShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::OBB_OBBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_OBBs;
	};

	template<> struct IntersectionTraits<BoundingShapeType::OBB, BoundingShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::OBB_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_Frustums;
	};

	/**
	 * @brief Intersection shape data for oriented bounding box (OBB) shapes.
	 */
	struct IntersectionData_OBB : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};
} // namespace

#endif /* REGEN_OBB_H_ */
