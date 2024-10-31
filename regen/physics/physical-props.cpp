
#include "physical-props.h"

using namespace regen;

PhysicalProps::PhysicalProps(
		const ref_ptr<btMotionState> &motionState,
		const ref_ptr<btCollisionShape> &shape)
		: shape_(shape),
		  motionState_(motionState),
		  constructInfo_(0, motionState.get(), shape.get()) {
	constructInfo_.m_restitution = 0;
	constructInfo_.m_friction = 1.5;
}

void PhysicalProps::setMotionState(const ref_ptr<btMotionState> &motionState) {
	motionState_ = motionState;
	constructInfo_.m_motionState = motionState.get();
}

void PhysicalProps::setMassProps(btScalar mass, const btVector3 &inertia) {
	constructInfo_.m_mass = mass;
	constructInfo_.m_localInertia = inertia;
}

void PhysicalProps::setRestitution(btScalar val) { constructInfo_.m_restitution = val; }

void PhysicalProps::setFriction(btScalar val) { constructInfo_.m_friction = val; }

void PhysicalProps::setRollingFriction(btScalar val) { /* constructInfo_.m_rollingFriction = val; */ }

void PhysicalProps::setLinearSleepingThreshold(btScalar val) { constructInfo_.m_linearSleepingThreshold = val; }

void PhysicalProps::setAngularSleepingThreshold(btScalar val) { constructInfo_.m_angularSleepingThreshold = val; }

void PhysicalProps::setAdditionalDamping(bool val) { constructInfo_.m_additionalDamping = val; }

void PhysicalProps::setAdditionalDampingFactor(btScalar val) { constructInfo_.m_additionalDampingFactor = val; }

void PhysicalProps::setLinearDamping(btScalar val) { constructInfo_.m_linearDamping = val; }

void PhysicalProps::setAdditionalLinearDampingThresholdSqr(
		btScalar val) { constructInfo_.m_additionalLinearDampingThresholdSqr = val; }

void PhysicalProps::setAngularDamping(btScalar val) { constructInfo_.m_angularDamping = val; }

void PhysicalProps::setAdditionalAngularDampingFactor(
		btScalar val) { constructInfo_.m_additionalAngularDampingFactor = val; }

void PhysicalProps::setAdditionalAngularDampingThresholdSqr(
		btScalar val) { constructInfo_.m_additionalAngularDampingThresholdSqr = val; }

void PhysicalProps::calculateLocalInertia() {
	shape_->calculateLocalInertia(
			constructInfo_.m_mass,
			constructInfo_.m_localInertia);
}

