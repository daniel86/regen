#include "character-controller.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

#include <utility>

using namespace regen;

/**
 * Action to detect the platform the character is standing on.
 */
class PlatformRayAction : public btActionInterface {
public:
    PlatformRayAction(
    		btDynamicsWorld* dynamicsWorld,
    		btCollisionObject* ghostObject,
    		GLfloat heightAdjust,
			const std::function<void(btRigidBody*)> &callback) :
    		ghostObject_(ghostObject),
    		dynamicsWorld_(dynamicsWorld),
			heightAdjust_(0, heightAdjust + 0.1f, 0),
    		callback_(callback),
    		platform_(nullptr)
    {}

    void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep) override {
        auto &rayStart = ghostObject_->getWorldTransform().getOrigin();
		auto rayEnd = rayStart - heightAdjust_;

        btCollisionWorld::ClosestRayResultCallback rayCallback(rayStart, rayEnd);
        dynamicsWorld_->rayTest(rayStart, rayEnd, rayCallback);

        if (rayCallback.hasHit()) {
            auto *platform = const_cast<btRigidBody*>(btRigidBody::upcast(rayCallback.m_collisionObject));
            if (platform) {
				if (platform_ != platform && platform->getMass() == 0.0) {
					platform_ = platform;
					callback_(platform_);
					return;
				}
			}
        }
        if (platform_) {
			platform_ = nullptr;
			callback_(platform_);
		}
    }

    void debugDraw(btIDebugDraw* debugDrawer) override {}

private:
    btCollisionObject* ghostObject_;
    btDynamicsWorld* dynamicsWorld_;
	btVector3 heightAdjust_;
	std::function<void(btRigidBody*)> callback_;
	btRigidBody *platform_;
};

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
		btIsMoving_(GL_FALSE),
		btPlatform_(nullptr) {
}

CharacterController::~CharacterController() {
	if (btGhostObject_.get() != nullptr) {
		bt_->dynamicsWorld()->removeCollisionObject(btGhostObject_.get());
		btGhostObject_ = ref_ptr<btPairCachingGhostObject>();
	}
	if (btController_.get() != nullptr) {
		bt_->dynamicsWorld()->removeAction(btController_.get());
		btController_ = ref_ptr<btKinematicCharacterController>();
	}
	if (btRayAction_.get() != nullptr) {
		bt_->dynamicsWorld()->removeAction(btRayAction_.get());
		btRayAction_ = ref_ptr<PlatformRayAction>();
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
	// find platform the player is standing on
	btRayAction_ = ref_ptr<PlatformRayAction>::alloc(
		bt_->dynamicsWorld().get(),
		btGhostObject_.get(),
		btCollisionHeight_ * 0.5f + btCollisionRadius_,
		[this](btRigidBody *btPlatform) {
			btPlatform_ = btPlatform;
			if (btPlatform) {
				previousPlatformPos_ = btPlatform->getWorldTransform().getOrigin();
			}
		});

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
	bt_->dynamicsWorld()->addAction(btRayAction_.get());

	return true;
}

void CharacterController::jump() {
	if (btController_.get()) {
		btController_->jump(btVector3(0.0, btJumpVelocity_, 0.0));
	}
}

void CharacterController::applyStep(GLfloat dt, const Vec3f &offset) {
	// Make a step. This is called each frame.

	if (dt <= std::numeric_limits<float>::epsilon()) {
		return;
	}

	if (!btController_.get()) {
		if (!initializePhysics()) {
			return;
		}
	}

	// synchronize the ghost object's transform with the OpenGL transform
	btTransform ghostTransform = btController_->getGhostObject()->getWorldTransform();
	ghostTransform.getOpenGLMatrix((btScalar *) &matVal_);
	// decrease the y position by the collision height, else
	// the character would float above the ground
	GLfloat heightAdjust = btCollisionHeight_ * 0.5f + btCollisionRadius_;
	matVal_.x[13] -= heightAdjust;

	btVector3 btVelocity = btController_->getLinearVelocity();
	btVelocity.setX(0.0);
	btVelocity.setZ(0.0);
	GLboolean isMoving = isMoving_;

	// Apply the platform velocity to the character controller
    if (btPlatform_) {
		btVector3 currentPlatformPos = btPlatform_->getWorldTransform().getOrigin();
		btVector3 platformVelocity = currentPlatformPos - previousPlatformPos_;
		if (platformVelocity.length() > 0.0001) {
			// not sure about the magic number 16.8, might
			// be related to some hacks in the kinematics controller :/
			btVelocity += 16.8 * platformVelocity / dt;
			isMoving = GL_TRUE;
		}
		previousPlatformPos_ = currentPlatformPos;
   }

	if (!isMoving_) {
		if (btIsMoving_ || isMoving) {
			btVelocity.setY(btController_->getLinearVelocity().getY());
			btController_->setLinearVelocity(btVelocity);
			btIsMoving_ = GL_FALSE;
		}
	} else {
		// Apply the step to the character controller
		btVelocity += btVector3(offset.x, offset.y, offset.z)/dt;
		btVelocity.setY(btController_->getLinearVelocity().getY());
		btController_->setLinearVelocity(btVelocity);
		btIsMoving_ = GL_TRUE;
	}
}
