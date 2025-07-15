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

		GPUFence() = default;

		~GPUFence();

		// Deleted copy constructor and assignment operator
		GPUFence(const GPUFence &) = delete;

		GPUFence &operator=(const GPUFence &) = delete;

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
	};
} // namespace

#endif /* REGEN_GPU_FENCE_H_ */
