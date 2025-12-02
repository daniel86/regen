#include "staging-system.h"
#include "regen/gl/queries/elapsed-time.h"
#include <regen/gl/gl-param.h>

using namespace regen;

// 4KB (page size) default alignment for staging buffers.
uint32_t StagingSystem::STAGING_BUFFER_ALIGNMENT = 4096u;
// Extra space adopted by the staging buffer to have some free room for dynamic changes.
float StagingSystem::STAGING_BUFFER_SLACK = 0.25;
// 256 bytes default alignment for ranges within the staging buffer
// these are mostly used in glBindBufferRange() calls.
// This is also the minimum value of fragments in the FreeList, best to
// not set it too low to avoid excessive fragmentation.
// note: initialized in constructor.
uint32_t StagingSystem::STAGING_RANGE_ALIGNMENT = 0;
// 1 second cooldown for "rare" reads
float StagingSystem::COOLDOWN_RARE_READ = 1000.0f;
// 50 ms minimum cooldown for "rare" writes
float StagingSystem::MIN_COOLDOWN_RARE_WRITE = 50;
// 500 ms maximum cooldown for "rare" reads
float StagingSystem::MAX_COOLDOWN_RARE_READ = 500.0f;
// 200 ms minimum cooldown for "never" writes
float StagingSystem::MIN_COOLDOWN_NEVER_WRITE = 200.0f;
// 2 seconds maximum cooldown for "never" writes
float StagingSystem::MAX_COOLDOWN_NEVER_WRITE = 2000.0f;

namespace regen {
	static constexpr bool STAGING_DEBUG_STALLS = false;
	static constexpr bool STAGING_EXPLICIT_FLUSH = false;
	static constexpr bool STAGING_DEBUG_TIME = false;
	static constexpr bool STAGING_DEBUG_STATISTICS = false;

	// a BO under control of the staging system
	struct StagingSystem::ManagedBO {
		BlockPtr bo = nullptr;
		// true for staged BOs
		bool isStaged = false;
		// offset in the staging buffer where the BO data starts
		uint32_t stagedOffset = 0u;
		// the size reserved in the staging buffer for this BO
		uint32_t stagedSize = 0u;
		// Records max value of update rate for adaptive moving.
		float maxUpdateRate = 0.0f;

		// define equality operator for ManagedBO
		bool operator==(const ManagedBO &other) const { return bo == other.bo; }
	};

	// a staging arena
	struct StagingSystem::Arena {
		Arena() = default;

		ArenaType type = ARENA_TYPE_LAST;
		BufferFlags flags = BufferFlags(COPY_WRITE_BUFFER);
		// accumulated size of all buffer objects in this arena, without additional alignment.
		// this will be re-computed each frame to account for dynamic changes in the buffer objects.
		uint32_t unalignedSize = 0;
		// the actual size of the ring buffer segments in this arena, with alignment applied
		// plus some extra space to handle dynamic allocation of segments without resizing.
		uint32_t alignedSize = 0;
		// the current number of segments in the ring buffer
		uint32_t numRingSegments = 2;
		// indicates if the arena has new CPU data to flush
		bool isDirty = false;
		// for rare updates, we use a cooldown to avoid updating too often.
		// this is a counter that accumulates the time since the last update, in milliseconds.
		float cooldownTime = 0.0f;
		// minimum cooldown time before the arena is updated again.
		// we initialize this to some reasonable value per arena type,
		// but also adjust it dynamically based on the actual update frequency.
		float minCooldown = 0.0f;
		float cooldownRange[2] = {0.0f, 0.0f}; // [min, max] cooldown range
		// the average update rate of the arena, in [0.0, 1.0]
		float updateRate = -1.0f;
		std::vector<ManagedBO> bufferObjects;
		ref_ptr<StagingBuffer> stagingBuffer;
		// manages available ranges in the ring buffer.
		// the ranges are relative to the buffer segments, i.e. the same offset applies to all.
		ref_ptr<FreeList> freeList;

		static Arena *create(ArenaType arenaType, ClientAccessMode accessMode);

		static void setStagingOffset(ManagedBO &managed, uint32_t offset, uint32_t size);

		void sort();

		void resize();

		bool reserve(ManagedBO &managed, uint32_t boRequiredSize) const;

		uint32_t getRangeSize(uint32_t requested) const;

		bool cooldown(float dt_ms);

		void resetUpdateHistory();

		void setMinCooldown(float v);

		void remove(const BlockPtr &bo);
	};
}

StagingSystem::StagingSystem()
		: arenas_() {
	if (STAGING_RANGE_ALIGNMENT == 0) {
		// make sure to meet all alignment requirements
		STAGING_RANGE_ALIGNMENT = std::max(16u,
										   static_cast<uint32_t>(glGetInteger(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT)));
		STAGING_RANGE_ALIGNMENT = std::max(STAGING_RANGE_ALIGNMENT,
										   static_cast<uint32_t>(glGetInteger(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT)));
		STAGING_RANGE_ALIGNMENT = std::max(STAGING_RANGE_ALIGNMENT,
										   static_cast<uint32_t>(glGetInteger(
												   GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT)));
		REGEN_INFO("Buffer alignment is " << STAGING_BUFFER_ALIGNMENT << " bytes "
										  << " and range alignment is " << STAGING_RANGE_ALIGNMENT << " bytes.");
	}
	for (auto &arena: arenas_) {
		arena = nullptr;
	}
}

