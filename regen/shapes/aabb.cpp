#include "aabb.h"

using namespace regen;

AABB::AABB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts)
		: BoundingBox(BoundingShapeType::AABB, mesh, parts) {
	updateAABB();
}

AABB::AABB(const Bounds<Vec3f> &bounds)
		: BoundingBox(BoundingShapeType::AABB, bounds) {
	updateAABB();
}

bool AABB::updateTransform(bool forceUpdate) {
	const uint32_t stamp = tfStamp();
	if (!forceUpdate && stamp == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = stamp;
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
	// initialize vertices with base bounds (= without transform)
	// we will apply the transform below on each vertex
	// to compute the transformed bounds.
	setVertices(baseBounds());
	tfOrigin_ = basePosition_;

	// apply transform
	if (transform_.get()) {
		if (transform_->hasModelMat()) {
			auto tf = transform_->modelMat()->getVertexClamped(transformIndex_);
			// compute transformed bounds
			Vec3f &transformed = tfOrigin_; // note: borrow allocated memory for the loop.
			tfBounds_.min = (tf.r ^ vertices_[0]).xyz_();
			tfBounds_.max = tfBounds_.min;
			for (int i = 1; i < 8; ++i) {
				transformed = (tf.r ^ vertices_[i]).xyz_();
				tfBounds_.min.setMin(transformed);
				tfBounds_.max.setMax(transformed);
			}
			tfOrigin_ = tfBounds_.center();
			// set vertices based on transformed bounds
			setVertices(tfBounds_);
		} else {
			tfBounds_ = baseBounds_;
		}
		if (transform_->hasModelOffset()) {
			auto &modelOffset = transform_->modelOffset();
			auto offset = modelOffset->getVertexClamped(transformIndex_);
			tfBounds_.min += offset.r.xyz_();
			tfBounds_.max += offset.r.xyz_();
			tfOrigin_ += offset.r.xyz_();
			for (int i = 0; i < 8; ++i) {
				vertices_[i] += offset.r.xyz_();
			}
		}
	} else {
		tfBounds_ = baseBounds_;
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
	const Vec3f &aMin = tfBounds().min;
	const Vec3f &aMax = tfBounds().max;
	const Vec3f &bMin = other.tfBounds().min;
	const Vec3f &bMax = other.tfBounds().max;
	return aMin.x < bMax.x && aMax.x > bMin.x &&
		   aMin.y < bMax.y && aMax.y > bMin.y &&
		   aMin.z < bMax.z && aMax.z > bMin.z;
}

Vec3f AABB::closestPointOnSurface(const Vec3f &point) const {
	const Vec3f &aMin = tfBounds().min;
	const Vec3f &aMax = tfBounds().max;
	Vec3f closestPoint;
	closestPoint.x = point.x < aMin.x ? aMin.x : (point.x > aMax.x ? aMax.x : point.x);
	closestPoint.y = point.y < aMin.y ? aMin.y : (point.y > aMax.y ? aMax.y : point.y);
	closestPoint.z = point.z < aMin.z ? aMin.z : (point.z > aMax.z ? aMax.z : point.z);
	return closestPoint;
}
