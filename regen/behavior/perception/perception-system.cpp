#include "perception-system.h"

using namespace regen;

namespace regen {
	static constexpr float PERCEPTION_DEFAULT_ASPECT = 1.0f;
	static constexpr float PERCEPTION_DEFAULT_FOV = 60.0f;
	static constexpr float PERCEPTION_DEFAULT_NEAR = 0.1f;
	static constexpr float PERCEPTION_DEFAULT_FAR = 15.0f;
}

PerceptionSystem::PerceptionSystem(
			const ref_ptr<SpatialIndex> &spatialIndex,
			const ref_ptr<BoundingShape> &indexedShape) :
	spatialIndex_(spatialIndex),
	indexedShape_(indexedShape)
{
	auto &tf = indexedShape_->transform();
	collisionEvt_.data.self = indexedShape_.get();

	// create the initial collision shape
	collisionShape_ = ref_ptr<Frustum>::alloc();
	collisionShape_->setPerspective(
		PERCEPTION_DEFAULT_ASPECT,
		PERCEPTION_DEFAULT_FOV,
		PERCEPTION_DEFAULT_NEAR,
		PERCEPTION_DEFAULT_FAR);
	collisionShape_->setTransform(tf, indexedShape->instanceID());
	updateCollisionShape(true);

	// include the collision shape in debug drawing
	spatialIndex_->addDebugShape(collisionShape_);
}

void PerceptionSystem::updateCollisionShape(bool forceUpdate) {
	auto nextStamp = indexedShape_->tfStamp();
	if (!forceUpdate && lastCollisionShapeStamp_ == nextStamp) {
		return;
	}
	lastCollisionShapeStamp_ = nextStamp;

	// update the collision shape transform
	if (const auto tf = indexedShape_->transform(); tf->hasModelMat()) {
		const auto modalMat = tf->modelMat()->getVertex(indexedShape_->instanceID());
		const Vec3f tfPos = modalMat.r.position() + eyeOffset_;
		collisionShape_->update(tfPos, modalMat.r.direction());
	} else {
		const Vec3f tfPos = tf->position(indexedShape_->instanceID()).r + eyeOffset_;
		collisionShape_->update(tfPos, Vec3f::front());
	}
	collisionShape_->updateOrthogonalProjection();
}

void PerceptionSystem::addMonitor(PerceptionMonitor *monitor) {
	if (monitor->type == PerceptionEventType::COLLISION) {
		collisionMonitors_.push_back(static_cast<CollisionMonitor*>(monitor));
	} else if (monitor->type == PerceptionEventType::DETECTION) {
		detectionMonitors_.push_back(static_cast<DetectionMonitor*>(monitor));
	} else {
		REGEN_WARN("Ignoring monitor with unknown type.");
	}
}

void PerceptionSystem::removeMonitor(PerceptionMonitor *monitor) {
	if (monitor->type == PerceptionEventType::COLLISION) {
		auto it = std::find(collisionMonitors_.begin(), collisionMonitors_.end(), monitor);
		if (it != collisionMonitors_.end()) collisionMonitors_.erase(it);
	} else if (monitor->type == PerceptionEventType::DETECTION) {
		auto it = std::find(detectionMonitors_.begin(), detectionMonitors_.end(), monitor);
		if (it != detectionMonitors_.end()) detectionMonitors_.erase(it);
	} else {
		REGEN_WARN("Ignoring monitor with unknown type.");
	}
}

void PerceptionSystem::handleIntersection(const BoundingShape &other) {
	if (indexedShape_.get() == &other) return; // skip self
	const Vec3f &thisCenter = indexedShape_->tfOrigin();
	const Vec3f &otherCenter = other.tfOrigin();
	auto &collisionData = collisionEvt_.data;

	// Compute perception data.
	collisionData.other = &other;
	collisionData.delta = thisCenter - otherCenter;
	collisionData.distance = collisionData.delta.length();
	collisionData.dir = collisionData.delta / (collisionData.distance + 1e-6f);
	collisionData.isBehind = (Vec3f::front().dot(collisionData.delta) < 0.0f);
	if (collisionData.distance < 0.01f) return;

	// Notify all monitors.
	for (auto &monitor : collisionMonitors_) {
		monitor->emitCollisionEvent(collisionEvt_);
	}
}

void PerceptionSystem::updateCollisions() {
	// Initialize monitors
	for (auto &monitor : collisionMonitors_) {
		monitor->initializeCollisionFrame();
	}
	// Update the collision shape transform + projection
	updateCollisionShape(false);
	auto &hits = spatialIndex_->foreachIntersection(*collisionShape_.get(), collisionMask_);
	for (uint32_t hitIdx = 0; hitIdx < hits.count; ++hitIdx) {
		auto &b_shape = spatialIndex_->itemShape(hits.data[hitIdx]);
		handleIntersection(*b_shape.get());
	}
	// Cleanup monitors
	for (auto &monitor : collisionMonitors_) {
		monitor->finalizeCollisionFrame();
	}
}

void PerceptionSystem::update(const Blackboard &kb, double dt_s) {
	updateCollisions();
}
