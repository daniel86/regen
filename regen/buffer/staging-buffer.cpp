#include "staging-buffer.h"
#include "regen/gl-types/gl-param.h"
#include "staging-system.h"

#define REGEN_USE_STAGING_ALLOCATOR
//#define REGEN_STAGING_USE_DIRECT_FLUSHING

using namespace regen;

float StagingBuffer::MAX_ACCEPTABLE_STALL_RATE = 0.1f; // 10% of frames can stall
// note: camera is currently with 448 bytes slightly below 512 bytes
uint32_t StagingBuffer::MIN_SIZE_MEDIUM = 512; // Bytes
uint32_t StagingBuffer::MIN_SIZE_LARGE = 64 * 1024; // 64 KiB
uint32_t StagingBuffer::MIN_SIZE_VERY_LARGE = 1024 * 1024; // 1 MiB

StagingBuffer::StagingBuffer(const BufferFlags &stagingFlags) :
		flags_(stagingFlags),
		storageMode_(getBufferStorageMode(stagingFlags)),
		storageFlags_(glStorageFlags(storageMode_)),
		accessFlags_(glAccessFlags(storageMode_)),
		stagingReadData_(nullptr) {
	stagingBO_ = ref_ptr<BufferObject>::alloc(stagingFlags.target, stagingFlags.updateHints);
	stagingBO_->setClientAccessMode(stagingFlags.accessMode);
	stagingBO_->setBufferMapMode(stagingFlags.mapMode);
	// initialize to single segment
	bufferSegments_.resize(1);
	bufferSegments_[0].offset = 0;
}

StagingBuffer::~StagingBuffer() {
	// free client data
	if (stagingReadData_) {
		delete[] stagingReadData_;
		stagingReadData_ = nullptr;
	}
	stagingRef_ = {};
	stagingBO_ = {};
}

BufferSizeClass StagingBuffer::getBufferSizeClass(uint32_t size) {
	if (size < StagingBuffer::MIN_SIZE_MEDIUM) {
		return BUFFER_SIZE_SMALL;
	} else if (size < StagingBuffer::MIN_SIZE_LARGE) {
		return BUFFER_SIZE_MEDIUM;
	} else if (size < StagingBuffer::MIN_SIZE_VERY_LARGE) {
		return BUFFER_SIZE_LARGE;
	} else {
		return BUFFER_SIZE_VERY_LARGE;
	}
}

BufferPool* StagingBuffer::getStagingAllocator(BufferStorageMode storageMode) {
	static std::array<BufferPool*,(int)BUFFER_STORAGE_MODE_LAST> bufferPools;
	BufferPool *stagingAllocator = bufferPools[(int)storageMode];
	if (stagingAllocator == nullptr) {
		stagingAllocator = new BufferPool();
		stagingAllocator->set_index((int)storageMode);
		stagingAllocator->set_alignment(StagingSystem::STAGING_BUFFER_ALIGNMENT);
		stagingAllocator->set_minSize(8u * 1024u * 1024u); // 2048 pages = 8 MiB
		bufferPools[(int)storageMode] = stagingAllocator;
	}
	return stagingAllocator;
}

