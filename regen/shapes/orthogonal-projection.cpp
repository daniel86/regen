#include "orthogonal-projection.h"
#include "bounding-sphere.h"
#include "frustum.h"

using namespace regen;

template <int Count>
std::pair<float, float> project(const std::vector<Vec2f> &points, const Vec2f &axis) {
	std::array<float, Count> projections;
	std::transform(points.begin(), points.end(), projections.begin(),
		[&axis](const Vec2f &point) {
			return point.dot(axis);
		});
	auto [minIt, maxIt] = std::minmax_element(projections.begin(), projections.end());
	return {*minIt, *maxIt};
}

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
			// axes of the rectangle (and quad)
			axes = {
					Axis(Vec2f(1, 0)),
					Axis(Vec2f(0, 1)),
					Axis(points[1] - points[0]),
					Axis(points[3] - points[0])
			};
			for (auto &axis: axes) {
				auto [axisMin, axisMax] = project<4>(points, axis.dir);
				axis.min = axisMin;
				axis.max = axisMax;
			}
			break;
		}
		case BoundingShapeType::FRUSTUM: {
			auto *frustum = dynamic_cast<const Frustum *>(&shape);
			type = OrthogonalProjection::Type::TRIANGLE;
			points.resize(3);
			auto &viewDir = frustum->direction();
			// FIXME: this is not entirely accurate.
			// first point: origin of the frustum
			auto basePoint = frustum->translation();
			auto *farPlanePoints = frustum->points;
			points[0] = Vec2f(basePoint.x, basePoint.z);
			// Project far plane points onto the xz plane
			std::array<Vec2f, 4> farPoints2D;
			Vec2f farPlaneCenter2D;
			for (int i = 0; i < 4; ++i) {
				farPoints2D[i] = Vec2f(farPlanePoints[i + 4].x, farPlanePoints[i + 4].z);
				farPlaneCenter2D += farPoints2D[i];
			}
			farPlaneCenter2D *= 0.25f; // Center of all far frustum points
			auto baseToCenter = farPlaneCenter2D - points[0];
			// Compute the angles relative to the center
			std::array<std::pair<float, int>, 4> angles;
			for (int i = 0; i < 4; ++i) {
				auto baseToPt = farPoints2D[i] - points[0];
				angles[i] = {std::acos(
					baseToCenter.dot(baseToPt) /
					(baseToCenter.length() * baseToPt.length())), i};
			}
			// Sort the angles to find the maximum
			std::sort(angles.begin(), angles.end(), [](const auto &a, const auto &b) {
				return a.first < b.first;
			});
			points[1] = farPoints2D[angles[0].second];
			points[2] = farPoints2D[angles[1].second];
			if ((points[1] - points[2]).length() < std::numeric_limits<float>::epsilon()) {
				// If the two points are the same, switch the last two point
				points[2] = farPoints2D[angles[2].second];
			}

			// axes of the triangle (and quad)
			axes = {
					Axis(Vec2f(1, 0)),
					Axis(Vec2f(0, 1)),
					Axis(points[1] - points[0]),
					Axis(points[2] - points[1]),
					Axis(points[0] - points[2])
			};
			for (auto &axis: axes) {
				auto [axisMin, axisMax] = project<3>(points, axis.dir);
				axis.min = axisMin;
				axis.max = axisMax;
			}
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
