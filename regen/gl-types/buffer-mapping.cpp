#include "buffer-mapping.h"
#include <regen/gl-types/gl-param.h>

using namespace regen;

BufferMapping::BufferMapping(
			const ref_ptr<BufferReference> &ref,
			GLbitfield storageFlags,
			BufferingMode storageBuffering) :
		refs_({ref}),
		storageFlags_(storageFlags),
		bufferType_(RING_BUFFER),
		storageBuffering_(storageBuffering),
		bufferSegments_((int)storageBuffering),
		storageClientData_(nullptr) {
	segmentSize_ = ref->allocatedSize() / (int) storageBuffering;
	initializeMapping();
}

BufferMapping::BufferMapping(
			const std::vector<ref_ptr<BufferReference>> &refs,
			GLbitfield storageFlags) :
		refs_(refs),
		storageFlags_(storageFlags),
		bufferType_(MULTI_BUFFER),
		storageBuffering_((BufferingMode)refs.size()),
		bufferSegments_(refs.size()),
		storageClientData_(nullptr) {
	segmentSize_ = refs[0]->allocatedSize();
	initializeMapping();
}

BufferMapping::~BufferMapping() {
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
				glUnmapNamedBuffer(refs_[i]->bufferID());
			}
			segment.mappedPtr = nullptr; // clear pointer
		}
	}
	// cleanup the mapped buffer
	if (mappedRing_) {
		glUnmapNamedBuffer(refs_[0]->bufferID());
		mappedRing_ = nullptr;
	}
}

