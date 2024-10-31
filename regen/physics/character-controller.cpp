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
		btMaxSlope_(0.8f),
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


void CharacterController::setMaxSlope(GLfloat maxSlope) {
	btMaxSlope_ = maxSlope;
	if (btController_.get()) {
		btController_->setMaxSlope(btMaxSlope_);
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
	setMaxSlope(btMaxSlope_);


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

void CharacterController::applyStep(GLfloat dt, const Vec3f &offset) {
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
	GLfloat heightAdjust = btCollisionHeight_ * 0.5f + btCollisionRadius_;
	matVal_.x[13] -= heightAdjust;

	btVector3 btVelocity = btController_->getLinearVelocity();
	btVelocity.setX(0.0);
	btVelocity.setZ(0.0);
	GLboolean isMoving = isMoving_;

	// Perform a raycast to detect the platform
    btVector3 rayStart = btTransform.getOrigin();
    btVector3 rayEnd = rayStart + btVector3(0, -heightAdjust  - 0.1f, 0);
    btCollisionWorld::ClosestRayResultCallback rayCallback(rayStart, rayEnd);
    bt_->dynamicsWorld()->rayTest(rayStart, rayEnd, rayCallback);

    if (rayCallback.hasHit()) {
        auto hitBody = const_cast<btRigidBody*>(
        		btRigidBody::upcast(rayCallback.m_collisionObject));
        if (hitBody) {
            auto &bodyVelocity = hitBody->getLinearVelocity();
            if (bodyVelocity.length() > 0.0001) {
				btVelocity += bodyVelocity;
				isMoving = GL_TRUE;
			}
        }
    }

	if (!isMoving_) {
		if (btIsMoving_ || isMoving) {
			btVelocity.setY(btController_->getLinearVelocity().getY());
			btController_->setLinearVelocity(btVelocity);
			btIsMoving_ = GL_FALSE;
		}
	} else {
		// Apply the step to the character controller
		btVelocity += btVector3(offset.x, offset.y, offset.z);
		btVelocity.setY(btController_->getLinearVelocity().getY());
		btController_->setLinearVelocity(btVelocity);
		btIsMoving_ = GL_TRUE;
	}
}
