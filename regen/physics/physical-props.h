
#ifndef PHYSICAL_PROPS_H_
#define PHYSICAL_PROPS_H_

#include <vector>
#include <regen/utility/ref-ptr.h>

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btCharacterControllerInterface.h>

namespace regen {
	/**
	 * Properties for a single physical object.
	 */
	class PhysicalProps {
	public:
		/**
		 * @param motionState Used for transform synchronization.
		 * @param shape Used for collision detection.
		 */
		PhysicalProps(
				const ref_ptr<btMotionState> &motionState,
				const ref_ptr<btCollisionShape> &shape);

		/**
		 * @param mass The object mass.
		 * @param inertia The initial inertia.
		 */
		void setMassProps(btScalar mass, const btVector3 &inertia);

		/**
		 * Best simulation results using zero restitution.
		 * @param val The restitution value.
		 */
		void setRestitution(btScalar val);

		/**
		 * Best simulation results when friction is non-zero.
		 * @param val The friction value.
		 */
		void setFriction(btScalar val);

		/**
		 * The m_rollingFriction prevents rounded shapes, such as spheres,
		 * cylinders and capsules from rolling forever.
		 */
		void setRollingFriction(btScalar val);

		/**
		 * @param val The linear sleeping threshold value.
		 */
		void setLinearSleepingThreshold(btScalar val);

		/**
		 * @param val The angular sleeping threshold value.
		 */
		void setAngularSleepingThreshold(btScalar val);

		/**
		 * Additional damping can help avoiding lowpass jitter motion,
		 * help stability for ragdolls etc.
		 * @param val True if additional damping should be used.
		 */
		void setAdditionalDamping(bool val);

		/**
		 * Additional damping can help avoiding lowpass jitter motion,
		 * help stability for ragdolls etc.
		 * @param val The damping value.
		 */
		void setAdditionalDampingFactor(btScalar val);

		/**
		 * @param val The damping value.
		 */
		void setLinearDamping(btScalar val);

		/**
		 * @param val The damping threshold.
		 */
		void setAdditionalLinearDampingThresholdSqr(btScalar val);

		/**
		 * @param val The damping value.
		 */
		void setAngularDamping(btScalar val);

		/**
		 * @param val The damping factor.
		 */
		void setAdditionalAngularDampingFactor(btScalar val);

		/**
		 * @param val The damping threshold.
		 */
		void setAdditionalAngularDampingThresholdSqr(btScalar val);

		/**
		 * Calculate local inertia after configuring the object.
		 */
		void calculateLocalInertia();

		/**
		 * @return The collision shape.
		 */
		auto &shape() { return shape_; }

		/**
		 * @return The motion state.
		 */
		auto &motionState() { return motionState_; }

		/**
		 * Set the motion state.
		 * @param motionState The motion state.
		 */
		void setMotionState(const ref_ptr<btMotionState> &motionState);

		/**
		 * @return Rigid body construct info.
		 */
		auto &constructInfo() { return constructInfo_; }

		/**
		 * Adds a collision object to the list.
		 * @param object The collision object.
		 */
		void addCollisionObject(const ref_ptr<btCollisionObject> &object) { collisionObjects_.push_back(object); }

		/**
		 * @return The list of collision objects.
		 */
		auto &collisionObjects() const { return collisionObjects_; }

		/**
		 * @return The gravity vector.
		 */
		auto gravity() const { return gravity_; }

		/**
		 * @param gravity The gravity vector.
		 */
		void setGravity(const btVector3 &gravity) { gravity_ = gravity; }

	protected:
		ref_ptr<btCollisionShape> shape_;
		ref_ptr<btMotionState> motionState_;
		btRigidBody::btRigidBodyConstructionInfo constructInfo_;
		std::vector<ref_ptr<btCollisionObject> > collisionObjects_;
		btVector3 gravity_;
	};
} // namespace

#endif /* PHYSICAL_PROPS_H_ */
