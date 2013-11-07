
#ifndef PHYSICAL_OBJECT_H_
#define PHYSICAL_OBJECT_H_

#include <regen/utility/ref-ptr.h>
#include <regen/physics/physical-props.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
  /**
   * Object in physics simulation.
   */
  class PhysicalObject
  {
  public:
    /**
     * Default constructor.
     * @param props Properrties used to create this object.
     */
    PhysicalObject(const ref_ptr<PhysicalProps> &props);

    /**
     * @return Rigid body representation.
     */
    const ref_ptr<btRigidBody>& rigidBody();
    /**
     * @return The collision shape.
     */
    const ref_ptr<btCollisionShape>& shape();
    /**
     * @return Motion state for transform synchronization.
     */
    const ref_ptr<btMotionState>& motionState();

  protected:
    ref_ptr<btRigidBody> rigidBody_;
    ref_ptr<PhysicalProps> props_;
  };
} // namespace

#endif /* PHYSICAL_OBJECT_H_ */
