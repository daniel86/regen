#ifndef REGEN_STENCIL_STATE_H_
#define REGEN_STENCIL_STATE_H_

#include "regen/scene/loading-context.h"

namespace regen {
	/**
	 * \brief Set stencil testing parameters.
	 */
	class StencilState : public State {
	public:
		StencilState();

		/**
		 * Load stencil state from scene graph.
		 * @param ctx the loading context.
		 * @param input the scene input node.
		 * @return the stencil state.
		 */
		static ref_ptr<StencilState> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Enable or disable stencil testing with this state.
		 * @param useStencilTest true to enable stencil testing.
		 */
		void set_useStencilTest(bool useStencilTest);

		/**
		 * Set the stencil mask.
		 * @param mask the stencil mask.
		 */
		void set_stencilMask(unsigned int mask);

		/**
		 * Set the stencil test mask.
		 * @param mask the stencil test mask.
		 */
		void set_stencilTestMask(unsigned int mask);

		/**
		 * Set the stencil test function.
		 * @param func the stencil test function.
		 */
		void set_stencilTestFunc(GLenum func);

		/**
		 * Set the stencil test reference value.
		 * @param ref the stencil test reference value.
		 */
		void set_stencilTestRef(int ref);

		/**
		 * Specifies the action to take when the stencil test fails.
		 * Eight symbolic constants are accepted: GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_INCR_WRAP,
		 * GL_DECR, GL_DECR_WRAP, and GL_INVERT. The initial value is GL_KEEP.
		 * @param op the stencil test sfail operation.
		 */
		void set_stencilTestFail(GLenum sfail);

		/**
		 * Specifies the stencil action when the stencil test passes, but the depth test fails.
		 * dpfail accepts the same symbolic constants as sfail. The initial value is GL_KEEP.
		 * @param op the stencil test dpfail operation.
		 */
		void set_depthTestFail(GLenum dpfail);

		/**
		 * Specifies the stencil action when both the stencil test and the depth test pass,
		 * or when the stencil test passes and either there is no depth buffer or depth testing
		 * is not enabled. dppass accepts the same symbolic constants as sfail. The initial value is GL_KEEP.
		 * @param op the stencil test dppass operation.
		 */
		void set_depthTestPass(GLenum dppass);

	protected:
		ref_ptr<State> stencilTestToggle_;
		ref_ptr<State> stencilMaskState_;
		ref_ptr<State> stencilFuncState_;
		ref_ptr<State> stencilOpState_;
	};
} // namespace

#endif /* REGEN_STENCIL_STATE_H_ */