StagingSystem::~StagingSystem() {
	copyInProgress_.store(false, std::memory_order_relaxed);
	for (auto &arena: arenas_) {
		delete arena;
		arena = nullptr;
	}
}

void StagingSystem::clear() {
	REGEN_INFO("Clearing staging arenas.");
	for (auto &arena: arenas_) {
		if (arena) {
			for (auto &managed: arena->bufferObjects) {
				managed.bo->resetStagingBuffer(false);
			}
			delete arena;
			arena = nullptr;
		}
	}
	copyInProgress_.store(false, std::memory_order_relaxed);
}

ref_ptr<StagingBuffer> StagingSystem::addBufferBlock(const BlockPtr &block) {
	auto &flags = block->stagingFlags();
	const auto sizeClass = StagingBuffer::getBufferSizeClass(block->updateStagedInputs());
	Arena *selectedArena = nullptr;

	if (flags.useExplicitStaging()) {
		if (flags.isReadable()) {
			selectedArena = addBufferBlock_readOnly(block, flags, sizeClass);
		} else if (flags.isWritable()) {
			selectedArena = addBufferBlock_writeOnly(block, flags, sizeClass);
		}
	} else {
		// implicit staging, use the write-only arena for never updated buffers.
		// This basically creates a virtual staging buffer that wraps the main buffer.
		selectedArena = addToArena(block, WRITE_ALMOST_NEVER);
	}

	if (selectedArena == nullptr) {
		REGEN_INFO("BO '" << block->name() << "' could not be added to staging arenas. "
						  << "No suitable arena found for flags: " << flags);
		return {};
	} else {
		return selectedArena->stagingBuffer;
	}
}

void StagingSystem::removeBufferBlock(const BlockPtr &block) {
	for (auto &arena: arenas_) {
		if (arena) arena->remove(block);
	}
}

StagingSystem::Arena *StagingSystem::addBufferBlock_readOnly(
		const BlockPtr &block,
		const BufferFlags &flags,
		BufferSizeClass /* sizeClass */) {
	if (flags.areUpdatesPerFrame()) {
		return addToArena(block, READ_PER_FRAME);
	} else if (flags.areUpdatesRare()) {
		return addToArena(block, READ_RARELY);
	}
	return nullptr;
}

StagingSystem::Arena *StagingSystem::addBufferBlock_writeOnly(
		const BlockPtr &block,
		const BufferFlags &flags,
		BufferSizeClass sizeClass) {
	if (flags.updateHints.frequency == BUFFER_UPDATE_NEVER
		|| (flags.areUpdatesRare() && sizeClass == BUFFER_SIZE_VERY_LARGE)) {
		return addToArena(block, WRITE_ALMOST_NEVER);
	} else if (flags.updateHints.frequency == BUFFER_UPDATE_PER_FRAME
			   && sizeClass == BUFFER_SIZE_VERY_LARGE) {
		return addToArena(block, WRITE_PER_FRAME_HUGE_DATA);
	} else if (flags.areUpdatesRare()) {
		return addToArena(block, WRITE_RARELY);
	} else if (flags.areUpdatesPerFrame()) {
		if (sizeClass <= BUFFER_SIZE_MEDIUM) { // MEDIUM/SMALL -> use large ring
			return addToArena(block, WRITE_PER_FRAME_SMALL_DATA);
		} else { // LARGE -> use small ring
			return addToArena(block, WRITE_PER_FRAME_LARGE_DATA);
		}
	}
	return nullptr;
}

