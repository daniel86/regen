#ifndef REGEN_CHARACTER_CONTROLLER_H
#define REGEN_CHARACTER_CONTROLLER_H

#include <regen/camera/camera-controller.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include "bullet-physics.h"

namespace regen {
	/**
	 * \brief Character controller.
	 * The character controller is a camera controller that uses a kinematic character controller
	 * to move the camera. The controller is used to move the camera in a first-person or third-person
	 * perspective. The controller also provides collision detection and response.
	 * The character is modeled as a capsule in the physics engine.
	 */
	class CharacterController : public CameraController {
	public:
		/**
		 * @param camera The camera.
		 * @param physics The physics engine.
		 */
		CharacterController(
				const ref_ptr<Camera> &camera,
				const ref_ptr<BulletPhysics> &physics);

		~CharacterController() override;

		CharacterController(const CharacterController &other) = delete;

		/**
		 * @param height The collision height.
		 */
		void setCollisionHeight(GLfloat height) { btCollisionHeight_ = height; }

		/**
		 * @return The collision height.
		 */
		auto collisionHeight() const { return btCollisionHeight_; }

		/**
		 * @param radius The collision radius.
		 */
		void setCollisionRadius(GLfloat radius) { btCollisionRadius_ = radius; }

		/**
		 * @return The collision radius.
		 */
		auto collisionRadius() const { return btCollisionRadius_; }

		/**
		 * @param force The gravity force.
		 */
		void setGravityForce(GLfloat force);

		/**
		 * @param velocity The jump velocity.
		 */
		void setJumpVelocity(GLfloat velocity) { btJumpVelocity_ = velocity; }

		/**
		 * @param height The step height.
		 */
		void setStepHeight(GLfloat height) { btStepHeight_ = height; }

		// override CameraController
		void applyStep(const Vec3f &offset) override;

		// override CameraController
		void jump() override;

	protected:
		ref_ptr<BulletPhysics> bt_;
		ref_ptr<btKinematicCharacterController> btController_;
		ref_ptr<btCollisionShape> btShape_;
		ref_ptr<btCollisionObject> btGhostObject_;
		GLfloat btCollisionHeight_;
		GLfloat btCollisionRadius_;
		GLfloat btStepHeight_;
		GLfloat btGravityForce_;
		GLfloat btJumpVelocity_;
		GLboolean btIsMoving_;

		bool initializePhysics();
	};
}

#endif //REGEN_CHARACTER_CONTROLLER_H
