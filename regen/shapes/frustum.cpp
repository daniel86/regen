#include <regen/utility/logging.h>

#include "frustum.h"

using namespace regen;

Frustum::Frustum() :
		BoundingShape(BoundingShapeType::FRUSTUM),
		orthoBounds(Vec2f(0), Vec2f(0)) {
	direction_ = ref_ptr<ShaderInput3f>::alloc("frustumDirection");
	direction_->setUniformData(Vec3f::front());
}

void Frustum::setPerspective(double _aspect, double _fov, double _near, double _far) {
	fov = _fov;
	aspect = _aspect;
	near = _near;
	far = _far;
	double fovR = _fov / 57.2957795;
	nearPlaneHalfSize.y = tan(fovR * 0.5) * near;
	nearPlaneHalfSize.x = nearPlaneHalfSize.y * aspect;
	farPlaneHalfSize.y = tan(fovR * 0.5) * far;
	farPlaneHalfSize.x = farPlaneHalfSize.y * aspect;
}

void Frustum::setOrtho(double left, double right, double bottom, double top, double _near, double _far) {
	fov = 0.0;
	aspect = abs((right - left) / (top - bottom));
	near = _near;
	far = _far;
	nearPlaneHalfSize.x = abs(right-left)*0.5;
	nearPlaneHalfSize.y = abs(top-bottom)*0.5;
	farPlaneHalfSize = nearPlaneHalfSize;
	orthoBounds.min.x = left;
	orthoBounds.min.y = bottom;
	orthoBounds.max.x = right;
	orthoBounds.max.y = top;
}

bool Frustum::updateTransform(bool forceUpdate) {
	if (!forceUpdate && lastTransformStamp_ == transformStamp() && lastDirectionStamp_ == directionStamp()) {
		return false;
	}
	lastTransformStamp_ = transformStamp();
	lastDirectionStamp_ = directionStamp();
	return true;
}

Vec3f Frustum::direction() const {
	if (direction_.get()) {
		return direction_->getVertex(0).r;
	} else {
		return Vec3f::front();
	}
}

unsigned int Frustum::directionStamp() const {
	if (direction_.get()) {
		return direction_->stampOfReadData();
	} else {
		return 0;
	}
}

void Frustum::update(const Vec3f &pos, const Vec3f &dir) {
	Vec3f d = dir;
	d.normalize();
	localTransform_.setPosition(pos);
	direction_->setVertex(0, d);
	shapeOrigin_ = pos;

	if (fov > 0.0) {
		updatePointsPerspective(pos, d);
	} else {
		updatePointsOrthogonal(pos, d);
	}

	// Top:		nTr - nTl - fTl
	planes[0].set(points[2], points[1], points[5]);
	// Bottom:	nBl - nBr - fBr
	planes[1].set(points[3], points[0], points[4]);
	// Left:	ntL - nbL - fbL
	planes[2].set(points[1], points[3], points[7]);
	// Right:	nbR - ntR - fbR
	planes[3].set(points[0], points[2], points[4]);
	// Near:	nTl - nTr - nBr
	planes[4].set(points[1], points[2], points[0]);
	// Far:		fTr - fTl - fBl
	planes[5].set(points[6], points[5], points[7]);
}

void Frustum::updatePointsPerspective(const Vec3f &pos, const Vec3f &dir) {
	auto v = dir.cross(Vec3f::up());
	v.normalize();
	auto u = v.cross(dir);
	u.normalize();
	// near plane points
	auto nc = pos + dir * near;
	auto rw = v * nearPlaneHalfSize.x;
	auto uh = u * nearPlaneHalfSize.y;
	points[0] = nc - uh + rw; // bottom right
	points[1] = nc + uh - rw; // top left
	points[2] = nc + uh + rw; // top right
	points[3] = nc - uh - rw; // bottom left
	// far plane points
	auto fc = pos + dir * far;
	rw = v * farPlaneHalfSize.x;
	uh = u * farPlaneHalfSize.y;
	points[4] = fc - uh + rw; // bottom right
	points[5] = fc + uh - rw; // top left
	points[6] = fc + uh + rw; // top right
	points[7] = fc - uh - rw; // bottom left
}

void Frustum::updatePointsOrthogonal(const Vec3f &pos, const Vec3f &dir) {
	auto v = dir.cross(Vec3f::up());
	v.normalize();
	auto u = v.cross(dir);
	u.normalize();
	// do not assume that the frustum is centered at the far/near plane centroids!
	// could be that left/right/top/bottom are not symmetric
	auto vl = v * orthoBounds.min.x; // left
	auto vr = v * orthoBounds.max.x; // right
	auto ub = u * orthoBounds.min.y; // bottom
	auto ut = u * orthoBounds.max.y; // top
	// near plane points
	auto nc = pos + dir * near;
	points[0] = nc + vr + ub; // bottom right
	points[1] = nc + vl + ut; // top left
	points[2] = nc + vr + ut; // top right
	points[3] = nc + vl + ub; // bottom left
	// far plane points
	auto fc = pos + dir * far;
	points[4] = fc + vr + ub; // bottom right
	points[5] = fc + vl + ut; // top left
	points[6] = fc + vr + ut; // top right
	points[7] = fc + vl + ub; // bottom left
}

