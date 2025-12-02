#ifndef REGEN_DEPTH_STATE_H_
#define REGEN_DEPTH_STATE_H_

#include "atomic-states.h"

namespace regen {
	/**
	 * \brief Allows manipulating how the depth buffer is handled.
	 */
	class DepthState : public ServerSideState {
	public:
		static ref_ptr<DepthState> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Enable or disable depth testing with this state.
		 */
		void set_useDepthTest(bool useDepthTest);

		/**
		 * Enable or disable depth writing with this state.
		 */
		void set_useDepthWrite(bool useDepthTest);

		/**
		 * Specifies the depth comparison function. Symbolic constants
		 * GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER,
		 * GL_NOTEQUAL, GL_GEQUAL, and GL_ALWAYS are accepted.
		 * The initial value is GL_LESS.
		 */
		void set_depthFunc(GLenum depthFunc = GL_LESS);

		/**
		 * specify mapping of depth values from normalized device coordinates to window coordinates.
		 * nearVal specifies the mapping of the near clipping plane to window coordinates. The initial value is 0.
		 * farVal specifies the mapping of the far clipping plane to window coordinates. The initial value is 1.
		 */
		void set_depthRange(double nearVal = 0.0, double farVal = 1.0);

	protected:
		ref_ptr<State> depthTestToggle_;
		ref_ptr<State> depthWriteToggle_;
		ref_ptr<State> depthRange_;
		ref_ptr<State> depthFunc_;
	};
} // namespace

#endif /* REGEN_DEPTH_STATE_H_ */