bool StagingBuffer::resizeBuffer(uint32_t segmentSize, uint32_t numRingSegments) {
	uint32_t numSegments = (flags_.bufferingMode == RING_BUFFER ?
								  numRingSegments :
								  (uint32_t) flags_.bufferingMode);
	numSegments = std::max(numSegments, 1u); // ensure at least one segment

	// initialize the buffer indices
	if (numSegments > 1) {
		writeBufferIndex_ = 1;
		readBufferIndex_ = 0;
	}
	if (bufferSegments_.size() != numSegments) {
		bufferSegments_.resize(numSegments);
	}

	if (segmentSize < segmentSize_) {
		// if size is reduced, keep using the adopted range as long as the size reduction is not too large.
		auto halfSegmentSize = segmentSize_ / 2;
		if (segmentSize > halfSegmentSize) {
			// update the offset for each segment
			for (uint32_t segmentIdx = 0u; segmentIdx < numSegments; segmentIdx++) {
				bufferSegments_[segmentIdx].offset = segmentIdx * segmentSize_;
			}
			return true; // no need to resize, just keep the existing segments
		}
	}
	segmentSize_ = segmentSize;

	// allocate data array for reading
	if (storageFlags_ & MAP_READ) {
		delete[] stagingReadData_;
		stagingReadData_ = new byte[segmentSize_];
	}
	// update the offset for each segment
	for (uint32_t segmentIdx = 0u; segmentIdx < numSegments; segmentIdx++) {
		bufferSegments_[segmentIdx].offset = segmentIdx * segmentSize_;
	}

	// obtain CPU accessible buffer
	if (flags_.useExplicitStaging()) {
		// in case of explicit staging, we need obtain separate CPU-accessible storage.
		// initialize storage for multiple buffers: numSegments*segmentSize_ bytes
		if (stagingRef_.get()) {
			BufferObject::orphanBufferRange(stagingRef_.get());
		}
#ifdef REGEN_USE_STAGING_ALLOCATOR
		stagingRef_ = BufferObject::adoptBufferRange(
				segmentSize_ * numSegments,
				getStagingAllocator(storageMode_));
#else
		stagingRef_ = stagingBO_->adoptBufferRange(
				segmentSize_ * numSegments);
#endif
		if (!stagingRef_->mappedData() && (storageFlags_ & MAP_PERSISTENT)) {
			REGEN_ERROR("Failed to map buffer " <<
												" target: " << flags_.target <<
												" map: " << flags_.mapMode <<
												" access: " << flags_.accessMode <<
												" buffering: " << flags_.bufferingMode <<
												" sync: " << flags_.syncFlags);
			GL_ERROR_LOG();
			stagingRef_ = {};
			return false;
		}
		if (clearBufferOnResize_) {
			// clear the buffer to zero
			stagingBO_->setBuffersToZero();
		}
		stagingCopyRange_.srcBufferID = stagingRef_->bufferID();
	} else {
		stagingRef_ = {};
	}
	REGEN_INFO("Resized staging buffer to "
					   << segmentSize_ / 1024.0 << " KiB per segment, "
					   << numSegments << " segments, total: "
					   << (segmentSize_ * numSegments) / 1024.0 << " KiB "
					   << " swap: " << useSwappingOnAccess_
					   << " clear: " << clearBufferOnResize_
					   << " max segments: " << maxRingSegments_);

	return true;
}

void StagingBuffer::swapBuffers() {
	const auto numBuffers = bufferSegments_.size();
	readBufferIndex_ += 1;
	if (readBufferIndex_ >= numBuffers) {
		readBufferIndex_ = 0;
	}
	writeBufferIndex_ += 1;
	if (writeBufferIndex_ >= numBuffers) {
		writeBufferIndex_ = 0;
	}
}

void StagingBuffer::markDrawAccessed(BufferRange &drawBuffer) {
	if (!flags_.useExplicitStaging() && (storageFlags_ & MAP_PERSISTENT)) {
		// When implicit staging is used, the fence point must be set after
		// the segment was consumed by the GPU in a draw call.
		RingSegment &segment = bufferSegments_[drawBuffer.segment_];
		segment.fence.setFencePoint();
	}
}

float StagingBuffer::getStallRate() const {
	if (bufferSegments_.empty()
		|| !isMapModePersistent(flags_.mapMode)
		|| !flags_.useSyncFences()) {
		return 0.0f;
	}
	const RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
	return writeSegment.fence.getStallRate();
}

void StagingBuffer::resetStallRate() {
	for (auto &segment: bufferSegments_) {
		segment.fence.resetStallHistory();
	}
}

void StagingBuffer::pushToFlushQueue(const BufferRange2ui *dirtySegments, uint32_t numDirtySegments) {
#ifndef REGEN_STAGING_USE_DIRECT_FLUSHING
	if (flags_.mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
		RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
		const uint32_t totalDirtySegments = writeSegment.numDirtySegments + numDirtySegments;
		// ensure the vector has enough space
		if (totalDirtySegments > writeSegment.dirtySegments.size()) {
			writeSegment.dirtySegments.resize(totalDirtySegments);
		}
		// copy the dirty segments into the vector
		auto *dataStart = writeSegment.dirtySegments.data() + writeSegment.numDirtySegments;
		std::memcpy(
				(byte *) dataStart,
				(byte *) dirtySegments,
				numDirtySegments * sizeof(BufferRange2ui));
		// finally increment the number of dirty segments
		writeSegment.numDirtySegments = totalDirtySegments;
	}
#endif
}

