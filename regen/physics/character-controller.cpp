#include "character-controller.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

using namespace regen;

CharacterController::CharacterController(
		const ref_ptr<Camera> &camera,
		const ref_ptr<BulletPhysics> &physics) :
		CameraController(camera),
		bt_(physics),
		btCollisionHeight_(0.8f),
		btCollisionRadius_(0.8f),
		btStepHeight_(0.25f),
		btGravityForce_(30.0f),
		btJumpVelocity_(16.0f),
		btIsMoving_(GL_FALSE) {
}

CharacterController::~CharacterController() {
	// Remove the collision object from the Bullet world
	if (btGhostObject_.get() != nullptr) {
		bt_->dynamicsWorld()->removeCollisionObject(btGhostObject_.get());
		btGhostObject_ = ref_ptr<btPairCachingGhostObject>();
	}
	// Remove the character controller from the Bullet world
	if (btController_.get() != nullptr) {
		bt_->dynamicsWorld()->removeAction(btController_.get());
		btController_ = ref_ptr<btKinematicCharacterController>();
	}
}

void CharacterController::setGravityForce(GLfloat force) {
	btGravityForce_ = force;
	if (btController_.get()) {
		btController_->setGravity(btVector3(0, -btGravityForce_, 0));
	}
}

bool CharacterController::initializePhysics() {
	if (!attachedToTransform_.get()) {
		return false;
	}

	// Create the collision shape for the character
	auto capsuleShape = ref_ptr<btCapsuleShape>::alloc(
			btCollisionRadius_, btCollisionHeight_);
	btShape_ = capsuleShape;

	// Create the ghost object for the character
	auto ghostObject = ref_ptr<btPairCachingGhostObject>::alloc();
	ghostObject->setCollisionShape(btShape_.get());
	ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
	btGhostObject_ = ghostObject;

	// Create the character controller
	btController_ = ref_ptr<btKinematicCharacterController>::alloc(
			ghostObject.get(), capsuleShape.get(), btStepHeight_);
	setGravityForce(btGravityForce_);

	btQuaternion rotation;
	rotation.setRotation(btVector3(0, 1, 0), meshHorizontalOrientation_);
	btTransform initialTransform;
	initialTransform.setFromOpenGLMatrix((btScalar *) attachedToTransform_->clientDataPtr());
	initialTransform.setRotation(rotation);
	ghostObject->setWorldTransform(initialTransform);

	// Add the character controller to the Bullet world
	bt_->dynamicsWorld()->addCollisionObject(
			btGhostObject_.get(),
			btBroadphaseProxy::CharacterFilter,
			btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
	bt_->dynamicsWorld()->addAction(btController_.get());

	return true;
}

void CharacterController::jump() {
	if (btController_.get()) {
		btController_->jump(btVector3(0.0, btJumpVelocity_, 0.0));
	}
}

void CharacterController::applyStep(const Vec3f &offset) {
	// Make a step. This is called each frame.

	if (!btController_.get()) {
		if (!initializePhysics()) {
			return;
		}
	}

	// synchronize the ghost object's transform with the OpenGL transform
	btTransform btTransform = btController_->getGhostObject()->getWorldTransform();
	btTransform.getOpenGLMatrix((btScalar *) &matVal_);
	// decrease the y position by the collision height, else
	// the character would float above the ground
	matVal_.x[13] -= btCollisionHeight_ * 0.5f + btCollisionRadius_;

	if (!isMoving_) {
		if (btIsMoving_) {
			btController_->setWalkDirection(btVector3(0, 0, 0));
			btIsMoving_ = GL_FALSE;
		}
	} else {
		// Apply the step to the character controller
		btController_->setWalkDirection(btVector3(offset.x, offset.y, offset.z));
		btIsMoving_ = GL_TRUE;
	}
}
