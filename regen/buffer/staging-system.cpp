#include "staging-system.h"
#include <regen/gl-types/gl-param.h>

#define REGEN_STAGING_SYSTEM_DEBUG

using namespace regen;

static uint32_t getStagingAlignment() {
	// Get the OpenGL alignment for uniform and shader storage buffers.
	// This is the minimum alignment required for staging buffers.
	static const uint32_t v = std::max(256u, std::max(
			glParam<uint32_t>(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT),
			glParam<uint32_t>(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT)));
	return v;
}

StagingSystem::StagingSystem()
		: arenas_() {
	for (auto &arena: arenas_) {
		arena = nullptr;
	}
}

StagingSystem::~StagingSystem() {
	for (auto &arena: arenas_) {
		delete arena;
		arena = nullptr;
	}
}

void StagingSystem::clear() {
	REGEN_INFO("Clearing staging arenas.");
	for (auto &arena: arenas_) {
		if (arena) {
			delete arena;
			arena = nullptr;
		}
	}
	arenas_.fill(nullptr);
}

ref_ptr<StagingBuffer> StagingSystem::addBufferBlock(const BlockPtr &block) {
	auto &flags = block->stagingFlags();
	auto sizeClass = StagingBuffer::getBufferSizeClass(block->updateBlockInputs());
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
		selectedArena = addToArena(block, ARENA_WRITE_NEVER_CP_NB);
	}

	if (selectedArena == nullptr) {
		REGEN_INFO("buffer block '" << block->getBlockName()
									<< "' could not be added to staging arenas. "
									<< "No suitable arena found for flags: " << flags);
		return {};
	} else {
		return selectedArena->stagingBuffer;
	}
}

StagingSystem::Arena *StagingSystem::addBufferBlock_readOnly(
		const BlockPtr &block,
		const BufferFlags &flags,
		BufferSizeClass /* sizeClass */) {
	// For reading, we just distinguish by the update frequency.
	// - use separate adaptive rings for per-frame (and per-draw)
	// - use temporary mapping with single buffer for rare reads
	if (flags.areUpdatesPerFrame()) {
		return addToArena(block, ARENA_READ_PER_FRAME_PM_RNG);
	} else if (flags.areUpdatesRare()) {
		return addToArena(block, ARENA_READ_RARE_TM_SB);
	}
	return nullptr;
}

