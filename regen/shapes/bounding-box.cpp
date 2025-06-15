#include <regen/utility/logging.h>

#include "bounding-box.h"
#include "aabb.h"
#include "obb.h"

using namespace regen;

BoundingBox::BoundingBox(BoundingBoxType type, const Bounds<Vec3f> &bounds)
		: BoundingShape(BoundingShapeType::BOX),
		  type_(type),
		  bounds_(bounds),
		  basePosition_((bounds.max + bounds.min) * 0.5f) {}

BoundingBox::BoundingBox(BoundingBoxType type, const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts)
		: BoundingShape(BoundingShapeType::BOX, mesh, parts),
		  type_(type),
		  bounds_(mesh->minPosition(), mesh->maxPosition()) {
	for (const auto &part : parts) {
		bounds_.min.setMin(part->minPosition());
		bounds_.max.setMax(part->maxPosition());
	}
	basePosition_ = (bounds_.max + bounds_.min) * 0.5f;
}

void BoundingBox::updateBounds(const Vec3f &min, const Vec3f &max) {
	bounds_.min = mesh_->minPosition();
	bounds_.max = mesh_->maxPosition();
	for (const auto &part : parts_) {
		bounds_.min.setMin(part->minPosition());
		bounds_.max.setMax(part->maxPosition());
	}
	basePosition_ = (bounds_.max + bounds_.min) * 0.5f;
}

void BoundingBox::updateShapeOrigin() {
	shapeOrigin_ = basePosition_;
	if (transform_.get()) {
		shapeOrigin_ += transform_->position(transformIndex_).r;
	}
}

std::pair<float, float> BoundingBox::project(const Vec3f &axis) const {
	const Vec3f *v = boxVertices();
	float min = v[0].dot(axis);
	float max = min;
	for (int i = 1; i < 8; ++i) {
		float projection = v[i].dot(axis);
		if (projection < min) min = projection;
		if (projection > max) max = projection;
	}
	return std::make_pair(min, max);
}

bool BoundingBox::hasIntersectionWithBox(const BoundingBox &other) const {
	switch (type_) {
		case BoundingBoxType::AABB:
			switch (other.type_) {
				case BoundingBoxType::AABB:
					return ((AABB &) *this).hasIntersectionWithAABB((const AABB &) other);
				case BoundingBoxType::OBB:
					return ((OBB &) other).hasIntersectionWithOBB(*this);
			}
		case BoundingBoxType::OBB:
			return ((OBB &) *this).hasIntersectionWithOBB(other);
	}
	return false;
}
