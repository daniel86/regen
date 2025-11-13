#ifndef REGEN_FRUSTUM_INTERSECTION_H_
#define REGEN_FRUSTUM_INTERSECTION_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/intersections/batched-intersection.h>

namespace regen {
	namespace shapes {
		void flush_Frustum_Spheres(BatchedIntersectionCase&);
		void flush_Frustum_AABBs(BatchedIntersectionCase&);
		void flush_Frustum_OBBs(BatchedIntersectionCase&);
		void flush_Frustum_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<IntersectionShapeType::FRUSTUM, IntersectionShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Frustum_Spheres;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::FRUSTUM, IntersectionShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_AABBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Frustum_AABBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::FRUSTUM, IntersectionShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_OBBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Frustum_OBBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::FRUSTUM, IntersectionShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Frustum_Frustums;
	};
} // namespace

#endif /* REGEN_FRUSTUM_INTERSECTION_H_ */