StagingSystem::Arena *StagingSystem::addBufferBlock_writeOnly(
		const BlockPtr &block,
		const BufferFlags &flags,
		BufferSizeClass sizeClass) {
	// NEVER* updated ANY SIZE + RARELY updated VERY LARGE Staging
	// - Use implicit staging: it does not make sense to keep an extra copy in staging
	// - Update: no persistent mapping, either temporary mapping or direct copy, or if writing is not allowed
	//   use a temporary buffer for writing. Fencing is never needed, assuming the data only updates once or very rarely
	// - *never* use multi-buffering as only few BOs might change in a frame. that would be very wasteful!
	if (flags.updateHints.frequency == BUFFER_UPDATE_NEVER
		|| (flags.areUpdatesRare() && sizeClass == BUFFER_SIZE_VERY_LARGE)) {
		return addToArena(block, ARENA_WRITE_NEVER_CP_NB);
	}
		// PER-FRAME (or PER-DRAW) updated VERY-LARGE Staging
		// - Use explicit staging with a single buffer. Using multi-buffering might be too expensive.
		// - Update: avoid mapping entirely. Rather use glCopyNamedBufferSubData. No fencing is needed.
	else if (flags.updateHints.frequency == BUFFER_UPDATE_PER_FRAME
			 && sizeClass == BUFFER_SIZE_VERY_LARGE) {
		return addToArena(block, ARENA_WRITE_PER_FRAME_CP_SB);
	}
		// RARELY updated SMALL to LARGE Staging
		// - Use explicit staging with a single buffer. Multi buffering is not worth it for rare updates
		// - Update: use temporary mapping, no fencing needed with range invalidation!
		// - *never* user multi-buffering as only few BOs might change in a frame. that would be very wasteful!
	else if (flags.areUpdatesRare()) {
		// note: BUFFER_SIZE_LARGE case already handled above
		return addToArena(block, ARENA_WRITE_RARE_TM_SB);
	}
		// PER-FRAME updated SMALL to MEDIUM Staging + LARGE
		// - Use explicit staging with an adaptive frame-indexed ring buffer. It would be ok if the
		//   number of segments gets rather large, e.g. 8-16 is fine for SMALL and MEDIUM buffer.
		// - Update: Use persistent mapping with fencing
		// - Partial writes: either upgrade to FULL, or use explicit flush with a separate staging arena
		//   (that uses flush). But flushing might not be worth it for small buffers at least
	else if (flags.areUpdatesPerFrame()) {
		// FIXME: Special attention is needed for synchronization of different per-frame buffers when they
		//        have different number of buffer segments!
		//        - the easiest way would be to use same number of segments for all per-frame buffers.
		//        - in some cases it could be useful to skip frames of buffers with less segments,
		//          but then we would get into synchronization issues.
		//        - but often it might not matter, i.e. in case there are no data dependencies.
		//          probably this should be modeled and taken into account here!
		if (sizeClass <= BUFFER_SIZE_MEDIUM) { // MEDIUM/SMALL -> use large ring
			return addToArena(block, ARENA_WRITE_PER_FRAME_PM_LARGE_RNG);
		} else { // LARGE -> use small ring
			// Note: PAR and FUL need to be separated, as the storage flags are different
			//     (PAR uses FLUSH, FUL uses COHERENT)
			if (flags.areUpdatesPartial()) {
				return addToArena(block, ARENA_WRITE_PAR_PER_FRAME_PM_SMALL_RNG);
			} else {
				return addToArena(block, ARENA_WRITE_FUL_PER_FRAME_PM_SMALL_RNG);
			}
		}
	}
	return nullptr;
}

StagingSystem::Arena *StagingSystem::createArena(ArenaType arenaType, BufferAccessMode accessMode) {
	auto *arena = new Arena();
	arena->type = arenaType;
	arena->flags.target = arena->flags.isReadable() ? COPY_READ_BUFFER : COPY_WRITE_BUFFER;
	arena->flags.accessMode = accessMode;
	// we do fencing here, so disable it at buffer level
	arena->flags.syncFlags |= BUFFER_SYNC_DISABLE_FENCING;

	// the maximum number of segments in the ring buffer
	uint32_t maxRingSegments = 16;

	switch (arenaType) {
		case ARENA_WRITE_FUL_PER_FRAME_PM_SMALL_RNG:
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			arena->flags.updateHints.scope = BUFFER_UPDATE_FULLY;
			arena->flags.mapMode = BUFFER_MAP_PERSISTENT_COHERENT;
			arena->flags.bufferingMode = RING_BUFFER;
			arena->numRingSegments = 2;
			maxRingSegments = 4;
			break;
		case ARENA_WRITE_PAR_PER_FRAME_PM_SMALL_RNG:
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_PERSISTENT_FLUSH;
			arena->flags.bufferingMode = RING_BUFFER;
			arena->numRingSegments = 2;
			maxRingSegments = 4;
			break;
		case ARENA_WRITE_PER_FRAME_PM_LARGE_RNG:
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			// NOTE: partial updates are upgraded to full upgrades
			arena->flags.updateHints.scope = BUFFER_UPDATE_FULLY;
			arena->flags.mapMode = BUFFER_MAP_PERSISTENT_COHERENT;
			arena->flags.bufferingMode = RING_BUFFER;
			arena->numRingSegments = 3;
			maxRingSegments = 16;
			break;
		case ARENA_WRITE_PER_FRAME_CP_SB:
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			// NOTE: partial and full updates are ok as no mapping is done!
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_DISABLED; // copy instead of mapping
			arena->flags.bufferingMode = SINGLE_BUFFER;
			break;
		case ARENA_READ_PER_FRAME_PM_RNG:
			arena->flags.updateHints.frequency = BUFFER_UPDATE_PER_FRAME;
			arena->flags.updateHints.scope = BUFFER_UPDATE_FULLY;
			arena->flags.mapMode = BUFFER_MAP_PERSISTENT_COHERENT;
			arena->flags.bufferingMode = RING_BUFFER;
			arena->numRingSegments = 2;
			maxRingSegments = 16;
			break;
		case ARENA_READ_RARE_TM_SB:
		case ARENA_WRITE_RARE_TM_SB:
			arena->flags.updateHints.frequency = BUFFER_UPDATE_RARE;
			// NOTE: partial and full updates are ok for writing!
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_TEMPORARY;
			arena->flags.bufferingMode = SINGLE_BUFFER;
			break;
		case ARENA_WRITE_NEVER_CP_NB:
			// no staging buffer -> implicit staging
			arena->flags.syncFlags |= BUFFER_SYNC_IMPLICIT_STAGING;
			arena->flags.updateHints.frequency = BUFFER_UPDATE_NEVER;
			// NOTE: partial and full updates are ok for writing!
			arena->flags.updateHints.scope = BUFFER_UPDATE_PARTIALLY;
			arena->flags.mapMode = BUFFER_MAP_DISABLED;
			arena->flags.bufferingMode = SINGLE_BUFFER;
			break;
		case ARENA_TYPE_LAST:
			break;
	}
	REGEN_INFO("Created staging arena \"" << arena->type << "\" with"
		<< " ring size: " << arena->numRingSegments << " -- " << maxRingSegments);

	// create a staging buffer for this arena
	arena->stagingBuffer = ref_ptr<StagingBuffer>::alloc(arena->flags);
	arena->stagingBuffer->setMaxRingSegments(maxRingSegments);

	return arena;
}

