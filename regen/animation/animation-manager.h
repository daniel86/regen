#ifndef REGEN_ANIMATION_MANAGER_H_
#define REGEN_ANIMATION_MANAGER_H_

#include <barrier>
#include <regen/animation/animation.h>
#include "regen/shapes/spatial-index.h"

namespace regen {
	/**
	 * \brief Manages multiple glAnimations in a separate thread.
	 */
	class AnimationManager {
	public:
		/**
		 * @return animation manager reference.
		 */
		static AnimationManager &get();

		/**
		 * Command types for adding/removing animations.
		 */
		enum CommandType : uint8_t {
			ADD,
			REMOVE
		};

		/**
		 * Animation command structure.
		 */
		struct Command {
			CommandType type;
			Animation *animation;
		};

		AnimationManager();

		~AnimationManager();

		/**
		 * Set the root state.
		 * @param rootState the root state.
		 */
		void setRootState(const ref_ptr<State> &rootState);

		/**
		 * @return the root state.
		 */
		const ref_ptr<State> &rootState() const { return rootState_; }

		/**
		 * Set the spatial indices.
		 * @param indices the spatial indices.
		 */
		void setSpatialIndices(const std::vector<ref_ptr<SpatialIndex>> &indices) { spatialIndices_ = indices; }

		/**
		 * @return the set of animations.
		 */
		const std::vector<Animation*>& cpuAnimations() { return cpuAnimations_; }

		/**
		 * @return the set of glAnimations.
		 */
		const std::vector<Animation*>& gpuAnimations() { return gpuAnimations_; }

		/**
		 * @return the set of unsynchronized animations.
		 */
		const std::vector<Animation*>& unsyncedAnimations() { return unsyncedAnimations_; }

		/**
		 * Shutdown the animation manager.
		 * @param blocking if true, wait for completion of any active animations.
		 */
		void shutdown(bool blocking = false);

		/**
		 * Pauses all animations.
		 * @param blocking if true, wait for completion of any active animations.
		 */
		void pause(bool blocking = false);

		/**
		 * Remove all animations from the manager.
		 */
		void clear();

		/**
		 * Resumes previously paused animations.
		 * @param runOnce if true, run one update step immediately upon resuming.
		 */
		void resume(bool runOnce = true);

		/**
		 * Reset the time of the animation manager.
		 */
		void resetTime();

		/**
		 * Adds an animation to the manager.
		 * @param animation a Animation instance.
		 */
		void addAnimation(Animation *animation);

		/**
		 * Removes previously added animation.
		 * @param animation a Animation instance.
		 */
		void removeAnimation(Animation *animation);

		/**
		 * Performs a single GPU update step.
		 * This should be called from the GPU thread each frame.
		 * @param dt_ms time delta in milliseconds since last update.
		 */
		void gpuUpdateStep(double dt_ms);

	private:
		// Time tracking
		boost::posix_time::ptime time_;
		boost::posix_time::ptime lastTime_;

		// GPU and CPU update threads
		boost::thread cpuUpdateThread_;
		boost::thread::id gpu_threadID_;
		boost::thread::id cpu_threadID_;

		// The root state for all animations
		ref_ptr<State> rootState_;
		// Spatial indices for visibility updates
		std::vector<ref_ptr<SpatialIndex>> spatialIndices_;

		// Synchronized GPU/CPU threads
		std::vector<Animation *> cpuAnimations_; // only touch in CPU thread
		std::vector<Animation *> gpuAnimations_; // only touch in GPU thread
		// Unsynchronized threads that run independently
		std::vector<Animation *> unsyncedAnimations_;
		std::vector<boost::thread> unsyncedThreads_;

		// Barrier to synchronize CPU and GPU threads at the end of each frame
		using BarrierCompletionFun = std::function<void()>;
		std::barrier<BarrierCompletionFun> frameBarrier_;
		// This lock is only used for adding/removing unsynced animations,
		// since there is no hot loop over the unsyncedAnimations_ list, this is acceptable.
		boost::mutex unsyncedListLock_;

		// The close flag is only toggled to true when system shuts down
		CachePadded<std::atomic_flag> closeFlag_ = { {false} };
		// The pause flag is toggled to true when animations are paused, eg.
		// when a new scene is loaded
		CachePadded<std::atomic_flag> pauseFlag_ = { {false} };
		// Flags to indicate if an update is active, one for each thread.
		// Unsynced animations currently have their dedicated threads, so each has its own flag.
		CachePadded<std::atomic_flag> cpu_isUpdateActive_ = { {false} };
		CachePadded<std::atomic_flag> gpu_isUpdateActive_ = { {false} };
		std::atomic<int> unsynced_numActiveUpdates_{0};

		// Command queues for adding/removing animations
		ThreadSafeQueue<Command> gpu_commandQueue_;
		ThreadSafeQueue<Command> cpu_commandQueue_;

		void flushGraphics() { frameBarrier_.arrive_and_wait(); }

		void cpuUpdate();

		void cpuUpdateStep();

		void unsyncedUpdate(Animation *animation);

		void waitForAnimations() const;

		void swapClientData();

		friend class Scene;
	};
} // namespace

#endif /* REGEN_ANIMATION_MANAGER_H_ */
