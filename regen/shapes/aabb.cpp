#include "aabb.h"

using namespace regen;

AABB::AABB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts)
		: BoundingBox(BoundingBoxType::AABB, mesh, parts) {
	updateAABB();
}

AABB::AABB(const Bounds<Vec3f> &bounds)
		: BoundingBox(BoundingBoxType::AABB, bounds) {
	updateAABB();
}

bool AABB::updateTransform(bool forceUpdate) {
	if (!forceUpdate && transformStamp() == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = transformStamp();
		updateAABB();
		return true;
	}
}

void AABB::setVertices(const Bounds<Vec3f> &minMax) {
	// initial vertices based on bounds
	const Vec3f &a = minMax.min;
	const Vec3f &b = minMax.max;
	vertices_[0] = Vec3f(a.x, a.y, a.z);
	vertices_[1] = Vec3f(a.x, a.y, b.z);
	vertices_[2] = Vec3f(a.x, b.y, a.z);
	vertices_[3] = Vec3f(a.x, b.y, b.z);
	vertices_[4] = Vec3f(b.x, a.y, a.z);
	vertices_[5] = Vec3f(b.x, a.y, b.z);
	vertices_[6] = Vec3f(b.x, b.y, a.z);
	vertices_[7] = Vec3f(b.x, b.y, b.z);
}

void AABB::updateAABB() {
	// initialize vertices based on bounds
	setVertices(bounds());

	updateShapeOrigin();
	// apply transform
	if (transform_.get()) {
		if (transform_->hasModelOffset()) {
			auto modelOffset = transform_->modelOffset();
			for (int i = 0; i < 8; ++i) {
				vertices_[i] += modelOffset->getVertexClamped(transformIndex_).r.xyz_();
			}
		}
		if (transform_->hasModelMat()) {
			auto tf = transform_->modelMat()->getVertexClamped(transformIndex_);
			// compute transformed bounds
			Vec3f transformed;
			Vec3f transformedMin = getShapeOrigin();
			Vec3f transformedMax = transformedMin;
			for (int i = 0; i < 8; ++i) {
				transformed = (tf.r ^ vertices_[i]).xyz_();
				transformedMin.setMin(transformed);
				transformedMax.setMax(transformed);
			}
			// set vertices based on transformed bounds
			setVertices(Bounds<Vec3f>(transformedMin, transformedMax));
		}
	}
}

const Vec3f *AABB::boxAxes() const {
	static const Vec3f aabb_axes[3] = {
			Vec3f::right(),
			Vec3f::up(),
			Vec3f::front()
	};
	return aabb_axes;
}

bool AABB::hasIntersectionWithAABB(const AABB &other) const {
	auto a_p = translation();
	auto b_p = other.translation();
	Vec3f aMin = a_p.r + bounds().min;
	Vec3f aMax = a_p.r + bounds().max;
	Vec3f bMin = b_p.r + other.bounds().min;
	Vec3f bMax = b_p.r + other.bounds().max;
	return aMin.x < bMax.x && aMax.x > bMin.x &&
		   aMin.y < bMax.y && aMax.y > bMin.y &&
		   aMin.z < bMax.z && aMax.z > bMin.z;
}

Vec3f AABB::closestPointOnSurface(const Vec3f &point) const {
	auto a_p = translation();
	Vec3f aMin = a_p.r + bounds().min;
	Vec3f aMax = a_p.r + bounds().max;
	Vec3f closestPoint;
	closestPoint.x = point.x < aMin.x ? aMin.x : (point.x > aMax.x ? aMax.x : point.x);
	closestPoint.y = point.y < aMin.y ? aMin.y : (point.y > aMax.y ? aMax.y : point.y);
	closestPoint.z = point.z < aMin.z ? aMin.z : (point.z > aMax.z ? aMax.z : point.z);
	return closestPoint;
}
