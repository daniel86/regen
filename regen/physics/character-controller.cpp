#include "character-controller.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>

using namespace regen;

namespace regen {
	/**
	 * Motion state for the character controller.
	 */
	class CharacterMotionState : public btMotionState {
	public:
		explicit CharacterMotionState(
				CharacterController *controller,
				const ref_ptr<ShaderInputMat4> &glTransform,
				GLuint glTransformIndex = 0)
				: controller_(controller),
				  glTransform_(glTransform),
				  glTransformIndex_(glTransformIndex) {}

		void getWorldTransform(btTransform &worldTrans) const override {
			auto *matrices = (Mat4f *) glTransform_->clientDataPtr();
			auto *mat = &matrices[glTransformIndex_];
			worldTrans.setFromOpenGLMatrix((btScalar *) mat);
		}

		void setWorldTransform(const btTransform &worldTrans) override {
			auto *matrices = (Mat4f *) glTransform_->clientDataPtr();
			auto *mat = &matrices[glTransformIndex_];
			auto cy = mat->x[0], sy = mat->x[2];
			worldTrans.getOpenGLMatrix((btScalar *) mat);
			mat->x[0] = cy;
			mat->x[2] = sy;
			mat->x[8] = -sy;
			mat->x[10] = cy;
			// decrease the y position by the collision height
			mat->x[13] -= controller_->collisionHeight() * 0.5 + controller_->collisionRadius();
			glTransform_->nextStamp();
		}

	private:
		CharacterController *controller_;
		ref_ptr<ShaderInputMat4> glTransform_;
		GLuint glTransformIndex_;
	};
}

CharacterController::CharacterController(
		const ref_ptr<Camera> &camera,
		const ref_ptr<BulletPhysics> &physics) :
		CameraController(camera),
		bt_(physics),
		btCollisionHeight_(0.8f),
		btCollisionRadius_(0.8f),
		btStepHeight_(0.25f),
		btGravityForce_(30.0f),
		btJumpVelocity_(16.0f) {
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

	// Create the motion state for the character
	btMotionState_ = ref_ptr<CharacterMotionState>::alloc(this, attachedToTransform_);
	// Create the character controller
	btController_ = ref_ptr<btKinematicCharacterController>::alloc(
			ghostObject.get(), capsuleShape.get(), btStepHeight_);
	btController_->setGravity(btVector3(0, -btGravityForce_, 0));

	btTransform initialTransform;
	initialTransform.setFromOpenGLMatrix((btScalar *) attachedToTransform_->clientDataPtr());
	btQuaternion rotation;
	rotation.setRotation(btVector3(0, 1, 0), meshHorizontalOrientation_);
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
	// react on key press input. the offset is the direction of movement.
	// in "direct" mode, the camera is moved directly to the new position.

	if (!btController_.get()) {
		return;
	}

	// Convert the offset to a Bullet vector
	btVector3 walkDirection(offset.x, offset.y, offset.z);
	// Apply the step to the character controller
	btController_->setWalkDirection(walkDirection);
}

void CharacterController::animate(GLdouble dt) {
	if (!btController_.get()) {
		if (!initializePhysics()) {
			return;
		}
	}

	CameraController::animate(dt);

	if (btController_.get()) {
		// Manually update the ghost object's transform
		btTransform characterTransform = btController_->getGhostObject()->getWorldTransform();
		btMotionState_->setWorldTransform(characterTransform);
	}
}