StagingSystem::Arena *StagingSystem::Arena::create(ArenaType arenaType, ClientAccessMode accessMode) {
	auto *arena = new Arena();
	arena->type = arenaType;
	arena->flags.accessMode = accessMode;
	// we do fencing here, so disable it at buffer level
	arena->flags.syncFlags |= BUFFER_SYNC_DISABLE_FENCING;
	arena->flags.target = arena->flags.isReadable() ? COPY_READ_BUFFER : COPY_WRITE_BUFFER;

	// the maximum number of segments in the ring buffer
	uint32_t maxRingSegments = 16;

	// TODO: Special attention is needed for synchronization of different per-frame buffers when they
	//       have different number of buffer segments! Though it does not seem to be a problem so far...
	//        - the easiest way would be to use same number of segments for all per-frame buffers.
	//        - in some cases it could be useful to skip frames of buffers with less segments,
	//          but then we would get into synchronization issues.
	//        - but often it might not matter, i.e. in case there are no data dependencies.
	//          probably this should be modeled and taken into account here!
	switch (arenaType) {
		case WRITE_PER_FRAME_SMALL_DATA:
			// PER-FRAME updated SMALL to MEDIUM Staging + LARGE
			// - Use explicit staging with an adaptive frame-indexed ring buffer. It would be ok if the
			//   number of segments gets rather large, e.g. 8-16 is fine for SMALL and MEDIUM buffer.
			// - Update: Use persistent mapping with fencing
			// - Partial writes: either upgrade to FULL, or use explicit flush with a separate staging arena
			//   (that uses flush). But flushing might not be worth it for small buffers at least
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			if constexpr(STAGING_EXPLICIT_FLUSH) {
				arena->flags.mapMode = BUFFER_MAP_PERSISTENT_FLUSH;
			} else {
				arena->flags.mapMode = BUFFER_MAP_PERSISTENT_COHERENT;
			}
			arena->flags.bufferingMode = RING_BUFFER;
			arena->numRingSegments = 3;
			maxRingSegments = 16;
			break;
		case WRITE_PER_FRAME_LARGE_DATA:
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			if constexpr(STAGING_EXPLICIT_FLUSH) {
				arena->flags.mapMode = BUFFER_MAP_PERSISTENT_FLUSH;
			} else {
				arena->flags.mapMode = BUFFER_MAP_PERSISTENT_COHERENT;
			}
			arena->flags.bufferingMode = RING_BUFFER;
			arena->numRingSegments = 2;
			maxRingSegments = 4;
			break;
		case WRITE_PER_FRAME_HUGE_DATA:
			// PER-FRAME (or PER-DRAW) updated VERY-LARGE Staging
			// - Use explicit staging with a single buffer. Using multi-buffering might be too expensive.
			// - Update: avoid mapping entirely. Rather use glCopyNamedBufferSubData. No fencing is needed.
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_DISABLED; // copy instead of mapping
			arena->flags.bufferingMode = SINGLE_BUFFER;
			break;
		case WRITE_RARELY:
			// RARELY updated SMALL to LARGE Staging
			// - Use explicit staging with a single buffer. Multi buffering is not worth it for rare updates
			// - Update: use temporary mapping, no fencing needed with range invalidation!
			// - *never* user multi-buffering as only few BOs might change in a frame. that would be very wasteful!
			// note: BUFFER_SIZE_VERY_LARGE case already handled above
			arena->flags.updateHints.frequency = BUFFER_UPDATE_RARE;
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_TEMPORARY;
			arena->flags.bufferingMode = SINGLE_BUFFER;
			arena->minCooldown = 100.0f;
			arena->cooldownRange[0] = MIN_COOLDOWN_RARE_WRITE;
			arena->cooldownRange[1] = MAX_COOLDOWN_RARE_READ;
			break;
		case WRITE_ALMOST_NEVER:
			// NEVER* updated ANY SIZE + RARELY updated VERY LARGE Staging
			// - Use implicit staging: it does not make sense to keep an extra copy in staging
			// - Update: no persistent mapping, either temporary mapping or direct copy, or if writing is not allowed
			//   use a temporary buffer for writing. Fencing is never needed, assuming the data only updates once or very rarely
			// - *never* use multi-buffering as only few BOs might change in a frame. that would be very wasteful!
			arena->flags.syncFlags |= BUFFER_SYNC_IMPLICIT_STAGING;
			arena->flags.updateHints.frequency = BUFFER_UPDATE_NEVER;
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_DISABLED;
			arena->flags.bufferingMode = SINGLE_BUFFER;
			arena->minCooldown = 500.0f;
			arena->cooldownRange[0] = MIN_COOLDOWN_NEVER_WRITE;
			arena->cooldownRange[1] = MAX_COOLDOWN_NEVER_WRITE;
			break;
		case READ_PER_FRAME:
			// For reading, we just distinguish by the update frequency.
			// - use separate adaptive rings for per-frame (and per-draw)
			// - use temporary mapping with single buffer for rare reads
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_PERSISTENT_COHERENT;
			arena->flags.bufferingMode = RING_BUFFER;
			arena->numRingSegments = 2;
			maxRingSegments = 16;
			break;
		case READ_RARELY:
			arena->flags.updateHints.frequency = BUFFER_UPDATE_RARE;
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_TEMPORARY;
			arena->flags.bufferingMode = SINGLE_BUFFER;
			arena->minCooldown = COOLDOWN_RARE_READ;
			break;
		case ARENA_TYPE_LAST:
			break;
	}
	// force update on the next frame for rarely updated arenas
	arena->cooldownTime = arena->minCooldown;

	REGEN_INFO("Created \"" << arena->type << "\" arena with"
							<< " ring size: " << arena->numRingSegments << " -- " << maxRingSegments);
	REGEN_INFO("    " << arena->flags);

	// create a staging buffer for this arena
	arena->stagingBuffer = ref_ptr<StagingBuffer>::alloc(arena->flags);
	arena->stagingBuffer->setMaxRingSegments(maxRingSegments);

	return arena;
}

StagingSystem::Arena *StagingSystem::addToArena(const BlockPtr &block, ArenaType arenaType, bool isMoved) {
	if (!arenas_[arenaType]) {
		arenas_[arenaType] = Arena::create(arenaType, block->stagingFlags().accessMode);
	}
	auto &targetArena = arenas_[arenaType];
	// Note: the buffer will reserve the required size in the next loop of updateRequiredSize
	auto &managed = targetArena->bufferObjects.emplace_back();
	managed.bo = block;
	managed.isStaged = false; // not staged yet
	// disable swapping for the staging buffer, we do it manually in the staging system
	targetArena->stagingBuffer->setSwappingOnAccess(false);
	if (isMoved) {
		REGEN_DEBUG("Moved \"" << block->name()
			<< "\" to \"" << targetArena->type << "\" arena.");
	} else {
		REGEN_DEBUG("Added \"" << block->name()
			<< "\" to \"" << targetArena->type << "\" arena.");
	}
	return targetArena;
}

