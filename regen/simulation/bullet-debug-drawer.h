#ifndef REGEN_BULLET_DEBUG_DRAWER_H
#define REGEN_BULLET_DEBUG_DRAWER_H

#include <regen/scene/state-node.h>
#include "regen/shader/shader-state.h"
#include "regen/scene/state-configurer.h"
#include "bullet-physics.h"
#include <btBulletDynamicsCommon.h>

namespace regen {
	/**
	 * Debug drawer for Bullet physics engine.
	 * This class is a StateNode that can be added to the scene graph.
	 * It will draw the physics world using Bullet's debug drawing.
	 * The physics world must be set using setDynamicsWorld().
	 */
	class BulletDebugDrawer : public btIDebugDraw, public StateNode, public HasShader {
	public:
		explicit BulletDebugDrawer(const ref_ptr<BulletPhysics> &physics);

		// btIDebugDraw interface
		void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;

		void
		drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime,
						 const btVector3 &color) override;

		void reportErrorWarning(const char *warningString) override;

		void draw3dText(const btVector3 &location, const char *textString) override;

		void setDebugMode(int debugMode) override;

		int getDebugMode() const override;

		// StateNode interface
		void traverse(regen::RenderState *rs) override;

	private:
		ref_ptr<BulletPhysics> physics_;
		ref_ptr<ShaderInput3f> lineColor_;
		ref_ptr<ShaderInput3f> lineVertices_;

		ref_ptr<VAO> vao_;
		uint32_t vbo_{};
		uint32_t bufferSize_;
		int m_debugMode;

		regen::RenderState *renderState_{};
	};
}

#endif //REGEN_BULLET_DEBUG_DRAWER_H
