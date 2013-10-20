
#ifndef PHYSICAL_OBJECT_H_
#define PHYSICAL_OBJECT_H_

#include <regen/utility/ref-ptr.h>
#include <regen/bullet/physical-props.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
  class PhysicalObject
  {
  public:
    static ref_ptr<PhysicalObject> createInfiniteWall(
        btScalar planeConstant=0,
        const btVector3 &planeNormal=btVector3(0,1,0),
        const btVector3 &centerOfMassOffset=btVector3(0,0,0));
    static ref_ptr<PhysicalObject> createSphere(
        const ref_ptr<btMotionState> &motion,
        const btScalar &radius,
        const btScalar &mass=1.0f);

    PhysicalObject(const ref_ptr<PhysicalProps> &props);

    const ref_ptr<btRigidBody>& rigidBody();
    const ref_ptr<btCollisionShape>& shape();
    const ref_ptr<btMotionState>& motionState();

  protected:
    /** ... */
    ref_ptr<btRigidBody> rigidBody_;
    /** ... */
    ref_ptr<PhysicalProps> props_;
  };
} // namespace

#endif /* PHYSICAL_OBJECT_H_ */