bool Frustum::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) const {
	for (const auto &plane: planes) {
		if (plane.distance(center) < -radius) {
			return false;
		}
	}
	return true;
}

static inline bool hasIntersection_AABB_(const Plane *planes, const BoundingBox &box) {
	const auto &points = box.boxVertices();
	Vec3f p_min = points[0];
	Vec3f p_max = points[0];
	for (unsigned int i = 1u; i < 8u; ++i) {
		p_min.x = std::min(p_min.x, points[i].x);
		p_min.y = std::min(p_min.y, points[i].y);
		p_min.z = std::min(p_min.z, points[i].z);
		p_max.x = std::max(p_max.x, points[i].x);
		p_max.y = std::max(p_max.y, points[i].y);
		p_max.z = std::max(p_max.z, points[i].z);
	}
	for (unsigned int i = 0u; i < 6u; ++i) {
		// Select vertex farthest from the plane in direction of the plane normal.
		// If this point is behind the plane, the AABB must be outside of the frustum.
		auto &plane = planes[i];
		auto &n = plane.normal;
		Vec3f p(
			n.x > 0.0 ? p_min.x : p_max.x,
			n.y > 0.0 ? p_min.y : p_max.y,
			n.z > 0.0 ? p_min.z : p_max.z);
		if (n.dot(plane.point) - n.dot(p) < 0.0) {
			// AABB is outside of the frustum
			return false;
		}
	}
	return true;
}

static inline bool hasIntersection_OBB_(const Plane *planes, const BoundingBox &box) {
	bool allOutside;
	auto *points = box.boxVertices();
	for (unsigned int i = 0u; i < 6u; ++i) {
		allOutside = true;
		for (unsigned int j = 0u; j < 8; ++j) {
			if (planes[i].distance(points[j]) >= 0.0) {
				allOutside = false;
				break;
			}
		}
		if (allOutside) {
			return false;
		}
	}
	return true;
}

static inline bool hasIntersection_OBB_(
		const Plane *planes,
		const Vec3f &center,
		const Vec3f *points) {
	bool allOutside;
	for (unsigned int i = 0u; i < 6u; ++i) {
		allOutside = true;
		for (unsigned int j = 0u; j < 8; ++j) {
			if (planes[i].distance(center + points[j]) >= 0.0) {
				allOutside = false;
				break;
			}
		}
		if (allOutside) {
			return false;
		}
	}
	return true;
}

bool Frustum::hasIntersectionWithBox(const Vec3f &center, const Vec3f *point) const {
	return hasIntersection_OBB_(planes, center, point);
}

bool Frustum::hasIntersectionWithFrustum(const BoundingSphere &sphere) const {
	return hasIntersectionWithSphere(sphere.translation(), sphere.radius());
}

bool Frustum::hasIntersectionWithFrustum(const BoundingBox &box) const {
	if (box.isAABB()) {
		return hasIntersection_AABB_(planes, box);
	} else {
		return hasIntersection_OBB_(planes, box);
	}
}

bool Frustum::hasIntersectionWithFrustum(const Frustum &other) const {
	for (int i = 0; i < 6; ++i) {
		if (other.planes[i].distance(points[0]) < 0 &&
			other.planes[i].distance(points[1]) < 0 &&
			other.planes[i].distance(points[2]) < 0 &&
			other.planes[i].distance(points[3]) < 0 &&
			other.planes[i].distance(points[4]) < 0 &&
			other.planes[i].distance(points[5]) < 0 &&
			other.planes[i].distance(points[6]) < 0 &&
			other.planes[i].distance(points[7]) < 0) {
			return false;
		}
	}
	return true;
}

void Frustum::split(double splitWeight, std::vector<Frustum> &frustumSplit) const {
	const auto &n = near;
	const auto &f = far;
	const auto count = frustumSplit.size();
	auto ratio = f / n;
	double si, lastn, currf, currn;

	lastn = n;
	for (GLuint i = 1; i < count; ++i) {
		si = i / (GLdouble) count;

		// C_i = \lambda * C_i^{log} + (1-\lambda) * C_i^{uni}
		currn = splitWeight * (n * (pow(ratio, si))) +
				(1 - splitWeight) * (n + (f - n) * si);
		currf = currn * 1.005;

		frustumSplit[i - 1].setPerspective(aspect, fov, lastn, currf);

		lastn = currn;
	}
	frustumSplit[count - 1].setPerspective(aspect, fov, lastn, f);
}

Vec3f Frustum::closestPointOnSurface(const Vec3f &point) const {
	Vec3f closestPoint;
	float minDistanceSqr = std::numeric_limits<float>::max();

	for (const auto &plane: planes) {
		Vec3f planePoint = plane.closestPoint(point);
		float distanceSqr = (planePoint - point).lengthSquared();
		if (distanceSqr < minDistanceSqr) {
			minDistanceSqr = distanceSqr;
			closestPoint = planePoint;
		}
	}

	return closestPoint;
}


