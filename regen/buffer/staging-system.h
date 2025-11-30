#ifndef REGEN_STAGING_SYSTEM_H_
#define REGEN_STAGING_SYSTEM_H_

#include "staged-buffer.h"
#include "regen/gl-types/queries/elapsed-time.h"
#include "regen/utility/free-list.h"

// Note: We need to swap client buffers after each copy from
//   client buffer into staging. We can either do it directly in the staging
//   system right after the copy into staging, or we can let the animation thread
//   perform the swapping, but it has to wait for the copy into staging to complete.
//#define REGEN_STAGING_ANIMATION_THREAD_SWAPS_CLIENT

namespace regen {
	static constexpr bool ANIMATION_THREAD_SWAPS_CLIENT_BUFFERS = false;

	/**
	 * \brief A staging system that manages staging buffers for buffer objects (BOs).
	 *
	 * The staging system is responsible for managing the staging buffers and arenas
	 * for BOs, allowing efficient updates and access to buffer data.
	 *
	 * Note: It is currently implemented as a singleton, meaning that there is only one instance
	 * of the staging system in the application.
	 */
	class StagingSystem {
	public:
		enum ArenaType {
			// Note: keep per-frame modes first!
			// An arena for per-frame updates using persistent mapping of a small adaptive ring buffer.
			// Small meaning that the maximum number of ring segments is capped to some rather small value.
			WRITE_PER_FRAME_LARGE_DATA = 0,
			// An arena for per-frame updates using persistent mapping of a large adaptive ring buffer.
			// Large meaning that the maximum number of ring segments is capped to some rather high value.
			WRITE_PER_FRAME_SMALL_DATA,
			// An arena for per-frame updates where ring buffer is not feasible, e.g. due to excessive size.
			// In this arena mode, only a single buffer is used in staging, and data is copied directly
			// into this staging buffer (without any mapping).
			WRITE_PER_FRAME_HUGE_DATA,
			// An arena for reading data per frame. The arena uses persistent mapping
			// with an adaptive ring buffer.
			READ_PER_FRAME,
			// An arena for rare reading of data. It uses temporary mapping with a single buffer
			// in staging and polls the main buffer at regular intervals.
			READ_RARELY,
			// An arena for rare updates which are performed via a single buffer in staging which is
			// temporary mapped with range invalidation.
			WRITE_RARELY,
			// An arena for static data which is only updated very rarely.
			// This is using implicit staging without multi-buffering, the most appropriate
			// mode of uploading data to the GPU is determined by storage flags of the main buffer.
			WRITE_ALMOST_NEVER,
			ARENA_TYPE_LAST // keep last
		};
		using BlockPtr = StagedBuffer *;

		// each staging buffer segment is aligned to this size.
		// default is page size.
		static uint32_t STAGING_BUFFER_ALIGNMENT;
		// extra space for staging buffer.
		static float STAGING_BUFFER_SLACK;
		// the alignment for ranges within the staging buffer.
		// it won't be possible to reserve any memory range which is not a multiple of this value.
		static uint32_t STAGING_RANGE_ALIGNMENT;
		// static constants for cooldown times, in milliseconds
		static float COOLDOWN_RARE_READ;
		static float MIN_COOLDOWN_RARE_WRITE;
		static float MAX_COOLDOWN_RARE_READ;
		static float MIN_COOLDOWN_NEVER_WRITE;
		static float MAX_COOLDOWN_NEVER_WRITE;

		~StagingSystem();

		StagingSystem(const StagingSystem &) = delete;

		StagingSystem &operator=(const StagingSystem &) = delete;

		/**
		 * \brief Add a buffer block to the staging system.
		 *
		 * This method adds a buffer block to the appropriate staging arena based on its flags
		 * and size class. It returns a reference to the staging buffer that was created or used.
		 *
		 * @param block the buffer block to add.
		 * @return a reference to the staging buffer for the block.
		 */
		ref_ptr<StagingBuffer> addBufferBlock(const BlockPtr &block);

