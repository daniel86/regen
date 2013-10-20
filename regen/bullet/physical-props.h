
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
    void setRestitution(btScalar val);
    void setFriction(btScalar val);

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
