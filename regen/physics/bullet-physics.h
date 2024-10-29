
#ifndef BULLET_ANIMATION_H_
#define BULLET_ANIMATION_H_

#include <regen/animations/animation.h>
#include <regen/physics/physical-object.h>
#include <regen/physics/model-matrix-motion.h>

#include <btBulletDynamicsCommon.h>

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

		/**
		 * Adds an object to the physics simulation.
		 * @param object The object.
		 */
		void addObject(const ref_ptr<PhysicalObject> &object);

		auto &dynamicsWorld() { return dynamicsWorld_; }

		// override
		void glAnimate(RenderState *rs, GLdouble dt) override;

		void animate(GLdouble dt) override;

	protected:
		std::list<ref_ptr<PhysicalObject> > objects_;
		ref_ptr<btCollisionDispatcher> dispatcher_;
		ref_ptr<btDefaultCollisionConfiguration> configuration_;
		ref_ptr<btSequentialImpulseConstraintSolver> solver_;
		ref_ptr<btBroadphaseInterface> broadphase_;
		ref_ptr<btDiscreteDynamicsWorld> dynamicsWorld_;
	};
} // namespace

#endif /* BULLET_ANIMATION_H_ */