byte *StagingBuffer::getMappedSegment(
		const ref_ptr<BufferReference> &ref,
		uint32_t segmentOffset) {
	if (ref->mappedData()) {
		// if the buffer is mapped, we can return the mapped pointer
		return ref->mappedData() + segmentOffset;
	} else {
		return nullptr; // no mapped data
	}
}

void StagingBuffer::setSubData(
		const ref_ptr<BufferReference> &drawBufferRef,
		uint32_t localOffset,
		uint32_t dataSize,
		const byte *data) {
	RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
	const ref_ptr<BufferReference> &targetRef = (stagingRef_.get() ? stagingRef_ : drawBufferRef);

	if (storageFlags_ & MAP_PERSISTENT) {
		// in case of persistent mapping, we can write directly to the mapped pointer
		byte *mappedPtr = getMappedSegment(targetRef, writeSegment.offset);
		if (mappedPtr) {
			std::memcpy(mappedPtr + localOffset, data, dataSize);
		} else {
			REGEN_ERROR("Failed to write to persistent mapped buffer segment.");
			GL_ERROR_LOG();
		}
	} else if (flags_.isWritable()) {
		// in case of non-persistent mapping, we need to copy the data into the buffer without mapping.
		glNamedBufferSubData(
				targetRef->bufferID(),
				targetRef->address() + writeSegment.offset + localOffset,
				dataSize, data);
	} else {
		// in case of non-writable buffer, we need to adopt a writable buffer range then do buffer-to-buffer copy.
		auto tempRef = BufferObject::adoptBufferRange(
				dataSize,
				BufferObject::bufferPool(flags_.target, BUFFER_MODE_STATIC_WRITE));
		glNamedBufferSubData(
				tempRef->bufferID(),
				tempRef->address(),
				dataSize,
				data);
		glCopyNamedBufferSubData(
				tempRef->bufferID(),
				targetRef->bufferID(),
				tempRef->address(),
				targetRef->address() + writeSegment.offset + localOffset,
				dataSize);
	}
}

byte *StagingBuffer::beginMappedWrite(
		const ref_ptr<BufferReference> &drawBufferRef,
		bool isPartialWrite,
		uint32_t localOffset,
		uint32_t mappedSize) {
	RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
	const ref_ptr<BufferReference> &targetRef = (stagingRef_.get() ? stagingRef_ : drawBufferRef);

	if (storageFlags_ & MAP_PERSISTENT) {
		// if we have a persistent mapping, we need to wait for the fence.
		// the fence marks the point where the segment we want to write to was consumed.
		// in case of explicit staging, this is the point where staging buffer was copied to the GPU buffer.
		if (flags_.useSyncFences() &&
			!writeSegment.fence.wait(flags_.useFrameDropping())) {
			return nullptr; // drop frame
		}
		byte *mappedPtr = getMappedSegment(targetRef, writeSegment.offset);
		if (mappedPtr) {
			return mappedPtr + localOffset;
		} else {
			REGEN_ERROR("Failed to map persistent buffer segment.");
			GL_ERROR_LOG();
			return nullptr; // failed to map the segment
		}
	} else {
		// this case is for non-persistent mapping, where we need to map the buffer segment
		// for each write operation.
		GLbitfield mappingFlags = accessFlags_;
		if (!isPartialWrite) { mappingFlags |= MAP_INVALIDATE_RANGE; }
		byte *mappedPtr = (byte *) glMapNamedBufferRange(
				targetRef->bufferID(),
				targetRef->address() + writeSegment.offset + localOffset,
				mappedSize,
				mappingFlags);
		if (mappedPtr) {
			return mappedPtr;
		} else {
			REGEN_ERROR("Failed to temporary map buffer segment.");
			GL_ERROR_LOG();
			return nullptr; // failed to map the segment
		}
	}
}

