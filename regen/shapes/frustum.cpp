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
		BoundingShape(BoundingShapeType::FRUSTUM) {
	translation_ = ref_ptr<ShaderInput3f>::alloc("frustumCenter");
	translation_->setUniformData(Vec3f(0));
	direction_ = ref_ptr<ShaderInput3f>::alloc("frustumDirection");
	direction_->setUniformData(Vec3f::front());
}

void Frustum::set(double _aspect, double _fov, double _near, double _far) {
	fov = _fov;
	aspect = _aspect;
	near = _near;
	far = _far;
	// +0.2 is important because we might get artifacts at
	// the screen borders.
	double fovR = _fov / 57.2957795 + 0.2;
	nearPlane.y = tan(fovR * 0.5) * near;
	nearPlane.x = nearPlane.y * aspect;
	farPlane.y = tan(fovR * 0.5) * far;
	farPlane.x = farPlane.y * aspect;
}

bool Frustum::updateTransform(bool forceUpdate) {
	if (!forceUpdate && lastTransformStamp_ == transformStamp() && lastDirectionStamp_ == directionStamp()) {
		return false;
	}
	lastTransformStamp_ = transformStamp();
	lastDirectionStamp_ = directionStamp();
	return true;
}

const Vec3f &Frustum::direction() const {
	if (direction_.get()) {
		return direction_->getVertex(0);
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

	translation_->setUniformData(pos);
	direction_->setUniformData(d);

	Vec3f fc = pos + d * far;
	Vec3f nc = pos + d * near;
	Vec3f rw, uh, u, buf1, buf2;

	Vec3f v = d.cross(Vec3f::up());
	v.normalize();
	u = v.cross(d);
	u.normalize();

	rw = v * nearPlane.x;
	uh = u * nearPlane.y;
	buf1 = uh - rw;
	buf2 = uh + rw;
	points[0] = nc - buf1;
	points[1] = nc + buf1;
	points[2] = nc + buf2;
	points[3] = nc - buf2;

	rw = v * farPlane.x;
	uh = u * farPlane.y;
	buf1 = uh - rw;
	buf2 = uh + rw;
	points[4] = fc - buf1;
	points[5] = fc + buf1;
	points[6] = fc + buf2;
	points[7] = fc - buf2;

	planes[0].set(points[2], points[1], points[5]);
	planes[1].set(points[3], points[0], points[4]);
	planes[2].set(points[1], points[3], points[7]);
	planes[3].set(points[0], points[2], points[4]);
	planes[4].set(points[1], points[2], points[0]);
	planes[5].set(points[6], points[5], points[7]);
}

Vec3f Frustum::getCenterPosition() const {
	auto &basePosition = translation_->getVertex(translationIndex_);
	auto &dir = direction();
	return basePosition + dir * (near + (far - near) * 0.5f);
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

std::vector<Frustum *> Frustum::split(GLuint count, GLdouble splitWeight) const {
	const auto &n = near;
	const auto &f = far;

	std::vector<Frustum *> frustas(count);
	auto ratio = f / n;
	double si, lastn, currf, currn;

	lastn = n;
	for (GLuint i = 1; i < count; ++i) {
		si = i / (GLdouble) count;

		// C_i = \lambda * C_i^{log} + (1-\lambda) * C_i^{uni}
		currn = splitWeight * (n * (pow(ratio, si))) +
				(1 - splitWeight) * (n + (f - n) * si);
		currf = currn * 1.005;

		frustas[i - 1] = new Frustum;
		frustas[i - 1]->set(fov, aspect, lastn, currf);

		lastn = currn;
	}
	frustas[count - 1] = new Frustum;
	frustas[count - 1]->set(fov, aspect, lastn, f);

	return frustas;
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


