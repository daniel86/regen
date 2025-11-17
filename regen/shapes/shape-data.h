#ifndef REGEN_INTERSECTION_SHAPE_DATA_H_
#define REGEN_INTERSECTION_SHAPE_DATA_H_

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
} // namespace regen

#endif /* REGEN_INTERSECTION_SHAPE_DATA_H_ */