void StagingSystem::moveToArena(ManagedBO &managed, ArenaType targetArenaType) {
	addToArena(managed.bo, targetArenaType, true);
	// mark the BO as deleted in old arena
	managed.bo = nullptr;
	managed.isStaged = false;
	managed.maxUpdateRate = -1.0f;
}

void StagingSystem::updateBuffers() {
	// called once after all BOs were added to the system
	for (uint32_t arenaIdx = 0; arenaIdx < ARENA_TYPE_LAST; arenaIdx++) {
		auto &arena = arenas_[arenaIdx];
		if (!arena) continue; // skip uninitialized arenas

		// ensure draw buffers are allocated
		bool hasInvalidBlocks = false;
		for (uint32_t boIdx = 0; boIdx < arena->bufferObjects.size(); boIdx++) {
			auto &managed = arena->bufferObjects[boIdx];
			managed.bo->updateDrawBuffer();
			if (!managed.bo->isBufferValid()) {
				// something went wrong when updating the draw buffer.
				REGEN_WARN("Draw buffer of \"" << managed.bo->name()
											   << "\" is not valid. Skipping it in \"" << arena->type << "\" arena.");
				managed.bo = nullptr; // mark as invalid
				managed.isStaged = false;
				hasInvalidBlocks = true;
			}
		}

		// remove invalid blocks from the arena if any
		if (hasInvalidBlocks) {
			arena->bufferObjects.erase(
					std::remove_if(arena->bufferObjects.begin(), arena->bufferObjects.end(),
								   [](const ManagedBO &x) { return !x.bo; }),
					arena->bufferObjects.end());
		}

		// sort the valid BOs in the arena along draw buffer names and offsets
		arena->sort();

		// adopt a staging buffer range for this arena covering all BOs potentially
		//    with multiple segments in case of ring buffers or multi-buffering.
		updateArenaSize(arena);
		if (arena->unalignedSize > 0) {
			arena->resize();
		}
		if (!arena->stagingBuffer.get()) {
			REGEN_WARN("Failed to create staging buffer in \"" << arena->type << "\" arena"
															   << ". The arena will not be usable.");
			delete arena; // delete the arena
			arenas_[arenaIdx] = nullptr;
			continue; // skip this arena
		}

		// set the staging offset for each buffer object in the arena
		if (arena->flags.useExplicitStaging()) {
			arena->freeList = ref_ptr<FreeList>::alloc(arena->alignedSize);

			for (uint32_t boIdx = 0; boIdx < arena->bufferObjects.size(); boIdx++) {
				auto &managed = arena->bufferObjects[boIdx];

				uint32_t boAlignedSize = arena->getRangeSize(managed.bo->updateStagedInputs());
				auto [status, offset] = arena->freeList->reserve(boAlignedSize);
				if (status) {
					Arena::setStagingOffset(managed, offset, boAlignedSize);
				} else {
					REGEN_WARN("Failed to reserve staging space for buffer object '"
									   << managed.bo->name() << "' in \"" << arena->type << "\" arena"
									   << ". The arena will be disabled.");
					delete arena; // delete the arena
					arenas_[arenaIdx] = nullptr;
					continue; // skip this arena
				}
			}
		} else {
			for (auto &managed: arena->bufferObjects) {
				managed.bo->setStagingOffset(0u);
			}
		}
	}
}

bool StagingSystem::isCopyInProgress() const {
	// check if copy is in progress
	return copyInProgress_.load(std::memory_order_acquire);
}

void StagingSystem::setIsCopyInProgress() {
	copyInProgress_.store(true, std::memory_order_release);
}

void StagingSystem::scheduledCopy(const BufferCopyRange &c1) {
	if (numScheduledCopies_ > 0) {
		auto &c0 = scheduledCopies_[numScheduledCopies_ - 1];
		if (c0.srcBufferID == c1.srcBufferID
			&& c0.dstBufferID == c1.dstBufferID
			&& c0.srcOffset + c0.size == c1.srcOffset
			&& c0.dstOffset + c0.size == c1.dstOffset) {
			// merge the copy ranges
			c0.size += c1.size;
			return; // no need to schedule a new copy
		}
	}
	numScheduledCopies_ += 1;
	if (numScheduledCopies_ > scheduledCopies_.size()) {
		// resize the scheduled copies vector if needed
		scheduledCopies_.resize(numScheduledCopies_*2);
	}
	scheduledCopies_[numScheduledCopies_ - 1] = c1; // add the copy range to the scheduled copies
}

