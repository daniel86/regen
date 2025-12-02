#ifndef REGEN_KINEMATIC_PLAYER_CONTROLLER_H
#define REGEN_KINEMATIC_PLAYER_CONTROLLER_H

#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include "regen/behavior/user-controller.h"
#include "bullet-physics.h"

namespace regen {
	/**
	 * \brief Character controller.
	 * The character controller is a camera controller that uses a kinematic character controller
	 * to move the camera. The controller is used to move the camera in a first-person or third-person
	 * perspective. The controller also provides collision detection and response.
	 * The character is modeled as a capsule in the physics engine.
	 */
	class KinematicPlayerController : public UserController {
	public:
		/**
		 * @param camera The camera.
		 * @param physics The physics engine.
		 */
		KinematicPlayerController(const ref_ptr<Camera> &camera, const ref_ptr<BulletPhysics> &physics);

		~KinematicPlayerController() override;

		KinematicPlayerController(const KinematicPlayerController &other) = delete;

		/**
		 * @param height The collision height.
		 */
		void setCollisionHeight(float height) { btCollisionHeight_ = height; }

		/**
		 * @return The collision height.
		 */
		auto collisionHeight() const { return btCollisionHeight_; }

		/**
		 * @param radius The collision radius.
		 */
		void setCollisionRadius(float radius) { btCollisionRadius_ = radius; }

		/**
		 * @return The collision radius.
		 */
		auto collisionRadius() const { return btCollisionRadius_; }

		/**
		 * @param force The gravity force.
		 */
		void setGravityForce(float force);

		/**
		 * @param slope The maximum slope.
		 */
		void setMaxSlope(float slope);

		/**
		 * @param velocity The jump velocity.
		 */
		void setJumpVelocity(float velocity) { btJumpVelocity_ = velocity; }

		/**
		 * @param height The step height.
		 */
		void setStepHeight(float height) { btStepHeight_ = height; }

		// override CameraController
		void applyStep(float dt, const Vec3f &offset) override;

		// override CameraController
		void jump() override;

	protected:
		ref_ptr<BulletPhysics> bt_;
		ref_ptr<btKinematicCharacterController> btController_;
		ref_ptr<btCollisionShape> btShape_;
		ref_ptr<btCollisionObject> btGhostObject_;
		float btCollisionHeight_;
		float btCollisionRadius_;
		float btStepHeight_;
		float btGravityForce_;
		float btJumpVelocity_;
		float btMaxSlope_;
		bool btIsMoving_;

		ref_ptr<btActionInterface> btRayAction_;
		btRigidBody *btPlatform_;
		btVector3 previousPlatformPos_;

		bool initializePhysics();
	};
}

#endif //REGEN_KINEMATIC_PLAYER_CONTROLLER_H
