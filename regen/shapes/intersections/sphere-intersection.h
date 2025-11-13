#ifndef REGEN_SPHERE_INTERSECTION_H_
#define REGEN_SPHERE_INTERSECTION_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/intersections/batched-intersection.h>
#include <regen/utility/aligned-array.h>
#include <regen/math/simd.h>

namespace regen {
	struct BatchIntersection_Sphere_Spheres : BatchedIntersectionCase {
		BatchIntersection_Sphere_Spheres() = default;
		~BatchIntersection_Sphere_Spheres() override = default;
		BatchOf_float batch_spherePosX;
		BatchOf_float batch_spherePosY;
		BatchOf_float batch_spherePosZ;
		BatchOf_float batch_sphereRadius;
	};

	struct BatchIntersection_Sphere_AABBs : BatchedIntersectionCase {
		BatchIntersection_Sphere_AABBs() = default;
		~BatchIntersection_Sphere_AABBs() override = default;
		BatchOf_float batch_aabbMinX;;
		BatchOf_float batch_aabbMinY;
		BatchOf_float batch_aabbMinZ;
		BatchOf_float batch_aabbMaxX;;
		BatchOf_float batch_aabbMaxY;
		BatchOf_float batch_aabbMaxZ;
	};

	struct BatchIntersection_Sphere_OBBs : BatchedIntersectionCase {
		BatchIntersection_Sphere_OBBs() = default;
		~BatchIntersection_Sphere_OBBs() override = default;
		BatchOf_float batch_obbCenterX;
		BatchOf_float batch_obbCenterY;
		BatchOf_float batch_obbCenterZ;
		BatchOf_float batch_obbHalfSize[3];
	};

	struct BatchIntersection_Sphere_Frustums : BatchedIntersectionCase {
		BatchIntersection_Sphere_Frustums() = default;
		~BatchIntersection_Sphere_Frustums() override = default;
	};

	namespace shapes {
		void flush_Sphere_Spheres(BatchedIntersectionCase&);
		void flush_Sphere_AABBs(BatchedIntersectionCase&);
		void flush_Sphere_OBBs(BatchedIntersectionCase&);
		void flush_Sphere_Frustums(BatchedIntersectionCase&);
	}

	template<> struct IntersectionTraits<IntersectionShapeType::SPHERE, IntersectionShapeType::SPHERE> {
		using CaseType = BatchIntersection_Sphere_Spheres;
		static constexpr auto Case = IntersectionCaseType::SPHERE_SPHERES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_Spheres;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::SPHERE, IntersectionShapeType::AABB> {
		using CaseType = BatchIntersection_Sphere_AABBs;
		static constexpr auto Case = IntersectionCaseType::SPHERE_BOXES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_AABBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::SPHERE, IntersectionShapeType::OBB> {
		using CaseType = BatchIntersection_Sphere_OBBs;
		static constexpr auto Case = IntersectionCaseType::SPHERE_BOXES;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_OBBs;
	};

	template<> struct IntersectionTraits<IntersectionShapeType::SPHERE, IntersectionShapeType::FRUSTUM> {
		using CaseType = BatchIntersection_Sphere_Frustums;
		static constexpr auto Case = IntersectionCaseType::SPHERE_FRUSTUMS;
		static constexpr auto Init = BatchedIntersectionCase::case_NOOP;
		static constexpr auto Flush = shapes::flush_Sphere_Frustums;
	};
} // namespace

#endif /* REGEN_SPHERE_INTERSECTION_H_ */