void StagingSystem::updateData(float dt_ms) {
	//copyInProgress_.store(true, std::memory_order_release);
	if constexpr(STAGING_DEBUG_TIME) {
		elapsedTime().beginFrame();
	}
	if constexpr(STAGING_DEBUG_STATISTICS) {
		stats_.numDirtyArenas = 0;
		stats_.numDirtyBOs = 0;
		stats_.numDirtySegments = 0;
		stats_.numTotalBOs = 0;
	}

	for (uint32_t arenaIdx = 0; arenaIdx < ARENA_TYPE_LAST; arenaIdx++) {
		auto &arena = arenas_[arenaIdx];
		// skip inactive arenas: those that are not initialized, and those that are cooling down.
		if (!arena || arena->cooldown(dt_ms)) continue;

		// Dynamically resize the arena if needed.
		// NOTE: the arena will also indicate size change in case of adaptive size change in ring buffers,
		//       or the arena is not large enough to hold all BOs.
		if (updateArenaSize(arena)) {
			arena->resize();
			arena->sort();
		}
		if constexpr(STAGING_DEBUG_TIME) {
			elapsedTime().push(REGEN_STRING(arena->type << " resized"));
		}
		if (!arena->flags.isReadable() && !arena->isDirty) {
			// early exit writing arenas before fencing in case of no updates.
			continue;
		}
		if constexpr(STAGING_DEBUG_STATISTICS) {
			stats_.numDirtyArenas++;
		}

		const uint32_t copyIdx = arena->stagingBuffer->nextWriteIndex();
		const uint32_t drawIdx = arena->stagingBuffer->nextReadIndex();
		// we do manual synchronization in case of persistent mapping arenas.
		const bool useFence = isMapModePersistent(arena->flags.mapMode);
		// For now, reading arenas must not be marked dirty, it is assumed the draw
		// buffer is written to every frame.
		// Reason: the use as output buffer is currently not tracked, but could be done to mark
		// GPU write buffers as dirty -- but must be careful with syncing then!
		const bool forceUpdate = arena->flags.isReadable();

		// Wait for the fence in case of persistent mapped arenas.
		// This might block the CPU in case of the last write into this segment
		// has not been consumed by the GPU yet.
		// TODO: The interaction with the fence still consumes a lot of CPU time.
		//		- The main bottleneck now seems *setFencePoint*. Reason might be that
		//          we do glDeleteSync/glFenceSync calls every time setFencePoint is called.
		//		- As far as I know, we cannot re-use fences across frames.
		//		- Maybe the only way to improve would be to reduce the number of fences.
		//      - Idea: Let arenas share fences. However, this is difficult because arenas
		//        currently may have ring buffers of different sizes.
		//		- Maybe the mechanism can be adjusted such that we do not need a fence every
		//          frame for every arena.
		if (useFence) {
			arena->stagingBuffer->fence(copyIdx).wait();
		}

		// Copy data from CPU to staging to draw buffer,
		// or in case of reading, the other way around.
		for (auto &managed: arena->bufferObjects) {
			// NOTE: temporary mapping is only used for rare updates,
			//       so it is not really worth it to consider temporary mapping on arena level.
			// NOTE: This will only copy data if the BO is dirty, i.e. has new data to write.
			//if (managed.bo->hasDirtySegments()) {
			//	REGEN_INFO("Dirty BO: " << managed.bo->name());
			//}
			if constexpr(STAGING_DEBUG_STATISTICS) {
				if (managed.bo->hasDirtySegments()) {
					stats_.numDirtyBOs++;
					stats_.numDirtySegments += managed.bo->numDirtySegments();
				}
				stats_.numTotalBOs++;
			}
			managed.bo->copyStagingData(forceUpdate);
		}

		// Do the actual copy from staging to draw buffer.
		// We do this here as we attempted to coalesce the copy ranges into
		// larger contiguous ranges for fewer copies.
		for (uint32_t scheduleIdx = 0; scheduleIdx < numScheduledCopies_; scheduleIdx++) {
			auto &copy = scheduledCopies_[scheduleIdx];
			glCopyNamedBufferSubData(
					copy.srcBufferID, copy.dstBufferID,
					copy.srcOffset, copy.dstOffset, copy.size);
			//REGEN_INFO("Scheduled copy " << copy);
		}
		numScheduledCopies_ = 0; // reset scheduled copies
		if constexpr(STAGING_DEBUG_TIME) {
			elapsedTime().push(REGEN_STRING(arena->type << " copied"));
		}

		// Create a fence just after glCopyNamedBufferSubData -- marking the point where the
		// written data of this frame has been consumed by the GPU.
		if (useFence) {
			arena->stagingBuffer->fence(drawIdx).setFencePoint();
		}
		if constexpr(STAGING_DEBUG_TIME) {
			elapsedTime().push(REGEN_STRING(arena->type << " synced"));
		}

		// Advance to next segment in case of multi-buffering and ring buffers.
		arena->stagingBuffer->swapBuffers();
		arena->isDirty = false; // reset dirty flag
		if constexpr(STAGING_DEBUG_TIME) {
			elapsedTime().push(REGEN_STRING(arena->type << " swapped"));
		}

		if constexpr(STAGING_DEBUG_STALLS) {
			if (useFence) {
				REGEN_INFO("Arena " << arena->type << " stall rate: "
					<< arena->stagingBuffer->fence(copyIdx).getStallRate());
			}
			REGEN_INFO("Arena " << arena->type << " fragmentation: "
				<< arena->freeList->getFragmentationScore());
		}
	}

	if constexpr (!ANIMATION_THREAD_SWAPS_CLIENT_BUFFERS) {
		// finally, swap client buffers
		swapClientData();
	}

	if constexpr(STAGING_DEBUG_TIME) {
		elapsedTime().push("client swapped");
		elapsedTime().endFrame();
	}
	copyInProgress_.store(false, std::memory_order_release);

	if constexpr(STAGING_DEBUG_STATISTICS) {
		REGEN_INFO("Copied "
			<< stats_.numDirtyBOs << " (" << stats_.numTotalBOs << ") BO(s) with "
			<< stats_.numDirtySegments << " dirty segments in "
			<< stats_.numDirtyArenas << " arenas. ");
	}
}

