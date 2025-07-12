#include "staging-buffer.h"
#include <regen/gl-types/gl-param.h>

//#define REGEN_STAGING_USE_DIRECT_FLUSHING

using namespace regen;

StagingBuffer::StagingBuffer(
			const ref_ptr<BufferReference> &drawBufferRef,
			const BufferFlags &stagingFlags,
			BufferType bufferType) :
		flags_(stagingFlags),
		refGPU_(drawBufferRef),
		storageMode_(getBufferStorageMode(stagingFlags)),
		storageFlags_(glStorageFlags(storageMode_)),
		accessFlags_(glAccessFlags(storageMode_)),
		bufferType_(bufferType),
		bufferSegments_((int)stagingFlags.bufferingMode),
		stagingReadData_(nullptr) {
	if (stagingFlags.useExplicitStaging()) {
		segmentSize_ = drawBufferRef->allocatedSize();
	} else {
		// note: in case no explicit staging is used, the draw buffer must be allocated
		// upfront with the size of all segments combined.
		segmentSize_ = drawBufferRef->allocatedSize() / bufferSegments_.size();
	}
	bufferCPU_ = ref_ptr<BufferObject>::alloc(stagingFlags.target, stagingFlags.updateHints);
	bufferCPU_->setBufferAccessMode(stagingFlags.accessMode);
	bufferCPU_->setBufferMapMode(stagingFlags.mapMode);
	initializeMapping();
}

StagingBuffer::~StagingBuffer() {
	// free client data
	if (stagingReadData_) {
		delete[] stagingReadData_;
		stagingReadData_ = nullptr;
	}
	refsCPU_.clear();
	bufferCPU_ = {};
}

bool StagingBuffer::initializeMapping() {
	int status = 0;
	// initialize the buffer indices
	if (flags_.bufferingMode != SINGLE_BUFFER) {
		if (storageFlags_ & MAP_WRITE) {
			writeBufferIndex_ = 1;
			readBufferIndex_ = 0;
		} else {
			writeBufferIndex_ = 0;
			readBufferIndex_ = 1;
		}
	}
	// allocate data array for reading
	if (storageFlags_ & MAP_READ) {
		stagingReadData_ = new byte[segmentSize_];
	}

	// obtain CPU accessible buffer
	if (flags_.useExplicitStaging()) {
		// in case of explicit staging, we need obtain separate CPU-accessible storage
		if (bufferType_ == RING_BUFFER) {
			// initialize storage for a ring buffer: 3*segmentSize_ bytes
			refsCPU_ = { bufferCPU_->adoptBufferRange(segmentSize_ * bufferSegments_.size()) };
		} else {
			// allocate a buffer for each segment
			refsCPU_.resize(bufferSegments_.size());
			for (uint32_t i = 0u; i < bufferSegments_.size(); ++i) {
				refsCPU_[i] = bufferCPU_->adoptBufferRange(segmentSize_);
			}
		}
	} else {
		// directly use the GPU buffer as CPU-accessible storage
		refsCPU_ = { refGPU_ };
	}

	// initialize the persistent mapped segments of the staging buffer
	if (bufferType_ == RING_BUFFER) {
		mappedRing_ = refsCPU_[0]->mappedData();
		for (uint32_t i = 0u; i < bufferSegments_.size(); ++i) {
			auto &segment = bufferSegments_[i];
			segment.offset = i * segmentSize_;
			if (mappedRing_) {
				segment.mappedPtr = mappedRing_ + segment.offset;
			}
		}
		if (!mappedRing_ && (storageFlags_ & MAP_PERSISTENT)) {
			status = 1; // error
		}
	} else {
		for (uint32_t i = 0u; i < bufferSegments_.size(); ++i) {
			auto &segment = bufferSegments_[i];
			// if persistent mapping is requested, map the buffer segment
			if (storageFlags_ & MAP_PERSISTENT) {
				segment.mappedPtr = refsCPU_[i]->mappedData();
				if (!segment.mappedPtr) {
					status = 1; // error
					break;
				}
			}
		}
	}

	if(status) {
		REGEN_ERROR("Failed to map buffer " << refGPU_->bufferID() <<
				" size: " << refGPU_->allocatedSize()/1024.0 << " KiB " <<
				" target: " << flags_.target <<
				" map: " << flags_.mapMode <<
				" access: " << flags_.accessMode <<
				" buffering: " << flags_.bufferingMode <<
				" sync: " << flags_.syncFlags);
		GL_ERROR_LOG();
		return false;
	}

	// initialize created buffers to zero
	if (flags_.useExplicitStaging()) {
		// TODO: make this configurable? if user sets data, we should not zero it.
		//       however then ensure that data goes to correct segment in staging buffer!
		bufferCPU_->setBuffersToZero();
	}

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
		RingSegment &segment = bufferSegments_[drawBuffer.segment_];
		segment.writeFence.setFencePoint();
	}
}

