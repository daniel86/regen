#ifndef REGEN_STAGING_SYSTEM_H_
#define REGEN_STAGING_SYSTEM_H_

#include "buffer-block.h"

namespace regen {
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
			ARENA_WRITE_PER_FRAME_PM_SMALL_RNG = 0,
			// An arena for per-frame updates using persistent mapping of a large adaptive ring buffer.
			// Large meaning that the maximum number of ring segments is capped to some rather high value.
			// Note that for medium and small, we always copy the whole data (no PAR mode).
			ARENA_WRITE_PER_FRAME_PM_LARGE_RNG,
			// An arena for per-frame updates where ring buffer is not feasible, e.g. due to excessive size.
			// In this arena mode, only a single buffer is used in staging, and data is copied directly
			// into this staging buffer (without any mapping).
			ARENA_WRITE_PER_FRAME_CP_SB,
			// An arena for reading small data per frame. The arena uses persistent mapping
			// with an adaptive ring buffer.
			ARENA_READ_PER_FRAME_PM_RNG,
			// An arena for rare updates which are performed via a single buffer in staging which is
			// temporary mapped with range invalidation.
			ARENA_WRITE_RARE_TM_SB,
			// An arena for rare reading of small data. It uses temporary mapping with a single buffer
			// in staging.
			ARENA_READ_RARE_TM_SB,
			// An arena for static data which is only updated very rarely.
			// This is using implicit staging without multi-buffering.
			ARENA_WRITE_NEVER_CP_NB,
			ARENA_TYPE_LAST // keep last
		};
		using BlockPtr = BufferBlock *;

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
		void updateData();

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
		// a staging arena
		struct Arena {
			Arena() = default;

			ArenaType type = ARENA_TYPE_LAST;
			BufferFlags flags = BufferFlags(COPY_WRITE_BUFFER);
			// accumulated size of all buffer objects in this arena
			uint32_t requiredSize = 0;
			// the current number of segments in the ring buffer
			uint32_t numRingSegments = 2;
			// indicates if the arena has new CPU data to flush
			bool isDirty = false;
			std::vector<BlockPtr> bufferObjects;
			ref_ptr<StagingBuffer> stagingBuffer;

			void sort();

			void resize();

			bool updateRequiredSize();
		};

		std::array<Arena *, ARENA_TYPE_LAST> arenas_;

		Arena *addBufferBlock_readOnly(
				const BlockPtr &block,
				const BufferFlags &flags,
				BufferSizeClass sizeClass);

		Arena *addBufferBlock_writeOnly(
				const BlockPtr &block,
				const BufferFlags &flags,
				BufferSizeClass sizeClass);

		static Arena *createArena(ArenaType arenaType, BufferAccessMode accessMode);

		Arena *addToArena(const BlockPtr &block, ArenaType arenaType);
	};

	// support streaming operators for ArenaType
	std::ostream &operator<<(std::ostream &out, const StagingSystem::ArenaType &v);
} // namespace

#endif /* REGEN_STAGING_SYSTEM_H_ */
