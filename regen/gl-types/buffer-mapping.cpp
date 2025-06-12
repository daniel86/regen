#include "buffer-mapping.h"
#include <regen/gl-types/gl-param.h>

using namespace regen;

int BufferMapping::minMapAlignment_ = 64; // default value, can be overridden

BufferMapping::BufferMapping(GLbitfield storageFlags, BufferingMode storageBuffering, BufferType bufferType) :
		GLObject(glGenBuffers, glDeleteBuffers,
		bufferType==MULTI_BUFFER ? (int)storageBuffering : 1),
		storageFlags_(storageFlags),
		bufferType_(bufferType),
		storageBuffering_(storageBuffering),
		bufferSegments_((int)storageBuffering),
		storageClientData_(nullptr) {
	// validate storage flags
	if ((storageFlags_ & MAP_READ) && (storageFlags_ & MAP_WRITE)) {
		REGEN_WARN("MAP_READ and MAP_WRITE are set at the same time, this is not allowed.");
		storageFlags_ &= ~MAP_READ; // remove read flag
	}
	if ((storageFlags_ & MAP_FLUSH_EXPLICIT) && !(storageFlags_ & MAP_WRITE)) {
		REGEN_WARN("MAP_FLUSH_EXPLICIT can only be used with MAP_WRITE.");
		storageFlags_ &= ~MAP_FLUSH_EXPLICIT; // remove invalidate buffer flag
	}
	if ((storageFlags_ & MAP_COHERENT) && !(storageFlags_ & MAP_PERSISTENT)) {
		REGEN_WARN("MAP_COHERENT is set without MAP_PERSISTENT, this is not allowed.");
		storageFlags_ &= ~MAP_COHERENT; // remove coherent flag
	}
	if ((storageFlags_ & MAP_INVALIDATE_RANGE) && (storageFlags_ & MAP_PERSISTENT)) {
		REGEN_WARN("MAP_INVALIDATE_RANGE is set with MAP_PERSISTENT, this is not allowed.");
		storageFlags_ &= ~MAP_INVALIDATE_RANGE; // remove invalidate range flag
	}
	if ((storageFlags_ & MAP_INVALIDATE_BUFFER) && (storageFlags_ & MAP_PERSISTENT)) {
		REGEN_WARN("MAP_INVALIDATE_BUFFER is set with MAP_PERSISTENT, this is not allowed.");
		storageFlags_ &= ~MAP_INVALIDATE_BUFFER; // remove invalidate buffer flag
	}


	if (storageBuffering != SINGLE_BUFFER) {
		// write into the buffer that was read last frame,
		// but delay the read buffer by number of buffers in use.
		if (storageFlags_ & MAP_WRITE) {
			// use same buffer for mapped writing and copy to draw buffer such that the buffer
			// is not used for reading while writing for storageBuffering-1 frames.
			writeBufferIndex_ = 0;
			readBufferIndex_ = 0;
		} else {
			writeBufferIndex_ = 0;
			readBufferIndex_ = 1;
		}
	}
}

BufferMapping::~BufferMapping() {
	auto *rs = RenderState::get();
	// free client data
	if (storageClientData_) {
		delete[] storageClientData_;
		storageClientData_ = nullptr;
	}
	for (uint32_t i = 0u; i < bufferSegments_.size(); ++i) {
		auto &segment = bufferSegments_[i];
		if (segment.writeFence) {
			glDeleteSync(segment.writeFence);
			segment.writeFence = nullptr;
		}
		if (segment.mappedPtr) {
			if (!mappedRing_) {
				rs->buffer(glTarget_).push(ids_[i]);
				glUnmapBuffer(glTarget_);
				rs->buffer(glTarget_).pop();
			}
			segment.mappedPtr = nullptr; // clear pointer
		}
	}
	// cleanup the mapped buffer
	if (mappedRing_) {
		rs->buffer(glTarget_).push(ids_[0]);
		glUnmapBuffer(glTarget_);
		rs->buffer(glTarget_).pop();
		mappedRing_ = nullptr;
	}
}

