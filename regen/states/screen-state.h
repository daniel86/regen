#ifndef REGEN_SCREEN_STATE_H_
#define REGEN_SCREEN_STATE_H_

#include <regen/states/state.h>

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
				const ref_ptr<ShaderInput2i> &windowViewport,
				const GLenum drawBuffer = GL_FRONT);

		// override
		void enable(RenderState *) override;

		void disable(RenderState *) override;

	protected:
		ref_ptr<ShaderInput2i> windowViewport_;
		ref_ptr<ShaderInput2f> viewport_;
		ref_ptr<ShaderInput2f> inverseViewport_;
		GLenum drawBuffer_;
		Vec4ui glViewport_;
		unsigned int lastViewportStamp_ = 0;
	};
} // namespace

#endif /* REGEN_FBO_STATE_H_ */
