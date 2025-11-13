#include <regen/utility/logging.h>

#include "obb.h"

using namespace regen;

OBB::OBB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts)
		: BoundingBox(BoundingShapeType::OBB, mesh, parts) {
	updateOBB();
}

OBB::OBB(const Bounds<Vec3f> &bounds)
		: BoundingBox(BoundingShapeType::OBB, bounds) {
	updateOBB();
}

bool OBB::updateTransform(bool forceUpdate) {
	auto stamp = tfStamp();
	if (!forceUpdate && stamp == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = stamp;
		updateOBB();
		return true;
	}
}

void OBB::applyTransform(const Mat4f &tf) {
	boxAxes_[0] = (tf ^ Vec4f(Vec3f::right(), 0.0f)).xyz_();
	boxAxes_[1] = (tf ^ Vec4f(Vec3f::up(), 0.0f)).xyz_();
	boxAxes_[2] = (tf ^ Vec4f(Vec3f::front(), 0.0f)).xyz_();
	tfOrigin_ = (tf ^ Vec4f(tfOrigin_, 1.0f)).xyz_();
}

void OBB::updateOBB() {
	tfOrigin_ = basePosition_;

	Vec3f scaling = Vec3f::one();
	// compute axes of the OBB based on the model transformation
	if (transform_.get()) {
		if (transform_->hasModelMat()) {
			auto tf = transform_->modelMat()->getVertex(transformIndex_);
			applyTransform(tf.r);
			scaling = tf.r.scaling();
		}
		if (transform_->hasModelOffset()) {
			tfOrigin_ += transform_->modelOffset()->getVertexClamped(transformIndex_).r.xyz_();
		}
	} else if (localStamp_ != 1) {
		// use local transform if no model transformation is set.
		// note: stamp!=1 indicates we do not have an identity transform.
		applyTransform(localTransform_);
		scaling = localTransform_.scaling();
	} else {
		boxAxes_[0] = Vec3f::right();
		boxAxes_[1] = Vec3f::up();
		boxAxes_[2] = Vec3f::front();
	}
	if (scaling != Vec3f::one()) {
		// normalize axes if scaling is applied
		boxAxes_[0].normalize();
		boxAxes_[1].normalize();
		boxAxes_[2].normalize();
	}

	// compute vertices of the OBB
	auto halfSize = (baseBounds_.max - baseBounds_.min) * scaling * 0.5f;
	Vec3f x = boxAxes_[0] * halfSize.x;
	Vec3f y = boxAxes_[1] * halfSize.y;
	Vec3f z = boxAxes_[2] * halfSize.z;
	vertices_[0] = tfOrigin_ - x - y - z;
	vertices_[1] = tfOrigin_ - x - y + z;
	vertices_[2] = tfOrigin_ - x + y - z;
	vertices_[3] = tfOrigin_ - x + y + z;
	vertices_[4] = tfOrigin_ + x - y - z;
	vertices_[5] = tfOrigin_ + x - y + z;
	vertices_[6] = tfOrigin_ + x + y - z;
	vertices_[7] = tfOrigin_ + x + y + z;

	// finally compute the bounds of the OBB
	Vec3f extents =
		Vec3f(fabs(boxAxes_[0].x), fabs(boxAxes_[0].y), fabs(boxAxes_[0].z)) * halfSize.x +
		Vec3f(fabs(boxAxes_[1].x), fabs(boxAxes_[1].y), fabs(boxAxes_[1].z)) * halfSize.y +
		Vec3f(fabs(boxAxes_[2].x), fabs(boxAxes_[2].y), fabs(boxAxes_[2].z)) * halfSize.z;
	tfBounds_.min = tfOrigin_ - extents;
	tfBounds_.max = tfOrigin_ + extents;
}

Vec3f OBB::closestPointOnSurface(const Vec3f &point) const {
	auto halfSize = (baseBounds_.max - baseBounds_.min) * 0.5f;
	const float *halfSizes = &halfSize.x;
	const Vec3f *axes = boxAxes();
	const Vec3f &obbCenter = tfOrigin();
	Vec3f d = point - obbCenter;

	Vec3f tf_closest = obbCenter;
	for (int i = 0; i < 3; ++i) {
		tf_closest = tf_closest + axes[i] * math::clamp(d.dot(axes[i]), -halfSizes[i], halfSizes[i]);
	}
	return tf_closest;
}

bool OBB::overlapOnAxis(const BoundingBox &b, const Vec3f &axis) const {
	auto [minA, maxA] = this->project(axis);
	auto [minB, maxB] = b.project(axis);

	return maxA >= minB && maxB >= minA;
}

bool OBB::hasIntersectionWithOBB(const BoundingBox &other) const {
	const Vec3f *axes_a = boxAxes();
	const Vec3f *axes_b = other.boxAxes();
	std::array<Vec3f, 15> axes = {
			axes_a[0], axes_a[1], axes_a[2],
			axes_b[0], axes_b[1], axes_b[2],
			//
			axes_a[0].cross(axes_b[0]),
			axes_a[0].cross(axes_b[1]),
			axes_a[0].cross(axes_b[2]),
			//
			axes_a[1].cross(axes_b[0]),
			axes_a[1].cross(axes_b[1]),
			axes_a[1].cross(axes_b[2]),
			//
			axes_a[2].cross(axes_b[0]),
			axes_a[2].cross(axes_b[1]),
			axes_a[2].cross(axes_b[2])
	};

	for (const Vec3f &axis: axes) {
		if (axis.x == 0 && axis.y == 0 && axis.z == 0) continue; // Skip zero-length axes
		if (!overlapOnAxis(other, axis)) return false;
	}

	return true;
}