static inline void resetBuffer(RenderState *rs, GLenum target, uint32_t &id) {
	rs->buffer(target).push(id);
	glUnmapBuffer(target);
	rs->buffer(target).pop();
	// NOTE: this seems to be necessary on some drivers, e.g. AMD
	glDeleteBuffers(1, &id);
	glGenBuffers(1, &id);
}

static inline bool waitForFence(GLsync &fence, bool allowFrameDropping) {
	GLenum status = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
	if (allowFrameDropping) {
		// if we can drop frames, then we never want to wait and drop the frame instead!
		if (status == GL_TIMEOUT_EXPIRED) {
			return false; // drop frame
		}
	}
	else {
		// Non-blocking check failed, wait a bit
		do {
			status = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000); // 1µs timeout
		} while (status == GL_TIMEOUT_EXPIRED);
	}
	if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
		glDeleteSync(fence);
		fence = nullptr;
	}
	else  {
		REGEN_WARN("Unknown fence status: " << status <<
				" (0x" << std::hex << status << std::dec << ")");
		GL_ERROR_LOG();
	}
	return true;
}

static inline void createFence(GLsync &fence) {
	if (fence) {
		glDeleteSync(fence);
	}
	fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	if (!fence) {
		REGEN_ERROR("Failed to create fence sync object.");
	}
}

bool BufferMapping::initializeMapping(GLuint numBytes, GLenum bufferTarget) {
	static const int minMapAlignment = glParam<int>(GL_MIN_MAP_BUFFER_ALIGNMENT);
	auto *rs = RenderState::get();

	unalignedSegmentSize_ = numBytes;
	// make each segment size a multiple of BUFFER_MAPPING_ALIGNMENT
	segmentSize_ = alignUp(numBytes, std::max(minMapAlignment, BufferMapping::minMapAlignment_));
	storageSize_ = segmentSize_ * bufferSegments_.size();

	if (storageFlags_ & MAP_READ) {
		// allocate client data for reading
		delete[] storageClientData_;
		storageClientData_ = new byte[segmentSize_];
	}

	// cleanup previous buffers
	if (storageFlags_ & MAP_PERSISTENT) {
		if (mappedRing_) {
			resetBuffer(rs, glTarget_, ids_[0]);
			mappedRing_ = nullptr;
			for (auto &segment : bufferSegments_) {
				segment.mappedPtr = nullptr; // clear pointer
			}
		} else {
			for (uint32_t i = 0u; i < bufferSegments_.size(); ++i) {
				auto &segment = bufferSegments_[i];
				if (segment.mappedPtr) {
					resetBuffer(rs, glTarget_, ids_[i]);
					segment.mappedPtr = nullptr; // clear pointer
				}
			}
		}
	}
	glTarget_ = bufferTarget;

	// cleanup buffer segments
	for (auto &segment : bufferSegments_) {
		segment.offset = 0;
		segment.mappedPtr = nullptr;
		segment.hasData = false;
		if (segment.writeFence) {
			glDeleteSync(segment.writeFence);
			segment.writeFence = nullptr;
		}
	}

	int status = 0;

	if (bufferType_ == RING_BUFFER) {
		rs->buffer(bufferTarget).push(ids_[0]);
		// initialize storage for the ring buffer: 3*segmentSize_ bytes
		glBufferStorage(bufferTarget, storageSize_, nullptr, storageFlags_);
		// if persistent mapping is requested, map the ring buffer
		if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
			mappedRing_ = (byte *) glMapBufferRange(
					bufferTarget,
					0,
					storageSize_,
					storageFlags_);
			if (mappedRing_) {
				for (uint32_t i = 0u; i < bufferSegments_.size(); ++i) {
					auto &segment = bufferSegments_[i];
					segment.offset = i * segmentSize_;
					segment.mappedPtr = mappedRing_ + segment.offset;
				}
			} else {
				status = 1; // error
			}
		}
		rs->buffer(bufferTarget).pop();
	} else {
		for (uint32_t i = 0u; i < bufferSegments_.size(); ++i) {
			auto &segment = bufferSegments_[i];
			rs->buffer(bufferTarget).push(ids_[i]);
			// initialize storage for the segment: segmentSize_ bytes
			glBufferStorage(bufferTarget, segmentSize_, nullptr, storageFlags_);
			// if persistent mapping is requested, map the buffer segment
			if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
				segment.mappedPtr = (byte *) glMapBufferRange(
						bufferTarget,
						0,
						segmentSize_,
						storageFlags_);
				if (!segment.mappedPtr) {
					status = 1; // error
					break;
				}
			}
			rs->buffer(bufferTarget).pop();
		}
	}

	if(status) {
		REGEN_ERROR("Failed to map buffer " << ids_[0] <<
				" target: 0x" << std::hex << bufferTarget << std::dec <<
				" size: " << storageSize_/1024.0 << " KiB " <<
				" coherent: " << ((storageFlags_ & GL_MAP_COHERENT_BIT)!= 0) <<
				" write: " << ((storageFlags_ & GL_MAP_WRITE_BIT) != 0) <<
				" read: " << ((storageFlags_ & GL_MAP_READ_BIT) != 0) <<
				" coherent: " << ((storageFlags_ & GL_MAP_COHERENT_BIT) != 0) <<
				" flush: " << ((storageFlags_ & GL_MAP_FLUSH_EXPLICIT_BIT) != 0) <<
				" unsynchronized: " << ((storageFlags_ & GL_MAP_UNSYNCHRONIZED_BIT) != 0) <<
				" persistent: " << ((storageFlags_ & GL_MAP_PERSISTENT_BIT) != 0));
		GL_ERROR_LOG();
		return false;
	}

	// initialize mapped buffers to zero
	if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
		if (mappedRing_) {
			memset(mappedRing_, 0, storageSize_);
		} else {
			for (auto &segment : bufferSegments_) {
				if (segment.mappedPtr) {
					memset(segment.mappedPtr, 0, segmentSize_);
				}
			}
		}
	}

	return true;
}