StagingSystem::Arena *StagingSystem::addToArena(const BlockPtr &block, ArenaType arenaType) {
	if (!arenas_[arenaType]) {
		arenas_[arenaType] = createArena(arenaType, block->stagingFlags().accessMode);
	}
	auto &targetArena = arenas_[arenaType];
	targetArena->bufferObjects.push_back(block);
	// disable swapping for the staging buffer, we do it manually in the staging system
	targetArena->stagingBuffer->setSwappingOnAccess(false);
	REGEN_INFO("Added buffer block \"" << block->getBlockName()
		<< "\" to staging arena \"" << targetArena->type << "\"");
	return targetArena;
}

void StagingSystem::updateBuffers() {
	// called once after all BOs were added to the system
	for (uint32_t arenaIdx = 0; arenaIdx < ARENA_TYPE_LAST; arenaIdx++) {
		auto &arena = arenas_[arenaIdx];
		if (!arena) continue; // skip uninitialized arenas

		// ensure draw buffers are allocated
		bool hasInvalidBlocks = false;
		for (uint32_t boIdx = 0; boIdx < arena->bufferObjects.size(); boIdx++) {
			auto &bo = arena->bufferObjects[boIdx];
			bo->updateDrawBuffer();
			if (!bo->isBlockValid()) {
				// something went wrong when updating the draw buffer.
				REGEN_WARN("BufferBlock \"" << bo->getBlockName()
						<< "\" is not valid. Skipping it in staging arena "
						<< arena->type);
				arena->bufferObjects[boIdx] = {};
				hasInvalidBlocks = true;
			}
		}

		// remove invalid blocks from the arena if any
		if (hasInvalidBlocks) {
			arena->bufferObjects.erase(
					std::remove_if(arena->bufferObjects.begin(), arena->bufferObjects.end(),
								   [](const BlockPtr &bo) { return !bo; }),
					arena->bufferObjects.end());
		}

		// sort the valid BOs in the arena along draw buffer names and offsets
		arena->sort();

		// adopt a staging buffer range for this arena covering all BOs potentially
		//    with multiple segments in case of ring buffers or multi-buffering.
		arena->updateRequiredSize();
		if (arena->requiredSize > 0) {
			arena->resize();
		}
		if (!arena->stagingBuffer.get()) {
			// create a staging buffer for this arena
			REGEN_WARN("Failed to create staging buffer for arena type " << arena->type
																		 << ". The arena will not be usable.");
			delete arena; // delete the arena
			arenas_[arenaIdx] = nullptr;
			continue; // skip this arena
		}

		// set the staging offset for each buffer object in the arena
		uint32_t localOffset = 0;
		for (auto &bo: arena->bufferObjects) {
			bo->setStagingOffset(localOffset);
			localOffset += bo->drawBufferSize();
		}
	}
	REGEN_INFO("Staging arenas updated.");
}

