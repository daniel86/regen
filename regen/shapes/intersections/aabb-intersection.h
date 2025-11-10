#ifndef REGEN_AABB_INTERSECTION_H_
#define REGEN_AABB_INTERSECTION_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/intersections/batched-intersection.h>
#include <regen/utility/aligned-array.h>
#include <regen/math/simd.h>

namespace regen {
	struct BatchIntersection_AABB_Spheres : BatchedIntersectionCase {
		BatchIntersection_AABB_Spheres() = default;
		~BatchIntersection_AABB_Spheres() override = default;
	};

	struct BatchIntersection_AABB_AABBs : BatchedIntersectionCase {
		BatchIntersection_AABB_AABBs() = default;
		~BatchIntersection_AABB_AABBs() override = default;
	};

	struct BatchIntersection_AABB_OBBs : BatchedIntersectionCase {
		BatchIntersection_AABB_OBBs() = default;
		~BatchIntersection_AABB_OBBs() override = default;
	};

	struct BatchIntersection_AABB_Frustums : BatchedIntersectionCase {
		BatchIntersection_AABB_Frustums() = default;
		~BatchIntersection_AABB_Frustums() override = default;
	};

	namespace shapes {
		void flush_AABB_Spheres(BatchedIntersectionCase&);
		void flush_AABB_AABBs(BatchedIntersectionCase&);
		void flush_AABB_OBBs(BatchedIntersectionCase&);
		void flush_AABB_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<IntersectionShapeType::AABB, IntersectionShapeType::SPHERE> {
		using CaseType = BatchIntersection_AABB_Spheres;
		static constexpr auto Case = IntersectionCaseType::AABB_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_AABB_Spheres;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::AABB, IntersectionShapeType::AABB> {
		using CaseType = BatchIntersection_AABB_AABBs;
		static constexpr auto Case = IntersectionCaseType::AABB_AABBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_AABB_AABBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::AABB, IntersectionShapeType::OBB> {
		using CaseType = BatchIntersection_AABB_OBBs;
		static constexpr auto Case = IntersectionCaseType::AABB_OBBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_AABB_OBBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::AABB, IntersectionShapeType::FRUSTUM> {
		using CaseType = BatchIntersection_AABB_Frustums;
		static constexpr auto Case = IntersectionCaseType::AABB_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_AABB_Frustums;
	};
} // namespace

#endif /* REGEN_AABB_INTERSECTION_H_ */