void* BufferMapping::beginWriteBuffer(bool isPartialWrite) {
#ifdef REGEN_DEBUG_BUILD
	if (!(storageFlags_ & GL_MAP_WRITE_BIT)) {
		REGEN_WARN("beginWriteBuffer called without GL_MAP_WRITE_BIT set.");
		return nullptr;
	}
#endif
	auto &writeSegment = bufferSegments_[writeBufferIndex_];

	if (storageFlags_ & MAP_PERSISTENT) {
		if (writeSegment.writeFence) {
			// this should block until the last write from this segment into draw buffer is finished,
			// i.e. glCopyBufferSubData below. This is necessary to ensure that we do not write into the
			// segment that is currently still being copied into the draw buffer.
			if(!waitForFence(writeSegment.writeFence, allowFrameDropping_)) {
				// this indicates that we should drop the frame, i.e. fence still active and we
				// are allowed to drop frames.
				return nullptr;
			}
		}
		return writeSegment.mappedPtr;
	}
	else {
		auto *rs = RenderState::get();
		GLbitfield mappingFlags = storageFlags_;
		if (bufferType_ == MULTI_BUFFER) {
			if (!isPartialWrite) {
				// if not using ring buffer, we can invalidate the whole buffer
				mappingFlags |= MAP_INVALIDATE_BUFFER;
			}
		} else {
			if (!isPartialWrite) {
				// if using ring buffer, we can only invalidate the segment region
				mappingFlags |= MAP_INVALIDATE_RANGE;
			}
		}

		auto &writeBuffer = (bufferType_ == RING_BUFFER ? ids_[0] : ids_[writeBufferIndex_]);
		rs->buffer(glTarget_).push(writeBuffer);
		auto mapped = (byte *) glMapBufferRange(
				glTarget_,
				writeSegment.offset,
				segmentSize_,
				mappingFlags);
		if (mapped) {
			return mapped;
		} else {
			rs->buffer(glTarget_).pop();
			return nullptr;
		}
	}
}

