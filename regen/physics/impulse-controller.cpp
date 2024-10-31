#include "impulse-controller.h"
#include "model-matrix-motion.h"

using namespace regen;

ImpulseController::ImpulseController(
			const ref_ptr<Camera> &cam,
			const ref_ptr<PhysicalObject> &physicalObject) :
		CameraController(cam),
		physicalObject_(physicalObject),
		physicsSpeedFactor_(1.0) {
	// read transform from physical object
	btTransform transform;
	physicalObject->motionState()->getWorldTransform(transform);
	// create a custom motion state where we can do synchronization with
	// updating the camera transform which depends on the physical object.
	auto motionState = ref_ptr<Mat4fMotion>::alloc(&matVal_);
	transform.getOpenGLMatrix((btScalar *) matVal_.x);
	physicalObject->setMotionState(motionState);
	physicalObject->rigidBody()->setLinearVelocity(btVector3(0, 0, 0));
}

void ImpulseController::applyStep(const Vec3f &offset) {
	// update orientation of the physical object: set it to the camera orientation
	btTransform transform;
	btQuaternion rotation;
	physicalObject_->rigidBody()->getMotionState()->getWorldTransform(transform);
	rotation.setRotation(btVector3(0, 1, 0), horizontalOrientation_);
	transform.setRotation(rotation);
	physicalObject_->rigidBody()->getMotionState()->setWorldTransform(transform);

	transform.getOpenGLMatrix((btScalar *) &matVal_);

	if (!isMoving_) return;

	// apply impulse to the physical object
	btVector3 impulse(
		step_.x * physicsSpeedFactor_,
		step_.y * physicsSpeedFactor_,
		step_.z * physicsSpeedFactor_);
	physicalObject_->rigidBody()->activate(true);
	physicalObject_->rigidBody()->applyCentralImpulse(impulse);
}