void StagingBuffer::pushToFlushQueue(const Vec4ui *dirtySegments, uint32_t numDirtySegments) {
#ifndef REGEN_STAGING_USE_DIRECT_FLUSHING
	if (flags_.mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
		RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
		const uint32_t totalDirtySegments = writeSegment.numDirtySegments + numDirtySegments;
		// ensure the vector has enough space
		if (totalDirtySegments > writeSegment.dirtySegments.size()) {
			writeSegment.dirtySegments.resize(totalDirtySegments);
		}
		// copy the dirty segments into the vector
		// TODO: support merging of dirty segments?
		// TODO: can we limit dirty segment to (offset, size) pairs?
		auto *dataStart = writeSegment.dirtySegments.data() + writeSegment.numDirtySegments;
		std::memcpy(
			(byte*)dataStart,
			(byte*)dirtySegments,
			numDirtySegments * sizeof(Vec4ui));
		// finally increment the number of dirty segments
		writeSegment.numDirtySegments = totalDirtySegments;
	}
#endif
}

void StagingBuffer::setSubData(uint32_t localOffset, uint32_t dataSize, const void *data) {
	RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];

	if (storageFlags_ & MAP_PERSISTENT) {
		// in case of persistent mapping, we can write directly to the mapped pointer
		if (!writeSegment.mappedPtr) {
			REGEN_ERROR("Failed to write to persistent mapped buffer segment.");
			return;
		}
		std::memcpy(writeSegment.mappedPtr + localOffset, data, dataSize);
	} else {
		// in case of non-persistent mapping, we need to copy the data into the buffer without mapping.
		auto &writeBuffer = (bufferType_ == RING_BUFFER ? refsCPU_[0] : refsCPU_[writeBufferIndex_]);
		glNamedBufferSubData(
			writeBuffer->bufferID(),
			writeBuffer->address() + writeSegment.offset + localOffset,
			dataSize, data);
	}
}

void* StagingBuffer::beginMappedWrite(bool isPartialWrite, uint32_t localOffset, uint32_t mappedSize) {
	RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];

	if (storageFlags_ & MAP_PERSISTENT) {
		// if we have a persistent mapping, we need to wait for the fence.
		// the fence marks the point where the segment we want to write to was consumed.
		// in case of explicit staging, this is the point where staging buffer was copied to the GPU buffer.
		if(!writeSegment.writeFence.wait(flags_.useFrameDropping())) {
			return nullptr; // drop frame
		}
		return writeSegment.mappedPtr + localOffset;
	}
	else {
		// this case is for non-persistent mapping, where we need to map the buffer segment
		// for each write operation.
		auto &writeBuffer = (bufferType_ == RING_BUFFER ?
			refsCPU_[0] : refsCPU_[writeBufferIndex_]);

		GLbitfield mappingFlags = accessFlags_;
		if (!isPartialWrite) { mappingFlags |= MAP_INVALIDATE_RANGE; }

		return (byte *) glMapNamedBufferRange(
				writeBuffer->bufferID(),
				writeBuffer->address() + writeSegment.offset + localOffset,
				mappedSize,
				mappingFlags);
	}
}

