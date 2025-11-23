#ifndef REGEN_WORLD_MODEL_DEBUG_H
#define REGEN_WORLD_MODEL_DEBUG_H

#include <regen/states/state-node.h>
#include "regen/shader/shader-state.h"
#include "regen/states/state-configurer.h"
#include "regen/utility/debug-interface.h"
#include "world-model.h"
#include "regen/math/bezier.h"

namespace regen {
	class WorldModelDebug : public StateNode, public HasShader, public DebugInterface {
	public:
		explicit WorldModelDebug(const ref_ptr<WorldModel> &world);

		// DebugInterface interface
		void drawLine(const Vec3f &from, const Vec3f &to, const Vec3f &color) override;

		// DebugInterface interface
		void drawCircle(const Vec3f &center, float radius, const Vec3f &color) override;

		void drawCrossXZ(const Vec3f &center, float radius, const Vec3f &color);

		void drawArrow(const Vec3f &origin, const Vec3f &direction, float length, const Vec3f &color);

		void drawCurve(
			const math::Bezier<Vec2f> &curve,
			float height, int segments, const Vec3f &color);

		// StateNode interface
		void traverse(regen::RenderState *rs) override;

	private:
		ref_ptr<WorldModel> world_;
		ref_ptr<ShaderInput3f> lineColor_;
		ref_ptr<ShaderInput3f> lineVertices_;
		int lineLocation_;

		ref_ptr<VAO> vao_;
		uint32_t vbo_{};
		uint32_t bufferSize_;
	};
}

#endif //REGEN_WORLD_MODEL_DEBUG_H
