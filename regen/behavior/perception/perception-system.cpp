#include "perception-system.h"

#include "regen/shapes/obb.h"

using namespace regen;

PerceptionSystem::PerceptionSystem(
			const ref_ptr<SpatialIndex> &spatialIndex,
			const ref_ptr<BoundingShape> &indexedShape) :
	spatialIndex_(spatialIndex),
	indexedShape_(indexedShape)
{
	auto &mesh = indexedShape_->baseMesh();
	auto &tf = indexedShape_->transform();
	collisionEvt_.data.self = indexedShape_.get();

	// create the initial collision shape
	// TODO: Experiment with form of shape for collision detection.
	//     - best: frustum, but we might get away with a box or sphere.
	//     - frustum actually better for testing
	float lookAheadDistance = 15.0f;
	Bounds<Vec3f> collisionBounds = Bounds<Vec3f>::create(mesh->minPosition(), mesh->maxPosition());
	collisionBounds.min.z -= lookAheadDistance;
	collisionBounds.min.x -= 0.0f;
	collisionBounds.max.x += 0.0f;
	collisionBounds.min.y -= 1.0f;
	collisionBounds.max.y += 1.0f;
	collisionShape_ = ref_ptr<OBB>::alloc(collisionBounds);
	collisionShape_->setTransform(tf, indexedShape->instanceID());
	collisionShape_->updateTransform(true);
	spatialIndex_->addDebugShape(collisionShape_);
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
	// Update the collision shape transform
	collisionShape_->updateTransform(false);
	collisionShape_->updateOrthogonalProjection();
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
