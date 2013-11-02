
#ifndef PHYSICAL_PROPS_H_
#define PHYSICAL_PROPS_H_

#include <regen/utility/ref-ptr.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
  class PhysicalProps
  {
  public:
    PhysicalProps(
        const ref_ptr<btMotionState> &motionState,
        const ref_ptr<btCollisionShape> &shape);

    void setMassProps(btScalar mass, const btVector3 &inertia);

    // best simulation results using zero restitution.
    void setRestitution(btScalar val);

    // best simulation results when friction is non-zero
    void setFriction(btScalar val);
    // the m_rollingFriction prevents rounded shapes, such as spheres,
    // cylinders and capsules from rolling forever.
    void setRollingFriction(btScalar val);

    void setLinearSleepingThreshold(btScalar val);
    void setAngularSleepingThreshold(btScalar val);

    // Additional damping can help avoiding lowpass jitter motion,
    // help stability for ragdolls etc.
    // Such damping is undesirable, so once the overall simulation
    // quality of the rigid body dynamics system has improved, this should become obsolete
    void setAdditionalDamping(bool val);
    void setAdditionalDampingFactor(btScalar val);

    void setLinearDamping(btScalar val);
    void setAdditionalLinearDampingThresholdSqr(btScalar val);

    void setAngularDamping(btScalar val);
    void setAdditionalAngularDampingFactor(btScalar val);
    void setAdditionalAngularDampingThresholdSqr(btScalar val);

    void calculateLocalInertia();

    const ref_ptr<btCollisionShape>& shape();
    const ref_ptr<btMotionState>& motionState();
    const btRigidBody::btRigidBodyConstructionInfo& constructInfo();

  protected:
    /** ... */
    btRigidBody::btRigidBodyConstructionInfo constructInfo_;
    /** ... */
    ref_ptr<btRigidBody> rigidBody_;
    /** ... */
    ref_ptr<btCollisionShape> shape_;
    /** ... */
    ref_ptr<btMotionState> motionState_;
  };
} // namespace

#endif /* PHYSICAL_PROPS_H_ */
