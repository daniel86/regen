#include "orthogonal-projection.h"
#include "bounding-sphere.h"
#include "frustum.h"

using namespace regen;

template<int Count>
std::pair<float, float> project(const std::vector<Vec2f> &points, const Vec2f &axis) {
	std::array<float, Count> projections{};
	std::transform(points.begin(), points.end(), projections.begin(),
				   [&axis](const Vec2f &point) {
					   return point.dot(axis);
				   });
	auto [minIt, maxIt] = std::minmax_element(projections.begin(), projections.end());
	return {*minIt, *maxIt};
}

OrthogonalProjection::OrthogonalProjection(const BoundingShape &shape) {
	update(shape);
}

void OrthogonalProjection::update(const BoundingShape &shape) {
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
			type = OrthogonalProjection::Type::RECTANGLE;
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
			if (frustum->fov > 0.0) {
				// TODO: Make a nicely fitting 2D projection of frustum if possible.
				//       But triangle is not always possible, i.e. for up and down directions.
				//       Better use a trapazoid as a general solution, then simplify to triangle if two points are equal.
				//       For now just use a rectangle, which is computed for parallel projections too.
				makePerspectiveProjection(*frustum);
				//makeParallelProjection(*frustum);
			} else {
				makeParallelProjection(*frustum);
			}
			break;
		}
	}
}

void OrthogonalProjection::makeParallelProjection(const Frustum &frustum) {
	// approximate the frustum with a rectangle.
	// To this end start with the 4 corners of the far plane,
	// and then extend the rectangle to include the near plane.
	// This should be a perfect fit in case of parallel projections.
	type = OrthogonalProjection::Type::RECTANGLE;
	auto *planePoints = frustum.points;
	points.resize(4);
	// the center points of near/far plane projection
	Vec2f nearCenter_2D(0.0f), farCenter_2D(0.0f);
	// the near plane points projected to the xz-plane
	std::array<Vec2f, 4> nearPoints_2D;
	for (int i = 0; i < 4; i++) {
		// initialize with the far plane points
		points[i] = Vec2f(planePoints[i + 4].x, planePoints[i + 4].z);
		// project the near plane points
		nearPoints_2D[i] = Vec2f(planePoints[i].x, planePoints[i].z);
		farCenter_2D += points[i];
		nearCenter_2D += nearPoints_2D[i];
	}
	farCenter_2D /= 4.0f;
	nearCenter_2D /= 4.0f;
	// distance between 2D projections of near/far plane centers
	auto centerDistance = (farCenter_2D - nearCenter_2D).length();
	// next find the two near plane points that are the farthest away from the far plane center
	std::array<std::pair<float, int>, 4> distances;
	for (int i = 0; i < 4; ++i) {
		distances[i] = {(nearPoints_2D[i] - farCenter_2D).length(), i};
	}
	std::sort(distances.begin(), distances.end(), [](const auto &a, const auto &b) {
		return a.first < b.first;
	});
	auto x_i1 = std::min(distances[0].second, distances[1].second);
	auto x_i2 = std::max(distances[0].second, distances[1].second);
	if (x_i1 == 0) {
		auto buf = x_i1;
		x_i1 = x_i2;
		x_i2 = buf;
	}
	// x_i3/4 are the indices where the corner points of far plane must be adjusted
	auto x_i4 = (x_i1 - 1) % 4;
	auto x_i3 = (x_i2 + 1) % 4;
	// edge length of near plane in 2D
	auto near_d1 = (nearPoints_2D[x_i3] - nearPoints_2D[x_i1]).length();
	// edge length of far plane in 2D
	auto far_d1 = (points[x_i3] - points[x_i1]).length();
	// compute adjusted edge length of the rectangle
	auto edgeLength = std::max(far_d1, centerDistance + (far_d1 + near_d1) * 0.5f);
	// finally scale the rectangle to include the near plane
	auto dir = (points[x_i3] - points[x_i1]).normalize();
	points[x_i4] = points[x_i1] + dir * edgeLength;
	points[x_i3] = points[x_i2] + dir * edgeLength;
	// axes of the rectangle
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
}

void OrthogonalProjection::makePerspectiveProjection(const Frustum &frustum) {
	type = OrthogonalProjection::Type::TRIANGLE;
	points.resize(3);
	// first point: origin of the frustum
	auto basePoint = frustum.translation();
	auto *farPlanePoints = frustum.points;
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
