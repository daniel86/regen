#ifndef REGEN_SPHERE_INTERSECTION_H_
#define REGEN_SPHERE_INTERSECTION_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/intersections/batched-intersection.h>

namespace regen {
	namespace shapes {
		void flush_Sphere_Spheres(BatchedIntersectionCase&);
		void flush_Sphere_AABBs(BatchedIntersectionCase&);
		void flush_Sphere_OBBs(BatchedIntersectionCase&);
		void flush_Sphere_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<IntersectionShapeType::SPHERE, IntersectionShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_Spheres;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::SPHERE, IntersectionShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_AABBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_AABBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::SPHERE, IntersectionShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_OBBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_OBBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::SPHERE, IntersectionShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::SPHERE_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_Frustums;
	};
} // namespace

#endif /* REGEN_SPHERE_INTERSECTION_H_ */