void StagingSystem::swapClientData() {
	if constexpr(STAGING_DEBUG_STATISTICS) {
		stats_.numSwapCopies = 0;
	}
	for (uint32_t arenaIdx = 0; arenaIdx < ARENA_TYPE_LAST; arenaIdx++) {
		auto &arena = arenas_[arenaIdx];
		if (!arena) continue; // skip uninitialized arenas

		for (auto &managed: arena->bufferObjects) {
			if constexpr(STAGING_DEBUG_STATISTICS) {
				stats_.numSwapCopies += managed.bo->clientBuffer()->swapData();
			} else {
				managed.bo->clientBuffer()->swapData();
			}
		}
	}
	if constexpr(STAGING_DEBUG_STATISTICS) {
		REGEN_INFO("Client swap required " << stats_.numSwapCopies << " copies.");
	}
}

bool StagingSystem::moveAdaptive(Arena *arena, ManagedBO &managed, float boUpdateRate) {
	managed.maxUpdateRate = std::max(boUpdateRate, managed.maxUpdateRate);

	if (arena->type < READ_PER_FRAME) { // this is a per-frame writing arena
		if (managed.maxUpdateRate < 0.25f) {
			// If the BO was max updated less than 25% of the frames, we move it to the rare update arena.
			// NOTE: we rather compare here with the max update rate. The reason being that there can be controller
			// that don't move an object for multiple seconds which might drain update rate to zero.
			// However, then the object may start moving again, but if it ended up on a stage with cooldown
			// it might take a long time until it is moved to PER-FRAME stage again (especially because
			// it takes n samples after moving until the BO computes an update rate again).
			moveToArena(managed, WRITE_RARELY);
			return true;
		}
	} else if (arena->type == WRITE_RARELY) {
		// NOTE: cooldown rate influences the update rate! That makes it a bit more difficult to
		//       make this stable. So if 95%, it does not mean 95% of all frames, but rather 95% of the
		//       frames where the arena was not cooling down.
		if (boUpdateRate > 0.95f) {
			// if the BO is updated more than 95% of the time, we can promote it to the per-frame arena.
			const auto sizeClass =
					StagingBuffer::getBufferSizeClass(managed.bo->drawBufferSize());
			const ArenaType targetArena = (sizeClass == BUFFER_SIZE_LARGE ?
										   WRITE_PER_FRAME_LARGE_DATA : (sizeClass == BUFFER_SIZE_VERY_LARGE ?
																		 WRITE_PER_FRAME_HUGE_DATA :
																		 WRITE_PER_FRAME_SMALL_DATA));
			moveToArena(managed, targetArena);
			return true;
		} else if (boUpdateRate < 0.1f) {
			// if the BO is updated less than 10% of the time, we can demote it to the never updated arena.
			moveToArena(managed, WRITE_ALMOST_NEVER);
			return true;
		}
	} else if (arena->type == WRITE_ALMOST_NEVER) {
		if (boUpdateRate > 0.75f) {
			// if the BO is updated more than 5% of the time, we can promote it to the rare update arena.
			moveToArena(managed, WRITE_RARELY);
			return true;
		}
	}
	return false;
}

bool StagingSystem::Arena::reserve(ManagedBO &managed, uint32_t boRequiredSize) const {
	auto [status, offset] = freeList->reserve(boRequiredSize);
	if (status) {
		// found some free space in the arena, nice!
		Arena::setStagingOffset(managed, offset, boRequiredSize);
		return true;
	} else {
		return false;
	}
}