void StagingBuffer::endMappedWrite(BufferRange &nextDrawBuffer) {
	RingSegment &readSegment = bufferSegments_[readBufferIndex_];
	auto &readBuffer = (bufferType_ == RING_BUFFER ?
		refsCPU_[0] : refsCPU_[readBufferIndex_]);
	auto &writeBuffer = (bufferType_ == RING_BUFFER ?
		refsCPU_[0] : refsCPU_[writeBufferIndex_]);

	if ((storageFlags_ & MAP_PERSISTENT) == 0) {
		// non-persistent mapping
		glUnmapNamedBuffer(writeBuffer->bufferID());
	}
#ifdef REGEN_STAGING_USE_DIRECT_FLUSHING
	else if (accessFlags_ & MAP_FLUSH_EXPLICIT) {
		// direct flushing
		RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
		glFlushMappedNamedBufferRange(writeBuffer->bufferID(),
			writeSegment.offset, segmentSize_);
	}
#endif

	if (flags_.useExplicitStaging()) {
#ifndef REGEN_STAGING_USE_DIRECT_FLUSHING
		// Make sure the last write to current readBuffer is flushed before we copy the data.
		if (accessFlags_ & MAP_FLUSH_EXPLICIT) {
			for (uint32_t flushIdx = 0; flushIdx < readSegment.numDirtySegments; ++flushIdx) {
				// get the segment to flush
				const Vec4ui &flushSegment = readSegment.dirtySegments[flushIdx];
				glFlushMappedNamedBufferRange(
					readBuffer->bufferID(),
					readSegment.offset + flushSegment.x,
					flushSegment.y);
			}
			readSegment.numDirtySegments = 0; // reset the dirty segments
		}
#endif
		// copy data from staging buffer to the GPU buffer
		glCopyNamedBufferSubData(
			readBuffer->bufferID(),
			refGPU_->bufferID(),
			readBuffer->address() + readSegment.offset,
			refGPU_->address(),
			segmentSize_);
		if (storageFlags_ & MAP_PERSISTENT) {
			// Create a fence just after glCopyNamedBufferSubData -- marking the point where the
			// written data of this frame has been consumed by the GPU.
			readSegment.writeFence.setFencePoint();
		}
		nextDrawBuffer.buffer_ = refGPU_->bufferID();
		nextDrawBuffer.offset_ = refGPU_->address();
		nextDrawBuffer.segment_ = 0;
	}
	else {
		// NOTE: in case of shared buffers, the draw calls consuming the buffer need to
		// set the fence point
		nextDrawBuffer.buffer_ = readBuffer->bufferID();
		nextDrawBuffer.offset_ = readBuffer->address() + readSegment.offset;
		nextDrawBuffer.segment_ = readBufferIndex_;
	}
	nextDrawBuffer.size_ = segmentSize_;

	// finally swap buffers in case of double/triple buffering
	swapBuffers();
}

void StagingBuffer::beginNonMappedWrite() {
	// nothing to do here for the moment...
}

