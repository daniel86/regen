#ifndef REGEN_OBB_INTERSECTION_H_
#define REGEN_OBB_INTERSECTION_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/intersections/batched-intersection.h>
#include <regen/utility/aligned-array.h>
#include <regen/math/simd.h>

namespace regen {
	struct BatchIntersection_OBB_Spheres : BatchedIntersectionCase {
		BatchIntersection_OBB_Spheres() = default;
		~BatchIntersection_OBB_Spheres() override = default;
	};

	struct BatchIntersection_OBB_Boxes : BatchedIntersectionCase {
		BatchIntersection_OBB_Boxes() = default;
		~BatchIntersection_OBB_Boxes() override = default;
	};

	struct BatchIntersection_OBB_Frustums : BatchedIntersectionCase {
		BatchIntersection_OBB_Frustums() = default;
		~BatchIntersection_OBB_Frustums() override = default;
	};

	namespace shapes {
		void flush_OBB_Spheres(BatchedIntersectionCase&);
		void flush_OBB_Boxes(BatchedIntersectionCase&);
		void flush_OBB_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<IntersectionShapeType::OBB, IntersectionShapeType::SPHERE> {
		using CaseType = BatchIntersection_OBB_Spheres;
		static constexpr auto Case = IntersectionCaseType::OBB_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_Spheres;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::OBB, IntersectionShapeType::AABB> {
		using CaseType = BatchIntersection_OBB_Boxes;
		static constexpr auto Case = IntersectionCaseType::OBB_BOXES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_Boxes;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::OBB, IntersectionShapeType::OBB> {
		using CaseType = BatchIntersection_OBB_Boxes;
		static constexpr auto Case = IntersectionCaseType::OBB_BOXES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_Boxes;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::OBB, IntersectionShapeType::FRUSTUM> {
		using CaseType = BatchIntersection_OBB_Frustums;
		static constexpr auto Case = IntersectionCaseType::OBB_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_OBB_Frustums;
	};
} // namespace

#endif /* REGEN_OBB_INTERSECTION_H_ */