void StagingSystem::updateData() {
	// TODO: It would be ok to occasionally skip updates for rare/never updated arenas.
	//       It might be good in general to only iterate those every N frames instead of every frame.

	for (uint32_t arenaIdx = 0; arenaIdx < ARENA_TYPE_LAST; arenaIdx++) {
		auto &arena = arenas_[arenaIdx];
		if (!arena) continue; // skip uninitialized arenas
		// Dynamically resize the arena if needed.
		// NOTE: the arena will also indicate size change in case of adaptive size change in ring buffers,
		//       or of BOs were added, removed, or have changed their size.
		if (arena->updateRequiredSize()) {
			REGEN_INFO("Staging arena size changed.");
			arena->resize();
			arena->sort();
		}
		if (!arena->isDirty) {
			// early exit before fence in case of no updates
			continue;
		}

		const uint32_t copyIdx = arena->stagingBuffer->nextWriteIndex();
		const uint32_t drawIdx = arena->stagingBuffer->nextReadIndex();
		const bool useFence = arena->flags.useSyncFences() && isMapModePersistent(arena->flags.mapMode);

		// Wait for the fence in case of persistent mapped arenas.
		// This might block the CPU in case of the last write into this segment
		// has not been consumed by the GPU yet.
		if (useFence && !arena->stagingBuffer->fence(copyIdx).wait(false)) {
			continue; // drop frame
		}

		// Copy data from CPU to staging to draw buffer,
		// or in case or reading, the other way around.
		for (auto &bo: arena->bufferObjects) {
			// NOTE: temporary mapping is only used for rare updates,
			//       so it is not really worth it to consider temporary mapping on arena level.
			bo->copyStagingData();
		}

		// Create a fence just after glCopyNamedBufferSubData -- marking the point where the
		// written data of this frame has been consumed by the GPU.
		if (useFence) {
			arena->stagingBuffer->fence(drawIdx).setFencePoint();
		}

		// Advance to next segment in case of multi-buffering and ring buffers.
		arena->stagingBuffer->swapBuffers();

#ifdef REGEN_STAGING_SYSTEM_DEBUG
		if (useFence) {
			REGEN_INFO("Arena " << arena->type
				<< " stall rate: " << arena->stagingBuffer->fence(copyIdx).getStallRate());
		}
#endif
	}
}

bool StagingSystem::Arena::updateRequiredSize() {
	uint32_t newRequiredSize = 0u;
	isDirty = false; // reset dirty flag
	for (const auto &bo: bufferObjects) {
		newRequiredSize += bo->updateBlockInputs();
		isDirty = isDirty || bo->hasDirtySegments();
	}
	// align up to meet requirements for different buffer types,
	// and generally to improve performance.
	newRequiredSize = alignUp(newRequiredSize, getStagingAlignment());

	if (newRequiredSize != requiredSize) {
		requiredSize = newRequiredSize;
		return true; // size changed
	}

	// size did not change, next check if there is too much stall in the ring buffer.
	const uint32_t copyIdx = stagingBuffer->nextWriteIndex();
	if (stagingBuffer->fence(copyIdx).getStallRate() > StagingBuffer::MAX_ACCEPTABLE_STALL_RATE) {
		// if the stall rate is too high, we need to increase the number of segments in the ring buffer.
		// this will be done in resize() function.
		uint32_t newNumSegments = std::min(numRingSegments + 1u, stagingBuffer->maxRingSegments());
		if (newNumSegments != numRingSegments) {
			REGEN_INFO("Resizing staging arena " << type
				<< " from " << numRingSegments << " segments to "
				<< newNumSegments << " segments due to high stall rate.");
			numRingSegments = newNumSegments;
			stagingBuffer->resetStallRate();
			return true; // size changed
		}
	}

	return false;
}

