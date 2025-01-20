#include "orthogonal-projection.h"
#include "bounding-sphere.h"
#include "frustum.h"

using namespace regen;

OrthogonalProjection::OrthogonalProjection(const BoundingShape &shape)
		: type(OrthogonalProjection::Type::RECTANGLE) {
	switch (shape.shapeType()) {
		case BoundingShapeType::SPHERE: {
			// sphere projection is a circle
			type = OrthogonalProjection::Type::CIRCLE;
			auto *sphere = dynamic_cast<const BoundingSphere *>(&shape);
			auto sphereCenter = sphere->getCenterPosition();
			points.resize(2);
			points[0] = Vec2f(sphereCenter.x, sphereCenter.z);
			// note: second point stores the squared radius
			points[1] = Vec2f(sphere->radius() * sphere->radius(), 0);
			break;
		}
		case BoundingShapeType::BOX: {
			// box projection is a rectangle
			auto *box = dynamic_cast<const BoundingBox *>(&shape);
			// generate the 4 corners of the box in the xz-plane.
			// for this just take min/max of the box vertices as bounds,
			// this is not a perfect fit but fast.
			auto *vertices_3D = box->boxVertices();
			Vec2f min(vertices_3D[0].x, vertices_3D[0].z);
			Vec2f max(vertices_3D[0].x, vertices_3D[0].z);
			for (int i = 1; i < 8; i++) {
				min.x = std::min(min.x, vertices_3D[i].x);
				min.y = std::min(min.y, vertices_3D[i].z);
				max.x = std::max(max.x, vertices_3D[i].x);
				max.y = std::max(max.y, vertices_3D[i].z);
			}
			points.resize(4);
			// bottom-left
			points[0] = min;
			// bottom-right
			points[1] = Vec2f(max.x, min.y);
			// top-right
			points[2] = max;
			// top-left
			points[3] = Vec2f(min.x, max.y);
			break;
		}
		case BoundingShapeType::FRUSTUM: {
			// frustum projection is a rectangle: we take origin as one point of the rectangle
			// and use points of far plane as the other 3 points.
			auto *frustum = dynamic_cast<const Frustum *>(&shape);
			auto &planePoints = frustum->points;

//#define FRUSTUM_PROJECTION_TRIANGLE
#ifdef FRUSTUM_PROJECTION_TRIANGLE
			type = OrthogonalProjection::Type::TRIANGLE;
			auto &basePoint = frustum->basePosition();
			// Find the two points closest to the base point
			std::vector<std::pair<float, Vec3f>> distances;
			for (const auto &point: planePoints) {
				float distance = (point - basePoint).lengthSquared();
				distances.emplace_back(distance, point);
			}
			std::sort(distances.begin(), distances.end());
			Vec3f &nearest1 = distances[0].second;
			Vec3f &nearest2 = distances[1].second;
			Vec3f &farthest1 = distances[2].second;
			Vec3f &farthest2 = distances[3].second;
			Vec3f farthest_center = (farthest1 + farthest2) * 0.5f;
			// Compute direction vectors from base to nearest points
			Vec3f dir1 = nearest1 - basePoint;
			Vec3f dir2 = nearest2 - basePoint;
			dir1.normalize();
			dir2.normalize();
			// Solve for the lengths
			// TODO: I do not think below is accurate, the actual length could be different.
			//       IMO we need to solve equations for finding the length where the direction vectors intersect
			//       with (farthest_N - farthest_center) line.
			float length1 = (farthest1 - basePoint).length();
			float length2 = (farthest2 - basePoint).length();
			// Normalize direction vectors and scale to the computed lengths
			dir1 *= length1;
			dir2 *= length2;
			// Define the triangle points
			points.resize(3);
			points[0] = Vec2f(basePoint.x, basePoint.z);
			points[1] = Vec2f(basePoint.x + dir1.x, basePoint.z + dir1.z);
			points[2] = Vec2f(basePoint.x + dir2.x, basePoint.z + dir2.z);
#else
			// generate AABB around the frustum
			Vec2f min(planePoints[0].x, planePoints[0].z);
			Vec2f max(planePoints[0].x, planePoints[0].z);
			for (int i = 1; i < 8; i++) {
				min.x = std::min(min.x, planePoints[i].x);
				min.y = std::min(min.y, planePoints[i].z);
				max.x = std::max(max.x, planePoints[i].x);
				max.y = std::max(max.y, planePoints[i].z);
			}
			points.resize(4);
			// bottom-left
			points[0] = min;
			// bottom-right
			points[1] = Vec2f(max.x, min.y);
			// top-right
			points[2] = max;
			// top-left
			points[3] = Vec2f(min.x, max.y);
#endif
			break;
		}
	}
}

Bounds<Vec2f> OrthogonalProjection::bounds() const {
	Bounds<Vec2f> targetBounds(points[0], points[0]);
	if (type == OrthogonalProjection::Type::CIRCLE) {
		auto &center = points[0];
		auto radius = std::sqrt(points[1].x);
		targetBounds.min.x = center.x - radius;
		targetBounds.min.y = center.y - radius;
		targetBounds.max.x = center.x + radius;
		targetBounds.max.y = center.y + radius;
	} else {
		for (size_t i = 1; i < points.size(); i++) {
			targetBounds.min.x = std::min(targetBounds.min.x, points[i].x);
			targetBounds.min.y = std::min(targetBounds.min.y, points[i].y);
			targetBounds.max.x = std::max(targetBounds.max.x, points[i].x);
			targetBounds.max.y = std::max(targetBounds.max.y, points[i].y);
		}
	}
	return targetBounds;
}
