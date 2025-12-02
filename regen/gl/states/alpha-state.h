#ifndef REGEN_ALPHA_STATE_H_
#define REGEN_ALPHA_STATE_H_

#include <regen/gl/states/atomic-states.h>

namespace regen {
	/**
	 * AlphaState is a server-side state that allows to define
	 * discard and clip thresholds for alpha testing in fragment shaders.
	 */
	class AlphaState : public ServerSideState {
	public:
		AlphaState();

		/**
		 * Defines a discard threshold for alpha testing in fragment shaders.
		 * If set then fragments with alpha < discardThreshold_ will be discarded.
		 * @param discardThreshold the discard threshold input.
		 */
		void setDiscardThreshold(float discardThreshold);

		/**
		 * Defines a clip threshold for alpha computation in fragment shaders.
		 * If set then fragments with alpha < clipThreshold_ will be clipped.
		 * If not set then no clipping is done.
		 * @param edge the clip threshold input.
		 */
		void setClipThreshold(float edge);

		/**
		 * Defines a clip threshold for alpha computation in fragment shaders.
		 * Alpha below min will be mapped to 0, and alpha above max will be mapped to 1.
		 * In between values will be linearly interpolated.
		 * @param edgeMin the minimum clip threshold.
		 * @param edgeMax the maximum clip threshold.
		 */
		void setClipThreshold(float edgeMin, float edgeMax);

		/**
		 * If set then early depth test will be forced in fragment shaders.
		 * This can improve performance in some cases, but may lead to artifacts.
		 * @param forceEarlyDepthTest whether to force early depth test.
		 */
		void setForceEarlyDepthTest(bool forceEarlyDepthTest);

		/**
		 * Load an AlphaState from a scene input node.
		 * @param ctx the loading context.
		 * @param input the scene input node to load from.
		 * @return a reference to the loaded AlphaState.
		 */
		static ref_ptr<AlphaState> load(LoadingContext &ctx, scene::SceneInputNode &input);

	protected:
		// if set then fragment with alpha < discardThreshold_ will be discarded.
		ref_ptr<ShaderInput1f> discardThreshold_;
		ref_ptr<ShaderInput1f> clipThreshold_;
		ref_ptr<ShaderInput1f> clipMin_;
		ref_ptr<ShaderInput1f> clipMax_;
		bool forceEarlyDepthTest_ = false;
	};

} // namespace

#endif /* REGEN_ALPHA_STATE_H_ */
