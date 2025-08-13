#ifndef REGEN_SCREEN_STATE_H_
#define REGEN_SCREEN_STATE_H_

#include <regen/states/state.h>
#include "regen/textures/fbo-state.h"

namespace regen {
	/**
	 * \brief Activation of OpenGL Default Framebuffer.
	 */
	class ScreenState : public State {
	public:
		/**
		 * @param windowViewport The window size (width/height).
		 * @param drawBuffer GL_FRONT, GL_BACK or GL_FRONT_AND_BACK.
		 */
		explicit ScreenState(
				const ref_ptr<Screen> &screen,
				const GLenum drawBuffer = GL_FRONT);

		void setParentBufferState(const ref_ptr<FBOState> &fbo) { parentFBO_ = fbo; }

		// override
		void enable(RenderState *) override;

		void disable(RenderState *) override;

	protected:
		ref_ptr<Screen> screen_;
		ref_ptr<ShaderInput2f> viewport_;
		ref_ptr<ShaderInput2f> inverseViewport_;
		ref_ptr<FBOState> parentFBO_;
		GLenum drawBuffer_;
		Vec4ui glViewport_;
		unsigned int lastViewportStamp_ = 0;
	};
} // namespace

#endif /* REGEN_FBO_STATE_H_ */
