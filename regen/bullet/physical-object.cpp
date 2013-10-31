
#include "physical-object.h"
using namespace regen;

ref_ptr<PhysicalObject> PhysicalObject::createInfiniteWall(
    btScalar planeConstant,
    const btVector3 &planeNormal,
    const btVector3 &centerOfMassOffset)
{
  btTransform initialTransform(
      btQuaternion(0,0,0,1),
      centerOfMassOffset);
  ref_ptr<PhysicalProps> props = ref_ptr<PhysicalProps>::alloc(
      // synchronizing transformations (just a dummy for planes)
      ref_ptr<btDefaultMotionState>::alloc(initialTransform),
      // the physical shape
      ref_ptr<btStaticPlaneShape>::alloc(planeNormal,planeConstant)
  );
  // 0 -> Static object, infinite mass, will never move.
  props->setMassProps(0, btVector3(0,0,0));
  props->setRestitution(0.7f);
  props->setFriction(1.5f);
  return ref_ptr<PhysicalObject>::alloc(props);
}

ref_ptr<PhysicalObject> PhysicalObject::createBox(
    const ref_ptr<btMotionState> &motion,
    const btVector3 &halfExtend, const btScalar &mass)
{
  ref_ptr<PhysicalProps> props = ref_ptr<PhysicalProps>::alloc(
      motion, ref_ptr<btBoxShape>::alloc(halfExtend));
  props->setMassProps(mass, btVector3(0,0,0));
  props->setRestitution(0.7f);
  props->setFriction(1.5f);
  props->calculateLocalInertia();
  return ref_ptr<PhysicalObject>::alloc(props);
}

ref_ptr<PhysicalObject> PhysicalObject::createSphere(
    const ref_ptr<btMotionState> &motion,
    const btScalar &radius, const btScalar &mass)
{
  ref_ptr<PhysicalProps> props = ref_ptr<PhysicalProps>::alloc(
      motion, ref_ptr<btSphereShape>::alloc(radius));
  props->setMassProps(mass, btVector3(0,0,0));
  props->setRestitution(0.7f);
  props->setFriction(1.5f);
  props->calculateLocalInertia();
  return ref_ptr<PhysicalObject>::alloc(props);
}

PhysicalObject::PhysicalObject(const ref_ptr<PhysicalProps> &props)
: props_(props)
{ rigidBody_ = ref_ptr<btRigidBody>::alloc(props_->constructInfo()); }

const ref_ptr<btRigidBody>& PhysicalObject::rigidBody()
{ return rigidBody_; }
const ref_ptr<btCollisionShape>& PhysicalObject::shape()
{ return props_->shape(); }
const ref_ptr<btMotionState>& PhysicalObject::motionState()
{ return props_->motionState(); }