void BufferMapping::endWriteBuffer(const ref_ptr<BufferReference> &outputBuffer, GLenum outputTarget) {
	auto *rs = RenderState::get();
	auto &readSegment = bufferSegments_[readBufferIndex_];
	uint32_t readBuffer = (bufferType_ == RING_BUFFER ? ids_[0] : ids_[readBufferIndex_]);

	if (!(storageFlags_ & MAP_PERSISTENT)) {
		if (!glUnmapBuffer(glTarget_)) {
			REGEN_WARN("failed to unmap buffer!");
		}
		rs->buffer(glTarget_).pop();
	}

	// TODO: Below copy is not necessary! would be good to avoid it...
	//       However, this might make synchronization more difficult, as the buffer can be used in several
	//       places in the scene graph, and the fence would need to be placed after the last use of the buffer
	//       in a scene graph traversal.
	rs->buffer(outputTarget).apply(outputBuffer->bufferID());
	rs->copyReadBuffer().push(readBuffer);
	if (storageFlags_ & MAP_FLUSH_EXPLICIT) {
		glFlushMappedBufferRange(GL_COPY_READ_BUFFER, readSegment.offset, segmentSize_);
	}
	auto copySize = std::min(outputBuffer->allocatedSize(), unalignedSegmentSize_);
	glCopyBufferSubData(
			GL_COPY_READ_BUFFER,
			outputTarget,
			readSegment.offset,
			outputBuffer->address(),
			copySize);
	rs->copyReadBuffer().pop();
	if (storageFlags_ & MAP_PERSISTENT) {
		// Create a fence ust after glCopyBufferSubData such that we can wait for the GPU to finish
		// before writing into mapped range again.
		createFence(readSegment.writeFence);
	}

	// swap buffers
	readBufferIndex_ = (readBufferIndex_ + 1) % (int)storageBuffering_;
	writeBufferIndex_ = (writeBufferIndex_ + 1) % (int)storageBuffering_;
}

void BufferMapping::readBuffer(const ref_ptr<BufferReference> &inputReference, GLenum inputTarget) {
#ifdef REGEN_DEBUG_BUILD
	if (!(storageFlags_ & MAP_READ)) {
		REGEN_WARN("readBufferData called without GL_MAP_READ_BIT set.");
	}
#endif
	auto *rs = RenderState::get();
	auto &writeSegment = bufferSegments_[writeBufferIndex_];
	auto &readSegment = bufferSegments_[readBufferIndex_];
	uint32_t writeBuffer = (bufferType_ == RING_BUFFER ? ids_[0] : ids_[writeBufferIndex_]);
	uint32_t readBuffer = (bufferType_ == RING_BUFFER ? ids_[0] : ids_[readBufferIndex_]);

	// TODO: need to add fence here too?

	// copy data from inputReference to write buffer
	rs->buffer(inputTarget).apply(inputReference->bufferID());
	rs->copyWriteBuffer().push(writeBuffer);
	glCopyBufferSubData(
			inputTarget,
			GL_COPY_WRITE_BUFFER,
			inputReference->address(),
			writeSegment.offset,
			unalignedSegmentSize_);
	writeSegment.hasData = true;
	rs->copyWriteBuffer().pop();

	// copy data from read buffer to client data
	if (readSegment.hasData) {
		if (storageFlags_ & MAP_PERSISTENT) {
			std::memcpy(storageClientData_, readSegment.mappedPtr, segmentSize_);
		} else {
			// map read buffer, copy data to client data, unmap read buffer
			rs->copyReadBuffer().push(readBuffer);
			auto mapped = (byte *) glMapBufferRange(
					GL_COPY_READ_BUFFER,
					readSegment.offset,
					segmentSize_,
					GL_MAP_READ_BIT);
			if (mapped) {
				std::memcpy(storageClientData_, mapped, segmentSize_);
				glUnmapBuffer(GL_COPY_READ_BUFFER);
			}
			rs->copyReadBuffer().pop();
		}
		hasReadData_ = true;
	}

	// swap buffers
	readBufferIndex_ = (readBufferIndex_ + 1) % (int)storageBuffering_;
	writeBufferIndex_ = (writeBufferIndex_ + 1) % (int)storageBuffering_;
}
