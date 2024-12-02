
#include "bullet-physics.h"

using namespace regen;

BulletPhysics::BulletPhysics()
		: Animation(GL_FALSE, GL_TRUE) {
	setAnimationName("physics");
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
	dynamicsWorld_->setGravity(btVector3(0, -9.81f, 0));

    // Set the internal ghost pair callback
    ghostPairCallback_ = ref_ptr<btGhostPairCallback>::alloc();
    dynamicsWorld_->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(ghostPairCallback_.get());
}

BulletPhysics::~BulletPhysics() {
	clear();
	dynamicsWorld_ = {};
}

void BulletPhysics::clear() {
	for (auto &x: objects_) {
		dynamicsWorld_->removeRigidBody(x->rigidBody().get());
		for (auto &y: x->props()->collisionObjects()) {
			dynamicsWorld_->removeCollisionObject(y.get());
		}
	}
	objects_.clear();
}

void BulletPhysics::addObject(const ref_ptr<PhysicalObject> &object) {
	auto &props = object->props();
	auto rigidBody = object->rigidBody().get();

    // Determine if the object is static or dynamic
    bool isStatic = (rigidBody->getMass() == 0.0f);

    // Set the appropriate collision filter group and mask
    short group = isStatic ? btBroadphaseProxy::StaticFilter : btBroadphaseProxy::DefaultFilter;
    short mask = btBroadphaseProxy::AllFilter;

    // Add the rigid body to the dynamics world with the correct filter group and mask
    dynamicsWorld_->addRigidBody(rigidBody, group, mask);

	for (auto &x: props->collisionObjects()) {
		dynamicsWorld_->addCollisionObject(x.get());
	}
	objects_.push_back(object);
}

void BulletPhysics::glAnimate(RenderState *rs, GLdouble dt) {}

void BulletPhysics::animate(GLdouble dt) {
	auto timeStep = btScalar(dt / 1000.0);
	auto fixedTimeStep = btScalar(1.0 / 60.0);
	int maxSubSteps = 5;
	if (objects_.empty()) return;

	dynamicsWorld_->stepSimulation(timeStep, maxSubSteps, fixedTimeStep);
}
