/*
 * frustum.cpp
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#include <regen/utility/logging.h>

#include "frustum.h"

using namespace regen;

Frustum::Frustum() :
		BoundingShape(BoundingShapeType::FRUSTUM),
		orthoBounds(Vec2f(0), Vec2f(0)) {
	modelOffset_ = ref_ptr<ShaderInput3f>::alloc("frustumCenter");
	modelOffset_->setUniformData(Vec3f(0));
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
		return direction_->stamp();
	} else {
		return 0;
	}
}

void Frustum::update(const Vec3f &pos, const Vec3f &dir) {
	Vec3f d = dir;
	d.normalize();
	modelOffset_->setUniformData(pos);
	direction_->setUniformData(d);

	if (fov > 0.0) {
		updatePointsPerspective(pos, d);
	} else {
		updatePointsOrthogonal(pos, d);
	}

	planes[0].set(points[2], points[1], points[5]);
	planes[1].set(points[3], points[0], points[4]);
	planes[2].set(points[1], points[3], points[7]);
	planes[3].set(points[0], points[2], points[4]);
	planes[4].set(points[1], points[2], points[0]);
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
	points[0] = nc - uh + rw;
	points[1] = nc + uh - rw;
	points[2] = nc + uh + rw;
	points[3] = nc - uh - rw;
	// far plane points
	auto fc = pos + dir * far;
	rw = v * farPlaneHalfSize.x;
	uh = u * farPlaneHalfSize.y;
	points[4] = fc - uh + rw;
	points[5] = fc + uh - rw;
	points[6] = fc + uh + rw;
	points[7] = fc - uh - rw;
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
	points[0] = nc + vr + ub;
	points[1] = nc + vl + ut;
	points[2] = nc + vr + ut;
	points[3] = nc + vl + ub;
	// far plane points
	auto fc = pos + dir * far;
	points[4] = fc + vr + ub;
	points[5] = fc + vl + ut;
	points[6] = fc + vr + ut;
	points[7] = fc + vl + ub;
}

Vec3f Frustum::getCenterPosition() const {
	auto basePosition = modelOffset_->getVertex(modelOffsetIndex_);
	if (direction_.get()) {
		auto dir = direction_->getVertex(0);
		return basePosition.r + dir.r * (near + (far - near) * 0.5f);
	} else {
		return basePosition.r + Vec3f::front() * (near + (far - near) * 0.5f);
	}
}

bool Frustum::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) const {
	for (const auto &plane: planes) {
		if (plane.distance(center) < -radius) {
			return GL_FALSE;
		}
	}
	return GL_TRUE;
}

bool hasIntersection_(
		const Plane *planes,
		const Vec3f &center,
		const Vec3f *points,
		unsigned int numPoints) {
	GLboolean allOutside;
	for (unsigned int i = 0u; i < 6u; ++i) {
		allOutside = true;
		for (unsigned int j = 0u; j < numPoints; ++j) {
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

bool hasIntersection_(
		const Plane *planes,
		const Vec3f *points,
		unsigned int numPoints) {
	GLboolean allOutside;
	for (unsigned int i = 0u; i < 6u; ++i) {
		allOutside = true;
		for (unsigned int j = 0u; j < numPoints; ++j) {
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

bool Frustum::hasIntersectionWithBox(const Vec3f &center, const Vec3f *point) const {
	return hasIntersection_(planes, center, point, 8);
}

bool Frustum::hasIntersectionWithFrustum(const BoundingSphere &sphere) const {
	return hasIntersectionWithSphere(sphere.translation(), sphere.radius());
}

bool Frustum::hasIntersectionWithFrustum(const BoundingBox &box) const {
	return hasIntersection_(planes, box.boxVertices(), 8);
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
			return GL_FALSE;
		}
	}
	return GL_TRUE;
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
	float minDistance = std::numeric_limits<float>::max();

	for (const auto &plane: planes) {
		Vec3f planePoint = plane.closestPoint(point);
		float distance = (planePoint - point).length();
		if (distance < minDistance) {
			minDistance = distance;
			closestPoint = planePoint;
		}
	}

	return closestPoint;
}


