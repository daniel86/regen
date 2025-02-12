#ifndef REGEN_SPATIAL_INDEX_DEBUG_H
#define REGEN_SPATIAL_INDEX_DEBUG_H

#include <regen/states/state-node.h>
#include <regen/states/shader-state.h>
#include "regen/states/state-configurer.h"
#include "spatial-index.h"
#include "regen/utility/debug-interface.h"
#include <btBulletDynamicsCommon.h>

namespace regen {
	/**
	 * Debug drawer for Bullet physics engine.
	 * This class is a StateNode that can be added to the scene graph.
	 * It will draw the physics world using Bullet's debug drawing.
	 * The physics world must be set using setDynamicsWorld().
	 */
	class SpatialIndexDebug : public StateNode, public HasShader, public HasInput, public DebugInterface {
	public:
		explicit SpatialIndexDebug(const ref_ptr<SpatialIndex> &index);

		// DebugInterface interface
		void drawLine(const Vec3f &from, const Vec3f &to, const Vec3f &color) override;

		// DebugInterface interface
		void drawCircle(const Vec3f &center, float radius, const Vec3f &color) override;

		void drawBox(const BoundingBox &box);

		void drawSphere(const BoundingSphere &sphere);

		void drawFrustum(const Frustum &frustum, const Vec3f &color = Vec3f(1.0f, 0.0f, 1.0f));

		// StateNode interface
		void traverse(regen::RenderState *rs) override;

	private:
		ref_ptr<SpatialIndex> index_;
		ref_ptr<ShaderInput3f> lineColor_;
		ref_ptr<ShaderInput3f> lineVertices_;
		GLint lineLocation_;

		void debugFrustum(const Frustum &frustum, const Vec3f &color = Vec3f(0.0f, 1.0f, 0.0f));

		ref_ptr<VAO> vao_;
		GLuint vbo_{};
		GLuint bufferSize_;
	};
}

#endif //REGEN_SPATIAL_INDEX_DEBUG_H
