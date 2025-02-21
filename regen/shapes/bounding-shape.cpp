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

BoundingShape::BoundingShape(BoundingShapeType shapeType, const ref_ptr<Mesh> &mesh)
		: shapeType_(shapeType),
		  mesh_(mesh),
		  lastGeometryStamp_(mesh_->geometryStamp()) {
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

GLuint BoundingShape::numInstances() const {
	GLuint numInstances = 1;
	if (transform_.get()) {
		numInstances = std::max(transform_->get()->numInstances(), numInstances);
	}
	if (modelOffset_.get()) {
		numInstances = std::max(modelOffset_->numInstances(), numInstances);
	}
	return numInstances;
}

void BoundingShape::setTransform(const ref_ptr<ModelTransformation> &transform, unsigned int instanceIndex) {
	transform_ = transform;
	transformIndex_ = instanceIndex;
}

void BoundingShape::setTransform(const ref_ptr<ShaderInput3f> &center, unsigned int instanceIndex) {
	modelOffset_ = center;
	modelOffsetIndex_ = instanceIndex;
}

unsigned int BoundingShape::transformStamp() const {
	unsigned int stamp = 0;
	if (transform_.get()) {
		stamp = transform_->get()->stamp();
	}
	if (modelOffset_.get()) {
		stamp = std::max(stamp, modelOffset_->stamp());
	}
	return stamp;
}

Vec3f BoundingShape::translation() const {
	if (transform_.get()) {
		auto p = transform_->get()->getVertex(transformIndex_);
		if (modelOffset_.get()) {
			return p.r.position() + modelOffset_->getVertex(modelOffsetIndex_).r;
		}
		else {
			return p.r.position();
		}
	}
	else if (modelOffset_.get()) {
		return modelOffset_->getVertex(modelOffsetIndex_).r;
	}
	else {
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
