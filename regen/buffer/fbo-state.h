#ifndef REGEN_FBO_STATE_H_
#define REGEN_FBO_STATE_H_

#include "regen/states/state.h"
#include "regen/states/atomic-states.h"
#include "fbo.h"

namespace regen {
	/**
	 * \brief Framebuffer Objects are a mechanism for rendering to images
	 * other than the default OpenGL Default Framebuffer.
	 *
	 * They are OpenGL Objects that allow you to render directly
	 * to textures, as well as blitting from one framebuffer to another.
	 */
	class FBOState : public State {
	public:
		/**
		 * @param fbo FBO instance.
		 */
		explicit FBOState(const ref_ptr<FBO> &fbo);

		static ref_ptr<State> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @return the FBO instance.
		 */
		const ref_ptr<FBO> &fbo();

		/**
		 * Resize attached textures.
		 */
		void resize(GLuint width, GLuint height);

		/**
		 * Clear depth buffer to preset values.
		 */
		void setClearDepth();

		/**
		 * Clear stencil buffer to preset values.
		 */
		void setClearStencil();

		/**
		 * Clear color buffer to preset values.
		 */
		void setClearColor(const ClearColorState::Data &data);

		/**
		 * Clear color buffers to preset values.
		 */
		void setClearColor(const std::list<ClearColorState::Data> &data);

		/**
		 * Add a draw buffer to the list of color buffers to be drawn into.
		 */
		void addDrawBuffer(GLenum colorAttachment);

		/**
		 * Specify list of color buffers to be drawn into.
		 */
		void setDrawBuffers(const std::vector<GLenum> &attachments);

		/**
		 * Specify list of color buffers to be drawn into.
		 * Each frame only a single buffer is accessed by index
		 * and afterwards the index is incremented by 1.
		 * @param attachments list of color buffers.
		 */
		void setPingPongBuffers(const std::vector<GLenum> &attachments);

		void setParentBufferState(const ref_ptr<FBOState> &parentFBO);

		// override
		void enable(RenderState *) override;

		void disable(RenderState *) override;

	protected:
		ref_ptr<FBO> fbo_;
		// used for optimization of state management
		ref_ptr<FBOState> parentFBO_;

		ref_ptr<ClearState> clearCallable_;
		ref_ptr<ClearColorState> clearColorCallable_;
		ref_ptr<State> drawBufferCallable_;

		void createClearState();
	};
} // namespace

#endif /* REGEN_FBO_STATE_H_ */
