#ifndef REGEN_ANIMATION_H_
#define REGEN_ANIMATION_H_

#include <regen/utility/event-object.h>
#include <regen/gl-types/render-state.h>
#include <regen/states/state.h>

namespace regen {
	/**
	 * \brief Abstract base class for animations.
	 */
	class Animation : public EventObject {
	public:
		/**
		 * The animation state switched to inactive.
		 */
		static uint32_t ANIMATION_STOPPED;

		/**
		 * Create an animation.
		 * Note that the animation removes itself from the AnimationManager in the destructor.
		 * @param isGPUAnimation execute with render context.
		 * @param isCPUAnimation execute without render context in separate thread.
		 */
		Animation(bool isGPUAnimation, bool isCPUAnimation);

		~Animation() override;

		Animation(const Animation &) = delete;

		/**
		 * @return true if the animation implements glAnimate().
		 */
		bool isGPUAnimation() const { return isGPUAnimation_; }

		/**
		 * @return true if the animation implements animate().
		 */
		bool isCPUAnimation() const { return isCPUAnimation_; }

		/**
		 * @return true if this animation is synchronized.
		 */
		bool isSynchronized() const { return isSynchronized_; }

		/**
		 * Set the synchronized flag.
		 * @param v the synchronized flag.
		 */
		void setSynchronized(bool v) { isSynchronized_ = v; }

		/**
		 * @return the desired frame rate.
		 */
		float desiredFrameRate() const { return desiredFrameRate_; }

		/**
		 * @return true if this animation is active.
		 */
		bool isRunning() const { return isRunning_.test(std::memory_order_acquire); }

		/**
		 * @return the name of the animation.
		 */
		const std::string &animationName() const { return animationName_; }

		/**
		 * Set the name of the animation.
		 * @param name the name.
		 */
		void setAnimationName(std::string_view name) { animationName_ = name; }

		/**
		 * @return true if this animation has a name.
		 */
		bool hasAnimationName() const { return !animationName_.empty(); }

		/**
		 * @return the root state.
		 */
		const ref_ptr<State>& animationState() const { return animationState_; }

		/**
		 * Set the root state.
		 * @param state the root state.
		 */
		void joinAnimationState(const ref_ptr<State> &state);

		/**
		 * Remove the root state.
		 * @param state the root state.
		 */
		void disjoinAnimationState(const ref_ptr<State> &state);

		/**
		 * Activate this animation.
		 */
		virtual void startAnimation();

		/**
		 * Deactivate this animation.
		 */
		virtual void stopAnimation() { doStopAnimation(); }

		/**
		 * Make the next animation step.
		 * This should be called each frame.
		 * @param dt time difference to last call in milliseconds.
		 */
		virtual void cpuUpdate(double dt) {}

		/**
		 * Upload animation data to GL.
		 * This should be called each frame in a thread
		 * with a GL context.
		 * @param rs the render state.
		 * @param dt time difference to last call in milliseconds.
		 */
		virtual void gpuUpdate(RenderState *rs, double dt) {}

	protected:
		const bool isGPUAnimation_;
		const bool isCPUAnimation_;
		std::string animationName_;

		bool isSynchronized_ = true;
		float desiredFrameRate_ = 60.0f;

		std::atomic_flag isRunning_ = ATOMIC_FLAG_INIT;
		ref_ptr<State> animationState_;

		void doStopAnimation();

		friend class AnimationManager;
	};
} // namespace

#endif /* REGEN_ANIMATION_H_ */
