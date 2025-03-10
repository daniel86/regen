#include "bounding-sphere.h"
#include "regen/meshes/primitives/sphere.h"

using namespace regen;

BoundingSphere::BoundingSphere(const Vec3f &basePosition, GLfloat radius)
		: BoundingShape(BoundingShapeType::SPHERE),
		  basePosition_(basePosition),
		  radius_(radius) {}

BoundingSphere::BoundingSphere(const ref_ptr<Mesh> &mesh, float radius)
		: BoundingShape(BoundingShapeType::SPHERE, mesh),
		  basePosition_(mesh->centerPosition()),
		  radius_(radius > 0.0f ? radius : computeRadius(mesh->minPosition(), mesh->maxPosition())) {
	auto sphereMesh = dynamic_cast<Sphere *>(mesh.get());
	if (sphereMesh) {
		radius_ = sphereMesh->radius();
	}
}

void BoundingSphere::updateBounds(const Vec3f &min, const Vec3f &max) {
	basePosition_ = (min + max) * 0.5;
	auto sphereMesh = dynamic_cast<Sphere *>(mesh_.get());
	if (sphereMesh) {
		radius_ = sphereMesh->radius();
	} else {
		radius_ = computeRadius(min, max);
	}
}

float BoundingSphere::computeRadius(const Vec3f &min, const Vec3f &max) {
	auto radius = (max - basePosition_).length();
	return radius;
}

bool BoundingSphere::updateTransform(bool forceUpdate) {
	if (!forceUpdate && transformStamp() == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = transformStamp();
		return true;
	}
}

Vec3f BoundingSphere::getCenterPosition() const {
	Vec3f p = basePosition_;
	if (modelOffset_.get()) {
		p += modelOffset_->getVertex(modelOffsetIndex_).r;
	}
	if (transform_.get()) {
		p += transform_->get()->getVertex(transformIndex_).r.position();
	}
	return p;
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
