
#ifndef BULLET_ANIMATION_H_
#define BULLET_ANIMATION_H_

#include <regen/animations/animation.h>
#include <regen/physics/physical-object.h>
#include <regen/physics/model-matrix-motion.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
  class BulletPhysics : public Animation
  {
  public:
    /**
     * @param cam the camera to manipulate
     */
    BulletPhysics();

    void addObject(const ref_ptr<PhysicalObject> &object);
    void addWall(
        const GLfloat &width,
        const GLfloat &depth,
        const btVector3 &pos=btVector3(0,0,0));

    // override
    void glAnimate(RenderState *rs, GLdouble dt);
    void animate(GLdouble dt);

  protected:
    list< ref_ptr<PhysicalObject> > objects_;
    /** ... */
    ref_ptr<btCollisionDispatcher> dispatcher_;
    /** ... */
    ref_ptr<btDefaultCollisionConfiguration> configuration_;
    /** ... */
    ref_ptr<btSequentialImpulseConstraintSolver> solver_;
    /** ... */
    ref_ptr<btBroadphaseInterface> broadphase_;
    /** ... */
    ref_ptr<btDiscreteDynamicsWorld> dynamicsWorld_;
  };
} // namespace

#endif /* BULLET_ANIMATION_H_ */
