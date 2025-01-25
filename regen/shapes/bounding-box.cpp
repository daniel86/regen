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

BoundingBox::BoundingBox(BoundingBoxType type, const ref_ptr<Mesh> &mesh)
		: BoundingShape(BoundingShapeType::BOX, mesh),
		  type_(type),
		  bounds_(mesh->minPosition(), mesh->maxPosition()),
		  basePosition_((bounds_.max + bounds_.min) * 0.5f) {
}

void BoundingBox::updateBounds(const Vec3f &min, const Vec3f &max) {
	bounds_.min = mesh_->minPosition();
	bounds_.max = mesh_->maxPosition();
	basePosition_ = (bounds_.max + bounds_.min) * 0.5f;
}

Vec3f BoundingBox::getCenterPosition() const {
	Vec3f p = basePosition_;
	if (translation_.get()) {
		p += translation_->getVertex(translationIndex_);
	}
	if (transform_.get()) {
		return p + transform_->get()->getVertex(transformIndex_).position();
	} else {
		return p;
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