void StagingBuffer::endMappedWrite(
		const ref_ptr<BufferReference> &drawBufferRef,
		BufferRange &nextDrawBufferRange,
		uint32_t localOffset) {
	RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
	RingSegment &readSegment = bufferSegments_[readBufferIndex_];
	writeSegment.hasData = true;

	const bool hasStagingRef = stagingRef_.get() != nullptr;
	const ref_ptr<BufferReference> &targetRef = (hasStagingRef ? stagingRef_ : drawBufferRef);

	if ((storageFlags_ & MAP_PERSISTENT) == 0) {
		// non-persistent mapping
		glUnmapNamedBuffer(targetRef->bufferID());
	}
#ifdef REGEN_STAGING_USE_DIRECT_FLUSHING
	else if (accessFlags_ & MAP_FLUSH_EXPLICIT) {
		// direct flushing
		RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
		glFlushMappedNamedBufferRange(targetRef->bufferID(),
			writeSegment.offset + localOffset,
			drawBufferRef->allocatedSize());
	}
#endif

	if (flags_.useExplicitStaging()) {
#ifndef REGEN_STAGING_USE_DIRECT_FLUSHING
		// Make sure the last write to current readBuffer is flushed before we copy the data.
		if (accessFlags_ & MAP_FLUSH_EXPLICIT) {
			for (uint32_t flushIdx = 0; flushIdx < readSegment.numDirtySegments; ++flushIdx) {
				// get the segment to flush
				const BufferRange2ui &flushSegment = readSegment.dirtySegments[flushIdx];
				glFlushMappedNamedBufferRange(
						targetRef->bufferID(),
						readSegment.offset + localOffset + flushSegment.offset,
						flushSegment.size);
			}
			readSegment.numDirtySegments = 0; // reset the dirty segments
		}
#endif
		// NOTE: We delay copy to draw buffer until we reach a read segment that has been written to.
		//   This causes some frames of delay until the upload starts. The draw buffer best has some
		//   meaningful initial value that can be drawn first few frames!
		if (readSegment.hasData) {
			// copy data from staging buffer to the draw buffer
			if (hasStagingRef) {
				// Let the staging system handle the copy, it may be able
				// to optimize the copy operation, e.g. by coalescing multiple copies
				// into a single copy operation.
				auto &staging = StagingSystem::instance();
				stagingCopyRange_.dstBufferID = drawBufferRef->bufferID();
				stagingCopyRange_.srcOffset = targetRef->address() + readSegment.offset + localOffset;
				stagingCopyRange_.dstOffset = drawBufferRef->address();
				stagingCopyRange_.size = drawBufferRef->allocatedSize();
				staging.scheduledCopy(stagingCopyRange_);
			} else {
				glCopyNamedBufferSubData(
						targetRef->bufferID(),
						drawBufferRef->bufferID(),
						targetRef->address() + readSegment.offset + localOffset,
						drawBufferRef->address(),
						drawBufferRef->allocatedSize());
				if (storageFlags_ & MAP_PERSISTENT && flags_.useSyncFences()) {
					// Create a fence just after glCopyNamedBufferSubData -- marking the point where the
					// written data of this frame has been consumed by the GPU.
					readSegment.fence.setFencePoint();
				}
			}
		}
		nextDrawBufferRange.offset_ = drawBufferRef->address();
		nextDrawBufferRange.segment_ = 0;
	} else {
		// NOTE: in case of shared buffers, the draw calls consuming the buffer need to
		// set the fence point
		nextDrawBufferRange.offset_ = drawBufferRef->address() + readSegment.offset + localOffset;
		nextDrawBufferRange.segment_ = readBufferIndex_;
	}

	// finally swap buffers in case of double/triple buffering
	if (useSwappingOnAccess_) swapBuffers();
}

void StagingBuffer::beginNonMappedWrite() {
	// nothing to do here for the moment...
}

