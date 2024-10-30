#include "impulse-controller.h"

using namespace regen;

ImpulseController::ImpulseController(
			const ref_ptr<Camera> &cam,
			const ref_ptr<PhysicalObject> &physicalObject) :
		CameraController(cam),
		physicalObject_(physicalObject),
		physicsSpeedFactor_(1.0) {
	btTransform transform;
	physicalObject->motionState()->getWorldTransform(transform);
	transform.getOpenGLMatrix((btScalar *) matVal_.x);
	physicalObject->rigidBody()->setLinearVelocity(btVector3(0, 0, 0));
}

void ImpulseController::applyStep(const Vec3f &offset) {
	if (!isMoving_) return;

	// apply impulse to the physical object
	btVector3 impulse(
		step_.x * physicsSpeedFactor_,
		step_.y * physicsSpeedFactor_,
		step_.z * physicsSpeedFactor_);
	physicalObject_->rigidBody()->activate(true);
	physicalObject_->rigidBody()->applyCentralImpulse(impulse);

	// update orientation of the physical object: set it to the camera orientation
	btTransform transform;
	btQuaternion rotation;
	physicalObject_->rigidBody()->getMotionState()->getWorldTransform(transform);
	rotation.setRotation(btVector3(0, 1, 0), horizontalOrientation_);
	transform.setRotation(rotation);
	physicalObject_->rigidBody()->getMotionState()->setWorldTransform(transform);
}
