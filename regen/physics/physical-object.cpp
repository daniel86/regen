
#include "physical-object.h"

using namespace regen;

PhysicalObject::PhysicalObject(const ref_ptr<PhysicalProps> &props)
		: props_(props) {
	rigidBody_ = ref_ptr<btRigidBody>::alloc(props_->constructInfo());
	rigidBody_->setGravity(props_->gravity());
}

void PhysicalObject::setMotionState(const ref_ptr<btMotionState> &motionState) {
	props_->setMotionState(motionState);
	rigidBody_->setMotionState(motionState.get());
}
