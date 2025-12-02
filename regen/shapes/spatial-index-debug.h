#ifndef REGEN_SPATIAL_INDEX_DEBUG_H
#define REGEN_SPATIAL_INDEX_DEBUG_H

#include <regen/scene/state-node.h>

#include "aabb.h"
#include "obb.h"
#include "regen/shader/shader-state.h"
#include "regen/scene/state-configurer.h"
#include "spatial-index.h"
#include "regen/utility/debug-interface.h"

namespace regen {
	class SpatialIndexDebug : public StateNode, public HasShader, public DebugInterface {
	public:
		explicit SpatialIndexDebug(const ref_ptr<SpatialIndex> &index);

		// DebugInterface interface
		void drawLine(const Vec3f &from, const Vec3f &to, const Vec3f &color) override;

		// DebugInterface interface
		void drawCircle(const Vec3f &center, float radius, const Vec3f &color) override;

		void drawBox(const AABB &box);

		void drawBox(const OBB &box);

		void drawBox(const Vec3f *boxVertices);

		void drawSphere(const BoundingSphere &sphere);

		void drawFrustum(const Frustum &frustum, const Vec3f &color = Vec3f(1.0f, 0.0f, 1.0f));

		// StateNode interface
		void traverse(regen::RenderState *rs) override;

	private:
		ref_ptr<SpatialIndex> index_;
		ref_ptr<ShaderInput3f> lineColor_;
		ref_ptr<ShaderInput3f> lineVertices_;
		int lineLocation_;

		void debugFrustum(const Frustum &frustum, const Vec3f &color = Vec3f(0.0f, 1.0f, 0.0f));

		ref_ptr<VAO> vao_;
		uint32_t vbo_{};
		uint32_t bufferSize_;

		friend class SpatialIndex;
	};
}

#endif //REGEN_SPATIAL_INDEX_DEBUG_H