bool BufferMapping::initializeMapping() {
	if (storageBuffering_ != SINGLE_BUFFER) {
		// write into the buffer that was read last frame,
		// but delay the read buffer by number of buffers in use.
		if (storageFlags_ & MAP_WRITE) {
			writeBufferIndex_ = 1;
			readBufferIndex_ = 0;
			//writeBufferIndex_ = 0;
			//readBufferIndex_ = 0;
		} else {
			writeBufferIndex_ = 0;
			readBufferIndex_ = 1;
		}
	}

	if (storageFlags_ & MAP_READ) {
		// allocate client data for reading
		storageClientData_ = new byte[segmentSize_];
	}

	int status = 0;

	if (bufferType_ == RING_BUFFER) {
		// if persistent mapping is requested, map the ring buffer
		if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
			mappedRing_ = refs_[0]->mappedData();
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
	} else {
		for (uint32_t i = 0u; i < bufferSegments_.size(); ++i) {
			auto &segment = bufferSegments_[i];
			// if persistent mapping is requested, map the buffer segment
			if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
				segment.mappedPtr = refs_[i]->mappedData();
				if (!segment.mappedPtr) {
					status = 1; // error
					break;
				}
			}
		}
	}

	if(status) {
		REGEN_ERROR("Failed to map buffer " << refs_[0]->bufferID() <<
				" size: " << refs_[0]->allocatedSize()/1024.0 << " KiB " <<
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
	if ((storageFlags_ & GL_MAP_PERSISTENT_BIT) && (storageFlags_ & GL_MAP_WRITE_BIT)) {
		if (mappedRing_) {
			memset(mappedRing_, 0, refs_[0]->allocatedSize());
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

static inline void insertWriteFence(BufferMapping::RingSegment &segment) {
	if (!segment.writeFence) {
		segment.writeFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	}
}

static void ensureWriteFence_cb(void *ptr) {
	// end-of-frame callback to ensure that the last write fence is created
	insertWriteFence(*static_cast<BufferMapping::RingSegment *>(ptr));
}

void* BufferMapping::beginWriteBuffer(bool isPartialWrite) {
	auto &writeSegment = bufferSegments_[writeBufferIndex_];

	if (storageFlags_ & MAP_PERSISTENT) {
		// ensure that the last write segment is synchronized, i.e. inserts a fence before the
		// next segment is written to.
		// usually the sync happens at the end of the frame, but in the case below there apparently
		// are multiple writes per frame.
		if (lastReadIndex_ >= 0) {
			insertWriteFence(bufferSegments_[lastReadIndex_]);
		}

		// block until the last write has been consumed by the GPU.,
		// i.e. all draw commands have been queued that use the buffer.
		if (writeSegment.writeFence) {
			if(!waitForFence(writeSegment.writeFence, allowFrameDropping_)) {
				// this indicates that we should drop the frame, i.e. fence still active, and we
				// are allowed to drop frames.
				return nullptr;
			}
		}
		return writeSegment.mappedPtr;
	}
	else {
		// TODO: Consider using GL_MAP_UNSYNCHRONIZED_BIT with manual sync over GL_MAP_INVALIDATE_RANGE_BIT.
		GLbitfield mappingFlags = storageFlags_;
		if (!isPartialWrite) { mappingFlags |= MAP_INVALIDATE_RANGE; }
		const auto writeBuffer = (bufferType_ == RING_BUFFER ?
				refs_[0]->bufferID() :
				refs_[writeBufferIndex_]->bufferID());
		return (byte *) glMapNamedBufferRange(
				writeBuffer,
				refs_[writeBufferIndex_]->address() + writeSegment.offset,
				segmentSize_,
				mappingFlags);
	}
}

void BufferMapping::endWriteBuffer(BufferRange &nextDrawBuffer) {
	const auto writeBuffer = (bufferType_ == RING_BUFFER ?
			refs_[0]->bufferID() :
			refs_[writeBufferIndex_]->bufferID());
	auto &readSegment = bufferSegments_[readBufferIndex_];

	if (storageFlags_ & MAP_PERSISTENT) {
		auto &writeSegment = bufferSegments_[writeBufferIndex_];
		if (storageFlags_ & MAP_FLUSH_EXPLICIT) {
			glFlushMappedNamedBufferRange(
				writeBuffer,
				writeSegment.offset,
				segmentSize_);
		}
		// insert a fence after draw commands to ensure that the GPU has finished before we write
		// into the range that we will read this frame.
		RenderState::get()->pushPostRenderCallback(
				ensureWriteFence_cb, &readSegment);
		lastReadIndex_ = readBufferIndex_;
	}
	else { // non-persistent mapping
		glUnmapNamedBuffer(writeBuffer);
	}

	if (bufferType_ == RING_BUFFER) {
		nextDrawBuffer.buffer_ = refs_[0]->bufferID();
		nextDrawBuffer.offset_ = refs_[0]->address() + readSegment.offset;
	} else {
		nextDrawBuffer.buffer_ = refs_[readBufferIndex_]->bufferID();
		nextDrawBuffer.offset_ = refs_[readBufferIndex_]->address();
	}

	// swap buffers
	const auto numBuffers = (int)storageBuffering_;
	readBufferIndex_ += 1;
	if (readBufferIndex_ >= numBuffers) {
		readBufferIndex_ = 0;
	}
	writeBufferIndex_ += 1;
	if (writeBufferIndex_ >= numBuffers) {
		writeBufferIndex_ = 0;
	}
}

void BufferMapping::readBuffer(BufferRange &nextDrawBuffer) {
	auto &writeSegment = bufferSegments_[writeBufferIndex_];
	auto &readSegment = bufferSegments_[readBufferIndex_];
	uint32_t readBuffer = (bufferType_ == RING_BUFFER ? refs_[0]->bufferID() : refs_[readBufferIndex_]->bufferID());

	writeSegment.hasData = true;
	// copy data from read buffer to client data
	if (readSegment.hasData) {
		if (storageFlags_ & MAP_PERSISTENT) {
			std::memcpy(storageClientData_, readSegment.mappedPtr, segmentSize_);
		} else {
			// map read buffer, copy data to client data, unmap read buffer
			auto mapped = (byte *) glMapNamedBufferRange(
					readBuffer,
					refs_[readBufferIndex_]->address() + readSegment.offset,
					segmentSize_,
					GL_MAP_READ_BIT);
			if (mapped) {
				std::memcpy(storageClientData_, mapped, segmentSize_);
				glUnmapNamedBuffer(readBuffer);
			}
		}
		hasReadData_ = true;
	}

	nextDrawBuffer.buffer_ = (bufferType_ == RING_BUFFER ?
			refs_[0]->bufferID() :
			refs_[writeBufferIndex_]->bufferID());
	nextDrawBuffer.offset_ = (bufferType_ == RING_BUFFER ?
			refs_[0]->address() + writeSegment.offset :
			refs_[writeBufferIndex_]->address());

	// swap buffers
	readBufferIndex_ = (readBufferIndex_ + 1) % (int)storageBuffering_;
	writeBufferIndex_ = (writeBufferIndex_ + 1) % (int)storageBuffering_;
}