void StagingSystem::Arena::sort() {
	std::sort(bufferObjects.begin(), bufferObjects.end(),
			  [](const BlockPtr &a, const BlockPtr &b) {
				  // sort buffer blocks starting with lower draw buffer names,
				  // and then by draw buffer address smaller first.
				  if (a->drawBufferName() != b->drawBufferName()) {
					  return a->drawBufferName() < b->drawBufferName();
				  }
				  return (a->drawBufferAddress() < b->drawBufferAddress());
			  });
}

void StagingSystem::Arena::resize() {
	// the size of a draw buffer range has changed, or the number of segments in the ring buffer.
	// in this case we will orphan any adopted staging buffer ranges, and re-adopt one with the new size.
	if (requiredSize == 0u) {
		REGEN_WARN("Attempting to resize staging arena " << type
				<< " to zero Bytes. This is likely a bug.");
		return; // nothing to resize
	}
	REGEN_INFO("Resizing staging arena \"" << type << "\""
		<< " with " << bufferObjects.size() << " BOs"
		<< " to " << requiredSize / 1024.0f << " KiB per segment.");
	stagingBuffer->resizeBuffer(requiredSize, numRingSegments);

	uint32_t localOffset = 0;
	for (auto &bo: bufferObjects) {
		bo->updateDrawBuffer();

		// TODO: Come up with a mechanism to promote or demote BOs to/from staging buffers.
		//	   - Something along the lines of:
		/**
		auto updateRate = bo->getUpdateRate();
		if (updateRate > 0.0f) {
			if (!isNeverArena() && updateRate < 0.001f) {
				denote(ARENA_WRITE_NEVER_CP_NB);
			} else if (isPerFrameArena() && updateRate < 0.9f) {
				denote(ARENA_READ_RARE_TM_SB);
			} else if (!isPerFrameArena() && updateRate > 0.95f) {
				promote(XXX_PER_FRAME_XXX);
			}
		}
		**/

		// set the offset where this BO starts in each segment of the staging buffer.
		bo->setStagingOffset(localOffset);
		localOffset += bo->drawBufferSize();
	}
}

std::ostream &regen::operator<<(std::ostream &out, const StagingSystem::ArenaType &v) {
	switch (v) {
		case StagingSystem::ARENA_WRITE_FUL_PER_FRAME_PM_SMALL_RNG:
			out << "w-FUL-FRA-PM-RNG_SML";
			break;
		case StagingSystem::ARENA_WRITE_PAR_PER_FRAME_PM_SMALL_RNG:
			out << "w-PAR-FRA-PM-RNG_SML";
			break;
		case StagingSystem::ARENA_WRITE_PER_FRAME_PM_LARGE_RNG:
			out << "w-ANY-FRA-PM-RNG_LRG";
			break;
		case StagingSystem::ARENA_WRITE_PER_FRAME_CP_SB:
			out << "w-ANY-FRA-CP-SB";
			break;
		case StagingSystem::ARENA_READ_PER_FRAME_PM_RNG:
			out << "r-ANY-FRA-PM-RNG";
			break;
		case StagingSystem::ARENA_READ_RARE_TM_SB:
			out << "r-ANY-RAR-TM-SB";
			break;
		case StagingSystem::ARENA_WRITE_RARE_TM_SB:
			out << "w-ANY-RAR-TM-SB";
			break;
		case StagingSystem::ARENA_WRITE_NEVER_CP_NB:
			out << "w-ANY-NVR-CP-NB";
			break;
		case StagingSystem::ARENA_TYPE_LAST:
			out << "?";
			break;
	}
	return out;
}
