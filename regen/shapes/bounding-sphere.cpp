#include "bounding-sphere.h"

using namespace regen;

BoundingSphere::BoundingSphere(const Vec3f &basePosition, GLfloat radius)
		: BoundingShape(BoundingShapeType::SPHERE),
		  basePosition_(basePosition),
		  radius_(radius) {}

bool BoundingSphere::update() {
	if (transformStamp() == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = transformStamp();
		return true;
	}
}

Vec3f BoundingSphere::getCenterPosition() const {
	if (transform_.get()) {
		return basePosition_ + transform_->get()->getVertex(instanceIndex_).position();
	} else if (translation_.get()) {
		return basePosition_ + translation_->getVertex(instanceIndex_);
	}
	return basePosition_;
}

Vec3f BoundingSphere::closestPointOnSurface(const Vec3f &point) const {
	Vec3f p = getCenterPosition();
	Vec3f d = point - p;
	if (d.length() == 0) {
		return p + Vec3f(0, 0, 1) * radius();
	}
	d.normalize();
	return p + d * radius();
}

bool BoundingSphere::hasIntersectionWithSphere(const BoundingShape &other) const {
	Vec3f p_this = getCenterPosition();
	Vec3f p_other = other.getCenterPosition();
	if ((p_this - p_other).length() <= radius()) {
		return true;
	}
	Vec3f closestPoint = other.closestPointOnSurface(p_this);
	return (closestPoint - p_this).length() <= radius();
}
