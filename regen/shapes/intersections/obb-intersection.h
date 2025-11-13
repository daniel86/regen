#ifndef REGEN_OBB_INTERSECTION_H_
#define REGEN_OBB_INTERSECTION_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/intersections/batched-intersection.h>

namespace regen {
	namespace shapes {
		void flush_OBB_Spheres(BatchedIntersectionCase&);
		void flush_OBB_AABBs(BatchedIntersectionCase&);
		void flush_OBB_OBBs(BatchedIntersectionCase&);
		void flush_OBB_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<IntersectionShapeType::OBB, IntersectionShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::OBB_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_Spheres;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::OBB, IntersectionShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::OBB_AABBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_AABBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::OBB, IntersectionShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::OBB_OBBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_OBBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::OBB, IntersectionShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::OBB_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_Frustums;
	};
} // namespace

#endif /* REGEN_OBB_INTERSECTION_H_ */
