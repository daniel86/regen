#ifndef REGEN_ORTHOGONAL_PROJECTION_H
#define REGEN_ORTHOGONAL_PROJECTION_H

#include <regen/shapes/bounds.h>

namespace regen {
	// forward declaration
	class BoundingShape;

	/**
	 * Orthogonal projection of a bounding shape.
	 * The projection is a 2D shape that represents the bounding shape in 2D space on yz-plane.
	 */
	struct OrthogonalProjection {
		/**
		 * Shape of the projection.
		 */
		enum Type {
			CIRCLE = 0,
			RECTANGLE,
			TRIANGLE,
			CONVEX_HULL
		};

		explicit OrthogonalProjection(const BoundingShape &shape);

		Type type = Type::CIRCLE;
		std::vector<Vec2f> points;
		struct Axis {
			explicit Axis(const Vec2f &dir) : dir(dir) {}
			Vec2f dir;
			float min = 0.0f;
			float max = 0.0f;
		};
		std::vector<Axis> axes;
		Bounds<Vec2f> bounds;

		void update(const BoundingShape &shape);

	protected:
		std::vector<Vec2f> tmpPoints;

		void createConvexHull(const Vec3f *inputPoints, uint32_t numPoints);
	};
}

#include <regen/shapes/bounding-shape.h>

#endif //REGEN_ORTHOGONAL_PROJECTION_H
