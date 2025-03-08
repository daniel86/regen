/*
 * blit-to-screen.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef BLIT_TO_SCREEN_H_
#define BLIT_TO_SCREEN_H_

#include <regen/states/state.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	class BlitState : public State {
	public:
		BlitState() = default;

		static ref_ptr<BlitState> load(LoadingContext &ctx, scene::SceneInputNode &input);
	};

	/**
	 * \brief Blits a FBO color attachment to another FBO.
	 */
	class BlitToFBO : public BlitState {
	public:
		/**
		 * @param src Source FBO.
		 * @param dst Destination FBO.
		 * @param srcAttachment Source color attachment.
		 * @param dstAttachment Destination color attachment.
		 * @param keepRatio Preserve aspect ratio of input texture.
		 */
		BlitToFBO(
				const ref_ptr<FBO> &src,
				const ref_ptr<FBO> &dst,
				GLenum srcAttachment = GL_COLOR_ATTACHMENT0,
				GLenum dstAttachment = GL_COLOR_ATTACHMENT0,
				GLboolean keepRatio = GL_FALSE);

		/**
		 * filterMode must be GL_NEAREST or GL_LINEAR.
		 */
		void set_filterMode(GLenum filterMode = GL_LINEAR);

		/**
		 * The bitwise OR of the flags indicating which buffers are to be copied.
		 * The allowed flags are  GL_COLOR_BUFFER_BIT,
		 * GL_DEPTH_BUFFER_BIT and GL_STENCIL_BUFFER_BIT.
		 */
		void set_sourceBuffer(GLenum sourceBuffer = GL_COLOR_BUFFER_BIT);

		// override
		void enable(RenderState *state) override;

	protected:
		ref_ptr<FBO> src_;
		ref_ptr<FBO> dst_;
		GLenum srcAttachment_;
		GLenum dstAttachment_;
		GLenum filterMode_;
		GLenum sourceBuffer_;
		GLboolean keepRatio_;
	};

	/**
	 * \brief Blits a FBO color attachment to screen.
	 */
	class BlitToScreen : public BlitState {
	public:
		/**
		 * @param fbo FBO to blit.
		 * @param viewport the screen viewport.
		 * @param attachment color attachment to blit.
		 * @param keepRatio Preserve aspect ratio of input texture.
		 */
		BlitToScreen(
				const ref_ptr<FBO> &fbo,
				const ref_ptr<ShaderInput2i> &viewport,
				GLenum attachment = GL_COLOR_ATTACHMENT0,
				GLboolean keepRatio = GL_FALSE);

		/**
		 * filterMode must be GL_NEAREST or GL_LINEAR.
		 */
		void set_filterMode(GLenum filterMode = GL_LINEAR);

		/**
		 * The bitwise OR of the flags indicating which buffers are to be copied.
		 * The allowed flags are  GL_COLOR_BUFFER_BIT,
		 * GL_DEPTH_BUFFER_BIT and GL_STENCIL_BUFFER_BIT.
		 */
		void set_sourceBuffer(GLenum sourceBuffer = GL_COLOR_BUFFER_BIT);

		/**
		 * @return the FBO to blit.
		 */
		auto &fbo() { return fbo_; }

		/**
		 * @return the viewport.
		 */
		auto &viewport() { return viewport_; }

		/**
		 * @return the attachment of the FBO.
		 */
		auto &attachment() { return attachment_; }

		// override
		void enable(RenderState *state) override;

	protected:
		ref_ptr<FBO> fbo_;
		ref_ptr<ShaderInput2i> viewport_;
		GLenum attachment_;
		GLenum filterMode_;
		GLenum sourceBuffer_;
		GLboolean keepRatio_;
	};

	/**
	 * \brief Blits a FBO color attachment to screen.
	 *
	 * This is useful for ping-pong textures consisting of 2 images.
	 */
	class BlitTexToScreen : public BlitToScreen {
	public:
		/**
		 * @param fbo a FBO.
		 * @param texture a texture.
		 * @param viewport the screen viewport.
		 * @param attachment the first texture attachment.
		 */
		BlitTexToScreen(
				const ref_ptr<FBO> &fbo,
				const ref_ptr<Texture> &texture,
				const ref_ptr<ShaderInput2i> &viewport,
				GLenum attachment = GL_COLOR_ATTACHMENT0);

		// override
		void enable(RenderState *state) override;

	protected:
		ref_ptr<Texture> texture_;
		GLenum baseAttachment_;
	};
} // namespace

#endif /* BLIT_TO_SCREEN_H_ */
