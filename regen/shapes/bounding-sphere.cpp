#include "bounding-sphere.h"
#include "regen/meshes/primitives/sphere.h"

using namespace regen;

BoundingSphere::BoundingSphere(const Vec3f &basePosition, GLfloat radius)
		: BoundingShape(BoundingShapeType::SPHERE),
		  basePosition_(basePosition),
		  radius_(radius) {}

BoundingSphere::BoundingSphere(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts, float radius)
		: BoundingShape(BoundingShapeType::SPHERE, mesh, parts),
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
		updateShapeOrigin();
		return true;
	}
}

void BoundingSphere::updateShapeOrigin() {
	shapeOrigin_ = basePosition_ + translation();
}

Vec3f BoundingSphere::closestPointOnSurface(const Vec3f &point) const {
	const Vec3f &p = getShapeOrigin();
	Vec3f d = point - p;
	if (d.length() == 0) {
		return p + Vec3f(0, 0, 1) * radius();
	}
	d.normalize();
	return p + d * radius();
}

bool BoundingSphere::hasIntersectionWithSphere(const BoundingShape &other) const {
	const Vec3f &p_this = getShapeOrigin();
	const Vec3f &p_other = other.getShapeOrigin();
	if ((p_this - p_other).length() <= radius()) {
		return true;
	}
	Vec3f closestPoint = other.closestPointOnSurface(p_this);
	return (closestPoint - p_this).length() <= radius();
}

/**
bool hasHalfSphereIntersection(const Vec3f &center, GLfloat radius) const {
	// get the distance from the camera to the center of the sphere
	auto d = Plane(
			position_[0].xyz_(),
			direction_[0].xyz_()).distance(center);
	// check if the sphere is outside the far plane
	if (d - radius > projParams_[0].far) return false;
	// check if the sphere is inside the near plane
	if (d + radius < projParams_[0].near) return false;
	// check if the sphere is inside the half sphere
	auto halfSphereRadius = projParams_[0].far;
	auto halfSphereNormal = direction_[0];
	auto halfSphereCenter = position_[0].xyz_() + halfSphereNormal.xyz_() * halfSphereRadius;
	return Plane(halfSphereCenter, halfSphereNormal.xyz_()).distance(center) < radius;
}

bool hasHalfSphereIntersection(const Vec3f &center, const Vec3f *points) const {
	// get the distance from the camera to the center of the sphere
	auto d = Plane(
			position_[0].xyz_(),
			direction_[0].xyz_()).distance(center);
	// check if the sphere is outside the far plane
	if (d > projParams_[0].far) return false;
	// check if the sphere is inside the near plane
	if (d < projParams_[0].near) return false;
	// check if the sphere is inside the half sphere
	auto halfSphereRadius = projParams_[0].far;
	auto halfSphereNormal = direction_[0];
	auto halfSphereCenter = position_[0].xyz_() + halfSphereNormal.xyz_() * halfSphereRadius;
	auto halfSphere = Plane(halfSphereCenter, halfSphereNormal.xyz_());
	for (int i = 0; i < 8; ++i) {
		if (halfSphere.distance(center + points[i]) < 0) {
			return false;
		}
	}
	return true;
}
 */
