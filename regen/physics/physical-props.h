
#ifndef PHYSICAL_PROPS_H_
#define PHYSICAL_PROPS_H_

#include <regen/utility/ref-ptr.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
  /**
   * Properties for a single physical object.
   */
  class PhysicalProps
  {
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
    const ref_ptr<btCollisionShape>& shape();
    /**
     * @return The motion state.
     */
    const ref_ptr<btMotionState>& motionState();
    /**
     * @return Rigid body construct info.
     */
    const btRigidBody::btRigidBodyConstructionInfo& constructInfo();

  protected:
    btRigidBody::btRigidBodyConstructionInfo constructInfo_;
    ref_ptr<btRigidBody> rigidBody_;
    ref_ptr<btCollisionShape> shape_;
    ref_ptr<btMotionState> motionState_;
  };
} // namespace

#endif /* PHYSICAL_PROPS_H_ */