bool StagingSystem::updateArenaSize(Arena *arena) {
	uint32_t newUnalignedSize = 0u;
	uint32_t boAlignedSize = 0u;
	bool forceResize = false;
	bool hasInvalidBlocks = false;
	float boUpdateRate;

	arena->updateRate = 0.0f;
	for (uint32_t boIdx = 0; boIdx < arena->bufferObjects.size(); boIdx++) {
		auto &managed = arena->bufferObjects[boIdx];
		boUpdateRate = managed.bo->getUpdateRate();

		// first move around BOs based on update rate.
		if (boUpdateRate > -0.5f) {
			if (moveAdaptive(arena, managed, boUpdateRate)) {
				hasInvalidBlocks = true;
				arena->unalignedSize -= managed.stagedSize;
				continue; // skip this BO, as it was moved to another arena
			}
			arena->updateRate += boUpdateRate;
		}

		// update dirty segments + size of the BO
		boAlignedSize = arena->getRangeSize(managed.bo->updateStagedInputs());
		newUnalignedSize += boAlignedSize;
		arena->isDirty = arena->isDirty || managed.bo->hasDirtySegments();

		if (!managed.isStaged) {
			if (arena->flags.useExplicitStaging()) {
				// the BO did not yet reserve memory in the staging arena,
				// we need to reserve it now. If it fails, we need to resize the arena.
				if (!arena->freeList.get()) {
					// arena is not initialized yet.
					forceResize = true;
				} else if (!arena->reserve(managed, boAlignedSize)) {
					// force resize of the arena, as BO could not reserve memory in the staging arena.
					forceResize = true;
					REGEN_INFO("BO '" << managed.bo->name()
									  << "' is too large in \"" << arena->type << "\" arena."
									  << " Required: " << boAlignedSize / 1024.0f << " KiB"
									  << " but only " << arena->freeList->getMaxFreeSize() / 1024.0f << " KiB free.");
				}
			}
		} else if (managed.stagedSize != boAlignedSize) {
			// BO changed its size -> try to find a new place for it.
			// first release the allocated space.
			arena->freeList->release(managed.stagedSize, managed.stagedOffset);
			arena->unalignedSize -= managed.stagedSize;
			// then try to reserve new space for the BO.
			if (!arena->reserve(managed, boAlignedSize)) {
				// force resize of the arena, as BO could not reserve memory in the staging arena.
				forceResize = true;
				REGEN_INFO("BO '" << managed.bo->name()
									  << "' is too large in \"" << arena->type << "\" arena."
									  << " Required: " << boAlignedSize / 1024.0f << " KiB"
									  << " but only " << arena->freeList->getMaxFreeSize() / 1024.0f << " KiB free.");
			} else {
				// successfully reserved new space, update the unaligned size
				arena->unalignedSize += boAlignedSize;
				REGEN_INFO("Moved '" << managed.bo->name()
										<< "' within \"" << arena->type << "\" arena!");
			}
		}
	}
	if (hasInvalidBlocks) {
		// remove invalid blocks from the arena if any
		arena->bufferObjects.erase(
				std::remove_if(arena->bufferObjects.begin(), arena->bufferObjects.end(),
							   [](const ManagedBO &managed) { return !managed.bo; }),
				arena->bufferObjects.end());
	}
	if (!arena->bufferObjects.empty()) {
		arena->updateRate /= static_cast<float>(arena->bufferObjects.size());
	}

	if (newUnalignedSize != arena->unalignedSize || forceResize) {
		// Finally compute the new size of the arena, align this to page size, and
		// make some extra space for dynamic allocation of BOs.
		// IDEA: if there is a BO that changes frequently, move it to the end of the arena where it can grow.
		arena->unalignedSize = newUnalignedSize;
		// Only align up if we do explicit staging
		if (arena->flags.useExplicitStaging()) {
			arena->alignedSize = alignUp(
					static_cast<uint32_t>(static_cast<float>(arena->unalignedSize) * (1.0 + STAGING_BUFFER_SLACK)),
					STAGING_BUFFER_ALIGNMENT);
		} else {
			// for implicit staging, we do not align up, but just use the unaligned size.
			arena->alignedSize = arena->unalignedSize;
		}
		REGEN_INFO("Resizing \"" << arena->type
								 << "\" arena with " << arena->bufferObjects.size() << " BOs"
								 << " to " << arena->alignedSize / 1024.0f << " KiB per segment."
								 << " Unaligned: " << arena->unalignedSize / 1024.0f << " KiB");
		return true; // size changed
	}

	// size did not change, next check if there is too much stall in the ring buffer.
	const uint32_t copyIdx = arena->stagingBuffer->nextWriteIndex();
	const float stallRate = arena->stagingBuffer->fence(copyIdx).getStallRate();
	if (stallRate > StagingBuffer::MAX_ACCEPTABLE_STALL_RATE) {
		// if the stall rate is too high, we need to increase the number of segments in the ring buffer.
		// this will be done in resize() function.
		uint32_t newNumSegments = std::min(arena->numRingSegments + 1u, arena->stagingBuffer->maxRingSegments());
		if (newNumSegments != arena->numRingSegments) {
			REGEN_INFO("High stall rate (" << stallRate << ") detected in \"" << arena->type
										   << "\" arena with " << arena->numRingSegments << " segments"
										   << ", increasing to " << newNumSegments);
			arena->numRingSegments = newNumSegments;
			arena->stagingBuffer->resetStallRate();
			return true; // size changed
		}
	}

	return false;
}

void StagingSystem::Arena::setStagingOffset(ManagedBO &managed, uint32_t offset, uint32_t size) {
	// set the staging offset for the buffer object
	managed.bo->setStagingOffset(offset);
	managed.isStaged = true; // mark as staged
	managed.stagedOffset = offset;
	managed.stagedSize = size;
}

