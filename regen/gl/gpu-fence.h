#ifndef REGEN_GPU_FENCE_H_
#define REGEN_GPU_FENCE_H_

#include <GL/glew.h>

namespace regen {
	/**
	 * \brief A GPU fence for synchronizing GPU operations.
	 *
	 * This class wraps an OpenGL sync object (fence) that can be used to
	 * synchronize GPU operations, ensuring that certain operations are completed
	 * before proceeding with others.
	 */
	class GPUFence {
	public:
		// Default timeout for waiting on the fence
		static uint64_t WAIT_TIMEOUT;
		// Default range for stall detection
		static uint32_t STALL_RANGE;

		GPUFence();

		GPUFence(const GPUFence &);

		~GPUFence();

		GPUFence &operator=(const GPUFence &);

		/**
		 * Get the stall rate, which is the percentage of frames that had a stall.
		 * @return the stall rate as a float, where 0.0 means no stalls and 1.0 means all frames had stalls.
		 */
		float getStallRate() const;

		/**
		 * Reset the stall history, clearing the array of stalled frames.
		 */
		void resetStallHistory();

		/**
		 * Set a new fence point, deleting the old one if it exists.
		 * This will create a new OpenGL sync object that can be waited on.
		 */
		void setFencePoint();

		/**
		 * Ensure that a fence point is set, creating one if it does not exist.
		 * This is useful to ensure that the fence is ready for waiting.
		 */
		void ensureFencePoint();

		/**
		 * Wait for the fence to be signaled.
		 * @param allowFrameDropping if true, will not block on the fence, but drop frames instead.
		 * @return true if the fence was signaled, false if it was not signaled and frames were dropped.
		 */
		bool wait(bool allowFrameDropping = false);

		/**
		 * Check if the fence is signaled, non-blocking.
		 * @return true if the fence is signaled, false otherwise.
		 */
		bool isSignaled();

	private:
		GLsync fence_ = nullptr; // OpenGL sync object
		// Array for stall detection, true indicates we had a stall in a frame.
		// We record last n frames for computing the stall rate.
		bool *stalledFrames_;
		// Range for stall detection
		const uint32_t stallRange_;
		const float f_stallRange_;
		// Count of frames that had a stall
		uint32_t stallCount_ = 0;
		// Current index in the stall detection array
		uint32_t stallIdx_ = 0;

		void setStalledFrame(bool isStalled);
	};
} // namespace

#endif /* REGEN_GPU_FENCE_H_ */
