#ifndef REGEN_FRUSTUM_INTERSECTION_H_
#define REGEN_FRUSTUM_INTERSECTION_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/intersections/batched-intersection.h>
#include <regen/math/simd.h>

namespace regen {
	struct BatchIntersection_Frustum_Spheres : BatchedIntersectionCase {
		BatchIntersection_Frustum_Spheres() = default;
		~BatchIntersection_Frustum_Spheres() override = default;
		BatchOf_float batch_spherePosX;
		BatchOf_float batch_spherePosY;
		BatchOf_float batch_spherePosZ;
		BatchOf_float batch_sphereRadius;
	};

	struct BatchIntersection_Frustum_AABBs : BatchedIntersectionCase {
		BatchIntersection_Frustum_AABBs() = default;
		~BatchIntersection_Frustum_AABBs() override = default;
		BatchOf_float batch_aabbMinX;;
		BatchOf_float batch_aabbMinY;
		BatchOf_float batch_aabbMinZ;
		BatchOf_float batch_aabbMaxX;;
		BatchOf_float batch_aabbMaxY;
		BatchOf_float batch_aabbMaxZ;
	};

	struct BatchIntersection_Frustum_OBBs : BatchedIntersectionCase {
		BatchIntersection_Frustum_OBBs() = default;
		~BatchIntersection_Frustum_OBBs() override = default;
		BatchOf_float batch_obbCenterX;
		BatchOf_float batch_obbCenterY;
		BatchOf_float batch_obbCenterZ;
		BatchOf_float batch_obbHalfSize[3];
	};

	struct BatchIntersection_Frustum_Frustums : BatchedIntersectionCase {
		BatchIntersection_Frustum_Frustums() = default;
		~BatchIntersection_Frustum_Frustums() override = default;
	};

	namespace shapes {
		void flush_Frustum_Spheres(BatchedIntersectionCase&);
		void flush_Frustum_AABBs(BatchedIntersectionCase&);
		void flush_Frustum_OBBs(BatchedIntersectionCase&);
		void flush_Frustum_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<IntersectionShapeType::FRUSTUM, IntersectionShapeType::SPHERE> {
		using CaseType = BatchIntersection_Frustum_Spheres;
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Frustum_Spheres;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::FRUSTUM, IntersectionShapeType::AABB> {
		using CaseType = BatchIntersection_Frustum_AABBs;
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_AABBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Frustum_AABBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::FRUSTUM, IntersectionShapeType::OBB> {
		using CaseType = BatchIntersection_Frustum_OBBs;
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_OBBs;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Frustum_OBBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::FRUSTUM, IntersectionShapeType::FRUSTUM> {
		using CaseType = BatchIntersection_Frustum_Frustums;
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Frustum_Frustums;
	};
} // namespace

#endif /* REGEN_FRUSTUM_INTERSECTION_H_ */
