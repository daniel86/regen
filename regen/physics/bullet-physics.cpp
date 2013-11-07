
#include "bullet-physics.h"
using namespace regen;

BulletPhysics::BulletPhysics()
: Animation(GL_FALSE,GL_TRUE)
{
  // Build the broadphase
  broadphase_ = ref_ptr<btDbvtBroadphase>::alloc();
  // Set up the collision configuration and dispatcher
  configuration_ = ref_ptr<btDefaultCollisionConfiguration>::alloc();
  dispatcher_ = ref_ptr<btCollisionDispatcher>::alloc(configuration_.get());
  // The actual physics solver
  solver_ = ref_ptr<btSequentialImpulseConstraintSolver>::alloc();
  // The world.
  dynamicsWorld_ = ref_ptr<btDiscreteDynamicsWorld>::alloc(
      dispatcher_.get(), broadphase_.get(),
      solver_.get(), configuration_.get());
  dynamicsWorld_->setGravity(btVector3(0,-9.81f,0));
}

void BulletPhysics::addObject(const ref_ptr<PhysicalObject> &object)
{
  dynamicsWorld_->addRigidBody(object->rigidBody().get());
  objects_.push_back(object);
}

void BulletPhysics::glAnimate(RenderState *rs, GLdouble dt)
{}

void BulletPhysics::animate(GLdouble dt)
{
  btScalar timeStep = btScalar(dt/1000.0);
  btScalar fixedTimeStep = btScalar(1.0/60.0);
  int maxSubSteps = 5;

  dynamicsWorld_->stepSimulation(timeStep,maxSubSteps,fixedTimeStep);
}
