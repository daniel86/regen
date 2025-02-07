#ifndef REGEN_ORTHOGONAL_PROJECTION_H
#define REGEN_ORTHOGONAL_PROJECTION_H

#include <regen/shapes/bounds.h>
#include <regen/shapes/bounding-shape.h>
#include "frustum.h"

namespace regen {
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
			TRIANGLE
		};

		explicit OrthogonalProjection(const BoundingShape &shape);

		/**
		 * Get the bounds of the projection.
		 * @return The bounds
		 */
		Bounds<Vec2f> bounds() const;

		Type type;
		std::vector<Vec2f> points;
		struct Axis {
			explicit Axis(const Vec2f &dir) : dir(dir) {}
			Vec2f dir;
			float min;
			float max;
		};
		std::vector<Axis> axes;

	protected:
		void makePerspectiveProjection(const Frustum &frustum);
		void makeParallelProjection(const Frustum &frustum);
	};
}

#endif //REGEN_ORTHOGONAL_PROJECTION_H