void StagingBuffer::endNonMappedWrite(BufferRange &nextDrawBuffer) {
	// in case of single-buffering, we do not need to copy the data,
	// we just set the next draw buffer to the current segment.
	RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
	RingSegment &readSegment = bufferSegments_[readBufferIndex_];
	auto &readBuffer = (bufferType_ == RING_BUFFER ?
		refsCPU_[0] : refsCPU_[readBufferIndex_]);
	auto &writeBuffer = (bufferType_ == RING_BUFFER ?
		refsCPU_[0] : refsCPU_[writeBufferIndex_]);

	if (flags_.useExplicitStaging()) {
		// copy data from staging buffer to the GPU buffer
		glCopyNamedBufferSubData(
			readBuffer->bufferID(),
			refGPU_->bufferID(),
			readBuffer->address() + readSegment.offset,
			refGPU_->address(),
			segmentSize_);
		nextDrawBuffer.buffer_ = refGPU_->bufferID();
		nextDrawBuffer.offset_ = refGPU_->address();
		nextDrawBuffer.segment_ = 0;
	} else {
		nextDrawBuffer.buffer_ = writeBuffer->bufferID();
		nextDrawBuffer.offset_ = writeBuffer->address() + writeSegment.offset;
		nextDrawBuffer.segment_ = readBufferIndex_;
	}
	nextDrawBuffer.size_ = segmentSize_;

	swapBuffers();
}

bool StagingBuffer::readBuffer(BufferRange &nextDrawBuffer) {
	RingSegment &writeSegment = bufferSegments_[writeBufferIndex_];
	RingSegment &readSegment = bufferSegments_[readBufferIndex_];
	auto &readBuffer = (bufferType_ == RING_BUFFER ?
		refsCPU_[0] : refsCPU_[readBufferIndex_]);
	auto &writeBuffer = (bufferType_ == RING_BUFFER ?
		refsCPU_[0] : refsCPU_[writeBufferIndex_]);

	if (flags_.useExplicitStaging()) {
		// copy data from GPU buffer to staging buffer
		glCopyNamedBufferSubData(
				refGPU_->bufferID(),
				writeBuffer->bufferID(),
				refGPU_->address(),
				writeBuffer->address() + writeSegment.offset,
				segmentSize_);
		if (storageFlags_ & MAP_PERSISTENT) {
			// mark the point where copy to staging buffer was done.
			writeSegment.readFence.setFencePoint();
		}
	}
	writeSegment.hasData = true;

	// copy data from read buffer to client data
	if (readSegment.hasData) {
		if (storageFlags_ & MAP_PERSISTENT) {
			if (!readSegment.mappedPtr) {
				return false; // ERROR: "Failed to map buffer for reading"
			}
			// wait until the last glCopyNamedBufferSubData into this segment is done.
			if(!readSegment.readFence.wait(flags_.useFrameDropping())) {
				return true; // drop frame
			}
			// need to invalidate the range before copying if using persistent mapping
			// without coherent bit.
			if (flags_.mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
				glInvalidateBufferSubData(
						readBuffer->bufferID(),
						readBuffer->address() + readSegment.offset,
						segmentSize_);
			}
			std::memcpy(stagingReadData_, readSegment.mappedPtr, segmentSize_);
		}
		else {
			// temporary mapping case:
			// map read buffer, copy data to client data, unmap read buffer
			auto mapped = (byte *) glMapNamedBufferRange(
					readBuffer->bufferID(),
					readBuffer->address() + readSegment.offset,
					segmentSize_,
					GL_MAP_READ_BIT);
			if (mapped) {
				std::memcpy(stagingReadData_, mapped, segmentSize_);
				glUnmapNamedBuffer(readBuffer->bufferID());
			} else {
				return false; // ERROR: "Failed to map buffer temporary for reading"
			}
		}
		hasReadData_ = true;
	}

	if (flags_.useExplicitStaging()) {
		nextDrawBuffer.buffer_ = refGPU_->bufferID();
		nextDrawBuffer.offset_ = refGPU_->address();
		nextDrawBuffer.segment_ = 0;
	} else {
		nextDrawBuffer.buffer_ = writeBuffer->bufferID();
		nextDrawBuffer.offset_ = writeBuffer->address() + writeSegment.offset;
		nextDrawBuffer.segment_ = readBufferIndex_;
	}
	nextDrawBuffer.size_ = segmentSize_;

	// finally swap buffers in case of double/triple buffering
	swapBuffers();
	return true;
}
