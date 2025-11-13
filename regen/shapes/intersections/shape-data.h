#ifndef REGEN_INTERSECTION_SHAPE_DATA_H_
#define REGEN_INTERSECTION_SHAPE_DATA_H_

#include "regen/shapes/frustum.h"

namespace regen {
	/**
	 * @brief Base structure for intersection shape data used in batched intersection tests.
	 * This shape data is fixed during the test, i.e. we check one shape against many indexed shapes.
	 * IntersectionShapeData used to store some precomputed data for the test shape to speed up intersection tests.
	 */
	struct IntersectionShapeData {
		IntersectionShapeData() = default;
		virtual ~IntersectionShapeData() = default;
	};

	/**
	 * @brief Intersection shape data for sphere shapes.
	 */
	struct IntersectionData_Sphere : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};

	/**
	 * @brief Intersection shape data for axis-aligned bounding box (AABB) shapes.
	 */
	struct IntersectionData_AABB : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};

	/**
	 * @brief Intersection shape data for oriented bounding box (OBB) shapes.
	 */
	struct IntersectionData_OBB : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};

	/**
	 * @brief Intersection shape data for frustum shapes.
	 */
	struct IntersectionData_Frustum : IntersectionShapeData {
		std::array<Vec4f,6> planes;
		void update(const BoundingShape &shape) {
			const auto &frustum = static_cast<const Frustum &>(shape);
			for (size_t i = 0; i < 6; ++i) {
				planes[i] = frustum.planes[i].coefficients;
			}
		}
	};
} // namespace

#endif /* REGEN_INTERSECTION_SHAPE_DATA_H_ */
