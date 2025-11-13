#ifndef REGEN_AABB_INTERSECTION_H_
#define REGEN_AABB_INTERSECTION_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/intersections/batched-intersection.h>

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
} // namespace

#endif /* REGEN_AABB_INTERSECTION_H_ */
