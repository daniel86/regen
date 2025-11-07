#include <regen/utility/logging.h>
#include <regen/objects/mesh.h>

#include "bounding-box.h"
#include "aabb.h"
#include "obb.h"

using namespace regen;

BoundingBox::BoundingBox(BoundingBoxType type, const Bounds<Vec3f> &bounds)
		: BoundingShape(BoundingShapeType::BOX),
		  type_(type),
		  baseBounds_(bounds),
		  tfBounds_(bounds),
		  basePosition_((bounds.max + bounds.min) * 0.5f) {}

BoundingBox::BoundingBox(BoundingBoxType type, const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts)
		: BoundingShape(BoundingShapeType::BOX, mesh, parts),
		  type_(type),
		  baseBounds_(mesh->minPosition(), mesh->maxPosition()),
		  tfBounds_(mesh->minPosition(), mesh->maxPosition()) {
	for (const auto &part : parts) {
		baseBounds_.min.setMin(part->minPosition());
		baseBounds_.max.setMax(part->maxPosition());
	}
	basePosition_ = (baseBounds_.max + baseBounds_.min) * 0.5f;
	tfBounds_ = baseBounds_;
}

void BoundingBox::updateBaseBounds(const Vec3f &min, const Vec3f &max) {
	baseBounds_.min = mesh_->minPosition();
	baseBounds_.max = mesh_->maxPosition();
	for (const auto &part : parts_) {
		baseBounds_.min.setMin(part->minPosition());
		baseBounds_.max.setMax(part->maxPosition());
	}
	baseBounds_.min += baseOffset_;
	baseBounds_.max += baseOffset_;
	basePosition_ = (baseBounds_.max + baseBounds_.min) * 0.5f;
	if (!transform_ || !transform_->hasModelMat()) {
		tfBounds_ = baseBounds_;
	}
	// reset TF stamp to force update
	lastTransformStamp_ = 0;
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
