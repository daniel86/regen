
#ifndef BULLET_ANIMATION_H_
#define BULLET_ANIMATION_H_

#include <regen/animation/animation.h>
#include <regen/simulation/physical-object.h>
#include <regen/simulation/model-matrix-motion.h>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

namespace regen {
	/**
	 * The physics simulation framework.
	 * Running in regen animation thread.
	 */
	class BulletPhysics : public Animation {
	public:
		/**
		 * Default constructor.
		 */
		BulletPhysics();

		~BulletPhysics() override;

		/**
		 * Clears the physics simulation.
		 */
		void clear();

		/**
		 * Adds an object to the physics simulation.
		 * @param object The object.
		 */
		void addObject(const ref_ptr<PhysicalObject> &object);

		auto &dynamicsWorld() { return dynamicsWorld_; }

		// override
		void cpuUpdate(double dt) override;

	protected:
		std::list<ref_ptr<PhysicalObject> > objects_;
		ref_ptr<btCollisionDispatcher> dispatcher_;
		ref_ptr<btDefaultCollisionConfiguration> configuration_;
		ref_ptr<btSequentialImpulseConstraintSolver> solver_;
		ref_ptr<btBroadphaseInterface> broadphase_;
		ref_ptr<btDiscreteDynamicsWorld> dynamicsWorld_;
		ref_ptr<btGhostPairCallback> ghostPairCallback_;
	};
} // namespace

#endif /* BULLET_ANIMATION_H_ */
