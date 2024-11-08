
#ifndef REGEN_MOTION_BLUR_H_
#define REGEN_MOTION_BLUR_H_

#include <regen/states/atomic-states.h>
#include <regen/states/fullscreen-pass.h>

namespace regen {
	class MotionBlur : public FullscreenPass {
	public:
		MotionBlur(const ref_ptr<Camera> &camera);

		void enable(RenderState *rs) override;

		void disable(RenderState *rs) override;

	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<ShaderInputMat4> lastViewProjectionMat_;
	};
} // namespace

#endif /* REGEN_MOTION_BLUR_H_ */