		/**
		 * \brief Remove a buffer block from the staging system.
		 *
		 * This method removes a buffer block from the staging system and releases its resources.
		 * It is called when the buffer block is no longer needed.
		 *
		 * @param block the buffer block to remove.
		 */
		void removeBufferBlock(const BlockPtr &block);

		/**
		 * \brief Update the staging buffers for all arenas.
		 *
		 * This method is called once after all buffer objects have been added to the system.
		 * It ensures that all draw buffers are allocated and that the staging buffers are
		 * properly set up for each arena.
		 */
		void updateBuffers();

		/**
		 * \brief Update the data in the staging system.
		 *
		 * This method is called each frame to update the data in the staging system.
		 * It processes all arenas and flushes the data to the GPU as needed.
		 */
		void updateData(float dt_ms = 0.0f);

		/**
		 * \brief Check if a copy operation is in progress.
		 *
		 * This method checks if a copy operation is currently in progress in the staging system.
		 * It is used to avoid concurrent modifications of the staging buffers.
		 *
		 * @return true if a copy operation is in progress, false otherwise.
		 */
		bool isCopyInProgress() const;

		/**
		 * \brief Set the copy in progress flag.
		 *
		 * This method sets the copy in progress flag to true, indicating that a copy operation
		 * is currently being performed. It is used to prevent concurrent modifications of the
		 * staging buffers during the copy operation.
		 */
		void setIsCopyInProgress();

		/**
		 * This method schedules a copy operation to be performed at the end of this frame.
		 * @param copyRange the range of data to copy.
		 */
		void scheduledCopy(const BufferCopyRange &copyRange);

		/**
		 * This method is called to swap the client data buffers, making the last writes available
		 * for the next frame.
		 */
		void swapClientData();

		/**
		 * \brief Clear the staging system.
		 *
		 * This method clears all arenas and releases the resources used by the staging system.
		 */
		void clear();

		/**
		 * \brief Get the singleton instance of the staging system.
		 *
		 * This method returns the singleton instance of the staging system.
		 * It is a thread-safe implementation.
		 *
		 * @return a reference to the staging system instance.
		 */
		static StagingSystem &instance() {
			static StagingSystem instance;
			return instance;
		}

	private:
		StagingSystem();

	protected:
		// a BO under control of the staging system
		struct ManagedBO;
		// a staging arena
		struct Arena;

		std::array<Arena *, ARENA_TYPE_LAST> arenas_;
		std::atomic<bool> copyInProgress_ = false;
		std::vector<BufferCopyRange> scheduledCopies_;
		uint32_t numScheduledCopies_ = 0;

		Arena *addBufferBlock_readOnly(
				const BlockPtr &block,
				const BufferFlags &flags,
				BufferSizeClass sizeClass);

		Arena *addBufferBlock_writeOnly(
				const BlockPtr &block,
				const BufferFlags &flags,
				BufferSizeClass sizeClass);

		Arena *addToArena(const BlockPtr &block, ArenaType arenaType, bool isMoved=false);

		bool moveAdaptive(Arena *arena, ManagedBO &managed, float boUpdateRate);

		void moveToArena(ManagedBO &managed, ArenaType targetArenaType);

		bool updateArenaSize(Arena *arena);

		struct StagingStatistics {
			uint32_t numDirtyArenas = 0;
			uint32_t numDirtyBOs = 0;
			uint32_t numDirtySegments = 0;
			uint32_t numTotalBOs = 0;
			uint32_t numSwapCopies = 0;
		} stats_;

		static ElapsedTimeDebugger elapsedTime() {
			static ElapsedTimeDebugger x("Staging System", 300);
			return x;
		}
	};

	// support streaming operators for ArenaType
	std::ostream &operator<<(std::ostream &out, const StagingSystem::ArenaType &v);
} // namespace

#endif /* REGEN_STAGING_SYSTEM_H_ */
