#include <regen/utility/logging.h>

#include "obb.h"

using namespace regen;

OBB::OBB(const ref_ptr<Mesh> &mesh)
		: BoundingBox(BoundingBoxType::OBB, mesh) {
	updateOBB();
}

OBB::OBB(const Bounds<Vec3f> &bounds)
		: BoundingBox(BoundingBoxType::OBB, bounds) {
	updateOBB();
}

bool OBB::updateTransform(bool forceUpdate) {
	if (!forceUpdate && transformStamp() == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = transformStamp();
		updateOBB();
		return true;
	}
}

void OBB::updateOBB() {
	Vec3f offset = basePosition_;
	if (translation_.get()) {
		offset += translation_->getVertex(0);
	}
	// compute axes of the OBB based on the model transformation
	if (transform_.get()) {
		auto &tf = transform_->get()->getVertex(0);
		boxAxes_[0] = (tf ^ Vec4f(Vec3f::right(), 0.0f)).xyz_();
		boxAxes_[1] = (tf ^ Vec4f(Vec3f::up(), 0.0f)).xyz_();
		boxAxes_[2] = (tf ^ Vec4f(Vec3f::front(), 0.0f)).xyz_();
		offset += tf.position();
	} else {
		boxAxes_[0] = Vec3f::right();
		boxAxes_[1] = Vec3f::up();
		boxAxes_[2] = Vec3f::front();
	}

	// compute vertices of the OBB
	auto halfSize = (bounds_.max - bounds_.min) * 0.5f;
	Vec3f x = boxAxes_[0] * halfSize.x;
	Vec3f y = boxAxes_[1] * halfSize.y;
	Vec3f z = boxAxes_[2] * halfSize.z;
	vertices_[0] = offset - x - y - z;
	vertices_[1] = offset - x - y + z;
	vertices_[2] = offset - x + y - z;
	vertices_[3] = offset - x + y + z;
	vertices_[4] = offset + x - y - z;
	vertices_[5] = offset + x - y + z;
	vertices_[6] = offset + x + y - z;
	vertices_[7] = offset + x + y + z;
}

Vec3f OBB::closestPointOnSurface(const Vec3f &point) const {
	auto halfSize = (bounds_.max - bounds_.min) * 0.5f;
	const GLfloat *halfSizes = &halfSize.x;
	const Vec3f *axes = boxAxes();
	Vec3f obbCenter = getCenterPosition();
	Vec3f d = point - obbCenter;
	Vec3f closest = obbCenter;

	for (int i = 0; i < 3; ++i) {
		float distance = d.dot(axes[i]);
		if (distance > halfSizes[i]) distance = halfSizes[i];
		if (distance < -halfSizes[i]) distance = -halfSizes[i];
		closest = closest + axes[i] * distance;
	}

	return closest;
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