void StagingBuffer::endNonMappedWrite(
		const ref_ptr<BufferReference> &drawBufferRef,
		BufferRange &nextDrawBufferRange,
		uint32_t localOffset) {
	// in case of single-buffering, we do not need to copy the data,
	// we just set the next draw buffer to the current segment.
	RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
	RingSegment &readSegment = bufferSegments_[readBufferIndex_];
	writeSegment.hasData = true;

	if (flags_.useExplicitStaging()) {
		// copy data from staging buffer to the GPU buffer
		if (readSegment.hasData) {
			// Let the staging system handle the copy.
			auto &staging = StagingSystem::instance();
			stagingCopyRange_.dstBufferID = drawBufferRef->bufferID();
			stagingCopyRange_.srcOffset = stagingRef_->address() + readSegment.offset + localOffset;
			stagingCopyRange_.dstOffset = drawBufferRef->address();
			stagingCopyRange_.size = drawBufferRef->allocatedSize();
			staging.scheduledCopy(stagingCopyRange_);
		}
		nextDrawBufferRange.offset_ = drawBufferRef->address();
		nextDrawBufferRange.segment_ = 0;
	} else {
		nextDrawBufferRange.offset_ = drawBufferRef->address() + writeSegment.offset + localOffset;
		nextDrawBufferRange.segment_ = readBufferIndex_;
	}

	if (useSwappingOnAccess_) swapBuffers();
}

bool StagingBuffer::readBuffer(
		const ref_ptr<BufferReference> &drawBufferRef,
		BufferRange &nextDrawBufferRange,
		uint32_t localOffset) {
	RingSegment &nextCopySegment = bufferSegments_[writeBufferIndex_];
	RingSegment &nextDrawSegment = bufferSegments_[readBufferIndex_];
	const ref_ptr<BufferReference> &stagingRef = (stagingRef_.get() ? stagingRef_ : drawBufferRef);

	if (flags_.useExplicitStaging()) {
		// copy data from GPU buffer to staging buffer
		glCopyNamedBufferSubData(
				drawBufferRef->bufferID(),
				stagingRef->bufferID(),
				drawBufferRef->address(),
				stagingRef->address() + nextDrawSegment.offset + localOffset,
				drawBufferRef->allocatedSize());
		if (storageFlags_ & MAP_PERSISTENT && flags_.useSyncFences()) {
			// mark the point where copy to staging buffer was done.
			nextDrawSegment.fence.setFencePoint();
		}
	}
	nextDrawSegment.hasData = true;

	// copy data from staging buffer to CPU memory
	if (nextCopySegment.hasData) {
		if (storageFlags_ & MAP_PERSISTENT) {
			byte *mappedPtr = getMappedSegment(stagingRef, nextCopySegment.offset);
			if (!mappedPtr) {
				return false; // ERROR: "Failed to map buffer for reading"
			}
			// wait until the last glCopyNamedBufferSubData into this segment is done.
			if (flags_.useSyncFences() && !nextCopySegment.fence.wait(flags_.useFrameDropping())) {
				return true; // drop frame
			}
			// need to invalidate the range before copying if using persistent mapping
			// without coherent bit.
			if (flags_.mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
				glInvalidateBufferSubData(
						stagingRef->bufferID(),
						stagingRef->address() + nextCopySegment.offset + localOffset,
						drawBufferRef->allocatedSize());
			}
			std::memcpy(
					stagingReadData_ + localOffset,
					mappedPtr + localOffset,
					drawBufferRef->allocatedSize());
		} else {
			// temporary mapping case:
			// map read buffer, copy data to client data, unmap read buffer
			auto mapped = (byte *) glMapNamedBufferRange(
					stagingRef->bufferID(),
					stagingRef->address() + nextCopySegment.offset + localOffset,
					drawBufferRef->allocatedSize(),
					GL_MAP_READ_BIT);
			if (mapped) {
				std::memcpy(
						stagingReadData_ + localOffset,
						mapped,
						drawBufferRef->allocatedSize());
				glUnmapNamedBuffer(stagingRef->bufferID());
			} else {
				REGEN_ERROR("Failed to map buffer for reading.");
				GL_ERROR_LOG();
				return false;
			}
		}
	}

	if (flags_.useExplicitStaging()) {
		nextDrawBufferRange.offset_ = drawBufferRef->address();
		nextDrawBufferRange.segment_ = 0;
	} else {
		nextDrawBufferRange.offset_ = drawBufferRef->address() + nextCopySegment.offset + localOffset;
		nextDrawBufferRange.segment_ = readBufferIndex_;
	}

	// finally swap buffers in case of double/triple buffering
	if (useSwappingOnAccess_) swapBuffers();
	return true;
}