uint32_t StagingSystem::Arena::getRangeSize(uint32_t requested) const {
	// update dirty segments + size of the BO
	if (flags.useExplicitStaging()) {
		// Only align up if we do explicit staging
		return alignUp(requested, STAGING_RANGE_ALIGNMENT);
	} else {
		// for implicit staging, we do not align up, but just use the unaligned size.
		return requested;
	}
}

void StagingSystem::Arena::remove(const BlockPtr &bo) {
	// remove the block from the arena
	auto it = std::find(bufferObjects.begin(), bufferObjects.end(), ManagedBO{bo});
	if (it != bufferObjects.end()) {
		if (freeList.get()) {
			freeList->release(it->stagedSize, it->stagedOffset);
		}
		bufferObjects.erase(it);
		bo->resetStagingBuffer(false);
		REGEN_INFO("Removed buffer block '" << bo->name() << "'"
											<< " from \"" << type << "\" arena");
	}
}

void StagingSystem::Arena::resize() {
	// the size of a draw buffer range has changed, or the number of segments in the ring buffer.
	// in this case we will orphan any adopted staging buffer ranges, and re-adopt one with the new size.
	if (unalignedSize == 0u || alignedSize == 0u) {
		REGEN_WARN("Attempting to resize \"" << type << "\" arena"
											 << " to zero Bytes. This is likely a bug.");
		return;
	}
	stagingBuffer->resizeBuffer(alignedSize, numRingSegments);

	if (flags.useExplicitStaging()) {
		if (freeList.get()) {
			freeList->clear(alignedSize);
		} else {
			freeList = ref_ptr<FreeList>::alloc(alignedSize);
		}
		sort();

		uint32_t boAlignedSize;
		for (auto &managed: bufferObjects) {
			managed.bo->updateDrawBuffer();

			boAlignedSize = getRangeSize(managed.bo->updateStagedInputs());
			auto [_, offset] = freeList->reserve(boAlignedSize);
			setStagingOffset(managed, offset, boAlignedSize);
		}
	} else {
		for (auto &managed: bufferObjects) {
			managed.bo->updateDrawBuffer();
		}
	}
}

void StagingSystem::Arena::sort() {
	std::sort(bufferObjects.begin(), bufferObjects.end(),
			  [](const ManagedBO &a, const ManagedBO &b) {
				  // sort buffer blocks starting with lower draw buffer names,
				  // and then by draw buffer address smaller first.
				  if (a.bo->drawBufferName() != b.bo->drawBufferName()) {
					  return a.bo->drawBufferName() < b.bo->drawBufferName();
				  }
				  return (a.bo->drawBufferAddress() < b.bo->drawBufferAddress());
			  });
}

bool StagingSystem::Arena::cooldown(float dt_ms) {
	if (updateRate < -0.01f) return false; // no updates, no cooldown

	// Avoid updating arenas with RARELY/NEVER updated BOs every frame.
	if (type > READ_RARELY) {
		cooldownTime += dt_ms;
		// skip this arena if it is not time to update it yet
		if (cooldownTime < minCooldown) { return true; }

		// mapping from update rate to cooldown time,
		// smooth with current value based on time
		const float targetCooldown = cooldownRange[0] +
			(1.0f - updateRate) * (cooldownRange[1] - cooldownRange[0]);
		const float alpha = 1.0f - expf(-dt_ms / 200.0f);
		setMinCooldown(alpha*targetCooldown + (1.0f-alpha)*minCooldown);
	} else if (type == READ_RARELY) {
		// note: cooldown is not adaptive for reading.
		cooldownTime += dt_ms;
		// skip this arena if it is not time to update it yet
		if (cooldownTime < minCooldown) { return true; }
		// reset cooldown
		cooldownTime = 0.0f;
	}

	// not cooling down, we can update the arena
	return false;
}

void StagingSystem::Arena::setMinCooldown(float v) {
	if (minCooldown != v) {
		minCooldown = v;
		resetUpdateHistory();
	}
}

void StagingSystem::Arena::resetUpdateHistory() {
	for (auto &managed: bufferObjects) {
		managed.bo->resetUpdateHistory();
	}
	updateRate = -1.0f; // reset update rate
}

std::ostream &regen::operator<<(std::ostream &out, const StagingSystem::ArenaType &v) {
	switch (v) {
		case StagingSystem::WRITE_PER_FRAME_LARGE_DATA:
			out << "PER_FRAME_LARGE_W";
			break;
		case StagingSystem::WRITE_PER_FRAME_SMALL_DATA:
			out << "PER_FRAME_SMALL_W";
			break;
		case StagingSystem::WRITE_PER_FRAME_HUGE_DATA:
			out << "PER_FRAME_HUGE_W";
			break;
		case StagingSystem::READ_PER_FRAME:
			out << "PER_FRAME_R";
			break;
		case StagingSystem::READ_RARELY:
			out << "RARELY_R";
			break;
		case StagingSystem::WRITE_RARELY:
			out << "RARELY_W";
			break;
		case StagingSystem::WRITE_ALMOST_NEVER:
			out << "STATIC";
			break;
		case StagingSystem::ARENA_TYPE_LAST:
			out << "?";
			break;
	}
	return out;
}
