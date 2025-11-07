#include "bounding-shape.h"
#include "bounding-sphere.h"
#include "bounding-box.h"
#include "frustum.h"
#include "regen/objects/mesh.h"

using namespace regen;

BoundingShape::BoundingShape(BoundingShapeType shapeType)
		: shapeType_(shapeType),
		  useLocalStamp_(false),
		  lastGeometryStamp_(0u) {
	localStamp_.store(0u);
	updateStampFunction();
}

BoundingShape::BoundingShape(BoundingShapeType shapeType,
			const ref_ptr<Mesh> &mesh,
			const std::vector<ref_ptr<Mesh>> &parts)
		: shapeType_(shapeType),
		  mesh_(mesh),
		  parts_(parts),
		  useLocalStamp_(false),
		  lastGeometryStamp_(mesh_->geometryStamp()) {
	if (mesh_.get()) {
		baseMesh_ = mesh_;
	} else if (!parts_.empty()) {
		baseMesh_ = parts_.front();
	}
	updateStampFunction();
}

BoundingShape::~BoundingShape() = default;

void BoundingShape::addPart(const ref_ptr<Mesh> &part) {
	if (part.get() != nullptr) {
		parts_.push_back(part);
	}
	if (!baseMesh_.get()) {
		baseMesh_ = part;
	}
}

void BoundingShape::updateStampFunction() {
	if (!useLocalStamp_ && transform_.get()) {
		stampFun_ = &BoundingShape::getTransformStamp;
	} else {
		stampFun_ = &BoundingShape::getLocalStamp;
	}
}

void BoundingShape::setUseLocalStamp(bool useLocalStamp) {
	useLocalStamp_ = useLocalStamp;
	updateStampFunction();
}

bool BoundingShape::updateGeometry() {
	if (mesh_.get()) {
		// check if mesh geometry has changed
		auto meshStamp = mesh_->geometryStamp();
		if (meshStamp == lastGeometryStamp_) {
			return false;
		} else {
			lastGeometryStamp_ = meshStamp;
			updateBaseBounds(mesh_->minPosition(), mesh_->maxPosition());
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

const OrthogonalProjection& BoundingShape::getOrthogonalProjection() const {
	if (!orthoProjection_.get()) {
		orthoProjection_ = ref_ptr<OrthogonalProjection>::alloc(*this);
	} else {
		orthoProjection_->update(*this);
	}
	return *orthoProjection_.get();
}

void BoundingShape::setBaseOffset(const Vec3f &offset) {
	baseOffset_ = offset;
	if (mesh_.get()) {
		mesh_->nextGeometryStamp();
	} else {
		nextGeometryStamp_ += 1u;
	}
}

uint32_t BoundingShape::numInstances() const {
	return transform_.get() ? transform_->numInstances() : 1u;
}

uint32_t BoundingShape::tfStamp() const {
	return (this->*stampFun_)();
}

void BoundingShape::setTransform(const ref_ptr<ModelTransformation> &transform, uint32_t instanceIndex) {
	transform_ = transform;
	localTransform_ = Mat4f::identity();
	transformIndex_ = instanceIndex;
	updateStampFunction();
}

void BoundingShape::setTransform(const Mat4f &localTransform) {
	localTransform_ = localTransform;
	if (transform_.get()) {
		localStamp_.store(transform_->stamp() + 1u, std::memory_order_relaxed);
		transform_ = {};
		transformIndex_ = 0;
	} else {
		localStamp_.fetch_add(1u, std::memory_order_relaxed);
	}
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
					return ((const BoundingSphere &) *this).hasIntersectionWithSphere((const BoundingSphere &) other);
				case BoundingShapeType::BOX:
					return ((const BoundingSphere &) *this).hasIntersectionWithShape((const BoundingBox &) other);
				case BoundingShapeType::FRUSTUM:
					return ((const Frustum &) other).hasIntersectionWithSphere((const BoundingSphere &) *this);
			}

		case BoundingShapeType::BOX:
			switch (other.shapeType()) {
				case BoundingShapeType::SPHERE:
					return ((const BoundingSphere &) other).hasIntersectionWithShape(*this);
				case BoundingShapeType::BOX:
					return ((const BoundingBox &) *this).hasIntersectionWithBox((const BoundingBox &) other);
				case BoundingShapeType::FRUSTUM:
					return ((const Frustum &) other).hasIntersectionWithBox((const BoundingBox &) *this);
			}

		case BoundingShapeType::FRUSTUM:
			switch (other.shapeType()) {
				case BoundingShapeType::SPHERE:
					return ((const Frustum *) this)->hasIntersectionWithSphere((const BoundingSphere &) other);
				case BoundingShapeType::BOX:
					return ((const Frustum *) this)->hasIntersectionWithBox((const BoundingBox &) other);
				case BoundingShapeType::FRUSTUM:
					return ((const Frustum *) this)->hasIntersectionWithFrustum((const Frustum &) other);
			}
	}
	return false;
}
