
#include "physical-object.h"

using namespace regen;

PhysicalObject::PhysicalObject(const ref_ptr<PhysicalProps> &props)
		: props_(props) { rigidBody_ = ref_ptr<btRigidBody>::alloc(props_->constructInfo()); }

const ref_ptr<btRigidBody> &PhysicalObject::rigidBody() { return rigidBody_; }

const ref_ptr<btCollisionShape> &PhysicalObject::shape() { return props_->shape(); }

const ref_ptr<btMotionState> &PhysicalObject::motionState() { return props_->motionState(); }
