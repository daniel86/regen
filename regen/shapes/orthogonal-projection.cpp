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
			auto viewDir2D = Vec2f(viewDir.x, viewDir.z);
			// first point: origin of the frustum
			auto basePoint = frustum->translation();
			points[0] = Vec2f(basePoint.x, basePoint.z);
			// Convert FOV from degrees to radians, and multiply by 0.5 to get the half-angle
			auto halfFov = static_cast<float>(frustum->fov * 0.00872665);
			// Project the far plane onto the xz-plane
			auto far = static_cast<float>(frustum->far) * viewDir2D.length();
			// Compute the far points by extending the base point in the right and left directions
			points[1] = points[0] + Vec2f(
				viewDir.x * cos(halfFov) + viewDir.z * sin(halfFov),
				-viewDir.x * sin(halfFov) + viewDir.z * cos(halfFov)
			) * far;
			points[2] = points[0] + Vec2f(
				viewDir.x * cos(-halfFov) + viewDir.z * sin(-halfFov),
				-viewDir.x * sin(-halfFov) + viewDir.z * cos(-halfFov)
			) * far;
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
