#include <regen/utility/logging.h>

#include "bounding-shape.h"
#include "bounding-sphere.h"
#include "bounding-box.h"
#include "frustum.h"

using namespace regen;

BoundingShape::BoundingShape(BoundingShapeType shapeType)
		: shapeType_(shapeType),
		  lastGeometryStamp_(0u) {
}

BoundingShape::BoundingShape(BoundingShapeType shapeType,
			const ref_ptr<Mesh> &mesh,
			const std::vector<ref_ptr<Mesh>> &parts)
		: shapeType_(shapeType),
		  mesh_(mesh),
		  parts_(parts),
		  lastGeometryStamp_(mesh_->geometryStamp()) {
	if (mesh_.get()) {
		baseMesh_ = mesh_;
	} else if (!parts_.empty()) {
		baseMesh_ = parts_.front();
	}
}

bool BoundingShape::updateGeometry() {
	if (mesh_.get()) {
		// check if mesh geometry has changed
		auto meshStamp = mesh_->geometryStamp();
		if (meshStamp == lastGeometryStamp_) {
			return false;
		} else {
			lastGeometryStamp_ = meshStamp;
			updateBounds(mesh_->minPosition(), mesh_->maxPosition());
			return true;
		}
	}
	else if (nextGeometryStamp_ != lastGeometryStamp_) {
		lastGeometryStamp_ = nextGeometryStamp_;
		return true;
	}
	else {
		return false;
	}
}

uint32_t BoundingShape::numInstances() const {
	return transform_.get() ? transform_->numInstances() : 1u;
}

uint32_t BoundingShape::transformStamp() const {
	return transform_.get() ? transform_->stamp() : localStamp_;
}

void BoundingShape::setTransform(const ref_ptr<ModelTransformation> &transform, uint32_t instanceIndex) {
	transform_ = transform;
	localTransform_ = Mat4f::identity();
	transformIndex_ = instanceIndex;
}

void BoundingShape::setTransform(const Mat4f &localTransform) {
	localTransform_ = localTransform;
	if (transform_.get()) {
		localStamp_ = transform_->stamp();
	}
	localStamp_ += 1u;
	transform_ = {};
	transformIndex_ = 0;
}

const Vec3f& BoundingShape::translation() const {
	if (transform_.get()) {
		return transform_->position(transformIndex_).r;
	} else {
		return localTransform_.position();
	}
}

bool BoundingShape::hasIntersectionWith(const BoundingShape &other) const {
	switch (shapeType()) {
		case BoundingShapeType::SPHERE:
			switch (other.shapeType()) {
				case BoundingShapeType::SPHERE:
				case BoundingShapeType::BOX:
					return ((const BoundingSphere &) *this).hasIntersectionWithSphere(other);
				case BoundingShapeType::FRUSTUM:
					return ((const Frustum &) other).hasIntersectionWithFrustum((const BoundingSphere &) *this);
			}

		case BoundingShapeType::BOX:
			switch (other.shapeType()) {
				case BoundingShapeType::SPHERE:
					return ((const BoundingSphere &) other).hasIntersectionWithSphere(*this);
				case BoundingShapeType::BOX:
					return ((const BoundingBox &) *this).hasIntersectionWithBox((const BoundingBox &) other);
				case BoundingShapeType::FRUSTUM:
					return ((const Frustum &) other).hasIntersectionWithFrustum((const BoundingBox &) *this);
			}

		case BoundingShapeType::FRUSTUM:
			switch (other.shapeType()) {
				case BoundingShapeType::SPHERE:
					return ((const Frustum *) this)->hasIntersectionWithFrustum((const BoundingSphere &) other);
				case BoundingShapeType::BOX:
					return ((const Frustum *) this)->hasIntersectionWithFrustum((const BoundingBox &) other);
				case BoundingShapeType::FRUSTUM:
					return ((const Frustum *) this)->hasIntersectionWithFrustum((const Frustum &) other);
			}
	}
	return false;
}
