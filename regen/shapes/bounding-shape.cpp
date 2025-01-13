#include <regen/utility/logging.h>

#include "bounding-shape.h"
#include "bounding-sphere.h"
#include "bounding-box.h"
#include "frustum.h"

using namespace regen;

GLuint BoundingShape::numInstances() const {
	if (transform_.get()) {
		return transform_->get()->numInstances();
	} else if (translation_.get()) {
		return translation_->numInstances();
	} else {
		return 1;
	}
}

void BoundingShape::setTransform(const ref_ptr<ModelTransformation> &transform, unsigned int instanceIndex) {
	transform_ = transform;
	translation_ = {};
	instanceIndex_ = instanceIndex;
}

void BoundingShape::setTransform(const ref_ptr<ShaderInput3f> &center, unsigned int instanceIndex) {
	translation_ = center;
	transform_ = {};
	instanceIndex_ = instanceIndex;
}

unsigned int BoundingShape::transformStamp() const {
	if (transform_.get()) {
		return transform_->get()->stamp();
	} else if (translation_.get()) {
		return translation_->stamp();
	} else {
		return 0;
	}
}

const Vec3f &BoundingShape::translation() const {
	if (transform_.get()) {
		return transform_->get()->getVertex(instanceIndex_).position();
	} else if (translation_.get()) {
		return translation_->getVertex(instanceIndex_);
	} else {
		return Vec3f::zero();
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
