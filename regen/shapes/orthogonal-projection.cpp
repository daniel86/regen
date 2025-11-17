#include "orthogonal-projection.h"

#include "aabb.h"
#include "bounding-sphere.h"
#include "frustum.h"
#include "obb.h"

#define FRUSTUM_USE_CONVEX_HULL

using namespace regen;

static inline void project(const std::vector<Vec2f> &points, OrthogonalProjection::Axis &axis) {
	float mn = std::numeric_limits<float>::infinity();
	float mx = -std::numeric_limits<float>::infinity();
	for (const auto &p : points) {
		float v = p.dot(axis.dir);
		if (v < mn) mn = v;
		if (v > mx) mx = v;
	}
	axis.min = mn;
	axis.max = mx;
}

static inline Vec2f perpendicular(const Vec2f &v) {
	Vec2f x(-v.y, v.x);
	//x.normalize();
	return x;
}

OrthogonalProjection::OrthogonalProjection(const BoundingShape &shape)
		: bounds(Bounds<Vec2f>::create(0.0f,0.0f)) {
	switch (shape.shapeType()) {
		case BoundingShapeType::SPHERE:
			points.resize(2);
		case BoundingShapeType::AABB:
		case BoundingShapeType::OBB:
			axes.resize(4, Axis(Vec2f(0,0)));
			axes[0].dir = Vec2f(1, 0);
			axes[1].dir = Vec2f(0, 1);
		default:
			break;
	}
	update(shape);
}

void OrthogonalProjection::update(const BoundingShape &shape) {
	switch (shape.shapeType()) {
		case BoundingShapeType::SPHERE: {
			// sphere projection is a circle
			type = OrthogonalProjection::Type::CIRCLE;
			auto *sphere = static_cast<const BoundingSphere *>(&shape);
			auto &batchData = sphere->globalBatchData_t();
			const uint32_t batchIdx = sphere->globalIndex();
			const float radius = batchData.radius()[batchIdx];
			points[0].x = batchData.posX()[batchIdx];;
			points[0].y = batchData.posZ()[batchIdx];
			// note: second point stores the squared radius
			points[1].x = radius*radius;
			break;
		}
		case BoundingShapeType::AABB: {
			createRectangle_AABB(shape);
			break;
		}
		case BoundingShapeType::OBB: {
			createRectangle_OBB(shape);
			break;
		}
		case BoundingShapeType::FRUSTUM: {
			auto *frustum = static_cast<const Frustum *>(&shape);
			// use the convex hull of the frustum points as projection
			createConvexHull(frustum->points, 8);
			break;
		}
		case BoundingShapeType::LAST: {
			return;
		}
	}

	// compute the bounds of the projection
	bounds.min = points[0];
	bounds.max = points[0];
	if (type == OrthogonalProjection::Type::CIRCLE) {
		auto &center = points[0];
		auto radius = std::sqrt(points[1].x);
		bounds.min.x = center.x - radius;
		bounds.min.y = center.y - radius;
		bounds.max.x = center.x + radius;
		bounds.max.y = center.y + radius;
	} else {
		for (size_t i = 1; i < points.size(); i++) {
			bounds.min.x = std::min(bounds.min.x, points[i].x);
			bounds.min.y = std::min(bounds.min.y, points[i].y);
			bounds.max.x = std::max(bounds.max.x, points[i].x);
			bounds.max.y = std::max(bounds.max.y, points[i].y);
		}
	}
}

// Helper for cross product (2D, z-component)
static inline float cross(const Vec2f& O, const Vec2f& A, const Vec2f& B) {
	return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

void OrthogonalProjection::createRectangle_AABB(const BoundingShape &box) {
	auto &aabb = static_cast<const AABB&>(box);
	auto &batchData = aabb.globalBatchData_t();
	const uint32_t batchIdx = aabb.globalIndex();
	type = RECTANGLE;
	// generate the 4 corners of the box in the xz-plane.
	points.resize(4);
	// bottom-left
	points[0].x = batchData.minX()[batchIdx];
	points[0].y = batchData.minZ()[batchIdx];
	// bottom-right
	points[1].x = batchData.maxX()[batchIdx];
	points[1].y = batchData.minZ()[batchIdx];
	// top-right
	points[2].x = batchData.maxX()[batchIdx];
	points[2].y = batchData.maxZ()[batchIdx];
	// top-left
	points[3].x = batchData.minX()[batchIdx];
	points[3].y = batchData.maxZ()[batchIdx];
	// axes of the rectangle (and quad)
	axes[2].dir = perpendicular(points[1] - points[0]);
	axes[3].dir = perpendicular(points[3] - points[0]);
	// project along each axis
	for (auto &axis: axes) {
		project(points, axis);
	}
}

void OrthogonalProjection::createRectangle_OBB(const BoundingShape &obb) {
	auto *vertices_3D = static_cast<const OBB&>(obb).boxVertices();
	type = RECTANGLE;
	// generate the 4 corners of the box in the xz-plane.
	// for this just take min/max of the box vertices as bounds,
	// this is not a perfect fit but fast.
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
	axes[2].dir = perpendicular(points[1] - points[0]);
	axes[3].dir = perpendicular(points[3] - points[0]);
	// project along each axis
	for (auto &axis: axes) {
		project(points, axis);
	}
}

void OrthogonalProjection::createConvexHull(const Vec3f *inputPoints, uint32_t numPoints) {
	type = OrthogonalProjection::Type::CONVEX_HULL;

	// Project into 2D (xz-plane) and sort lexicographically (x, then y)
	if (tmpPoints.size() != numPoints) {
		tmpPoints.resize(numPoints);
	}
	for (uint32_t i = 0; i < numPoints; ++i) {
		tmpPoints[i].x = inputPoints[i].x;
		tmpPoints[i].y = inputPoints[i].z;
	}
	std::sort(tmpPoints.begin(), tmpPoints.end(), [](const Vec2f& a, const Vec2f& b) {
		return (a.x < b.x) || (a.x == b.x && a.y < b.y);
	});

	std::vector<Vec2f> hull;
	hull.reserve(numPoints * 2);
	// Build lower hull
	for (const auto& p : tmpPoints) {
		while (hull.size() >= 2 && cross(hull[hull.size()-2], hull.back(), p) <= 0) {
			hull.pop_back();
		}
		hull.push_back(p);
	}
	size_t lowerSize = hull.size();
	// Build upper hull (skip first and last point)
	for (int i = int(tmpPoints.size()) - 2; i >= 0; --i) {
		const auto& p = tmpPoints[i];
		while (hull.size() > lowerSize && cross(hull[hull.size()-2], hull.back(), p) <= 0) {
			hull.pop_back();
		}
		hull.push_back(p);
	}
	if (!hull.empty()) hull.pop_back();

	// Save result + Compute SAT axes from polygon edges
	points = hull;
	axes.clear();
	axes.reserve(points.size() + 2); // +2 for world axes
	size_t N = points.size();
	for (size_t i = 0; i < N; ++i) {
		Vec2f edge = points[(i + 1) % N] - points[i];
		if (edge.lengthSquared() < 1e-8f) continue; // skip degenerate edges
		// Note: normalization is not strictly necessary for SAT
		axes.emplace_back(Vec2f(-edge.y, edge.x));
	}

	// add world X/Y axes for fast early rejection
	// note: this is not strictly necessary for correctness.
	axes.emplace_back(Vec2f(1.0f, 0.0f));
	axes.emplace_back(Vec2f(0.0f, 1.0f));

	// Precompute projections along each axis for SAT
	for (auto& axis : axes) {
		project(points, axis);
	}
}
