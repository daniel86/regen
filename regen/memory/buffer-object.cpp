#include "buffer-object.h"

#define USE_SHARED_TBO_BUFFER
#define USE_SHARED_UBO_BUFFER
#define USE_SHARED_SSBO_BUFFER

using namespace regen;

BufferObject::BufferObject(BufferTarget target, const BufferUpdateFlags &hints) :
		Resource(),
		flags_(target, hints),
		glTarget_(glBufferTarget(target)),
		allocatedSize_(0) {
}

BufferObject::BufferObject(const BufferObject &other) :
		Resource(),
		flags_(other.flags_),
		glTarget_(glBufferTarget(flags_.target)),
		allocations_(other.allocations_),
		allocatedSize_(other.allocatedSize_) {
}

BufferObject::~BufferObject() {
	while (!allocations_.empty()) {
		orphanBufferRange(allocations_.back().get());
	}
}

void BufferObject::setClientAccessMode(ClientAccessMode mode) {
	if (mode == BUFFER_GPU_ONLY) {
		return; // no need to set anything
	}
	flags_.accessMode = mode;
}

BufferPool **BufferObject::bufferPools() {
	static std::array<BufferPool *, (int) BufferTarget::TARGET_LAST * (int) BUFFER_STORAGE_MODE_LAST> bufferPools;
	return bufferPools.data();
}

BufferPool *BufferObject::bufferPool(BufferTarget target, BufferStorageMode mode) {
	auto *x = bufferPools();
	auto poolIndex = (int) target * BUFFER_STORAGE_MODE_LAST + (int) mode;
	return x[poolIndex];
}

void BufferObject::createMemoryPools() {
	auto *pools = bufferPools();
	for (int i = 0; i < (int) BufferTarget::TARGET_LAST * (int) BUFFER_STORAGE_MODE_LAST; ++i) {
		if (pools[i] == nullptr) {
			pools[i] = new BufferPool();
			pools[i]->set_index(i);
		}
	}
	// some buffer semantics need special attention as they require
	// alignment to be set, i.e. when using shared buffers consecutive
	// allocations need to be aligned to the size of the buffer.
	for (int i = 0;  i < BUFFER_STORAGE_MODE_LAST; ++i) {
		int poolIndex;

		poolIndex = (int) ARRAY_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
		pools[poolIndex]->set_alignment(PACKED_BASE_ALIGNMENT);

		// Element array buffers need to be aligned to 4 bytes such that binding offsets
		// are multiple of 4 bytes (which is needed for u_32 data)
		poolIndex = (int) ELEMENT_ARRAY_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
		pools[poolIndex]->set_alignment(4);


		poolIndex = (int) TEXTURE_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
#ifdef USE_SHARED_TBO_BUFFER
		pools[poolIndex]->set_alignment(glGetInteger(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// Meaning: Max number of texels, not bytes!
		pools[poolIndex]->set_maxSize(glGetInteger(GL_MAX_TEXTURE_BUFFER_SIZE) * 16);
		poolIndex = (int) UNIFORM_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
#ifdef USE_SHARED_UBO_BUFFER
		pools[poolIndex]->set_alignment(glGetInteger(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// common: ~64KB
		pools[poolIndex]->set_maxSize(glGetInteger(GL_MAX_UNIFORM_BLOCK_SIZE));
		poolIndex = (int) SHADER_STORAGE_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
#ifdef USE_SHARED_SSBO_BUFFER
		pools[poolIndex]->set_alignment(glGetInteger(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// common: ~2GB
		pools[poolIndex]->set_maxSize(glGetInteger(GL_MAX_SHADER_STORAGE_BLOCK_SIZE));
		// common: ~4KB
		poolIndex = (int) ATOMIC_COUNTER_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
		pools[poolIndex]->set_maxSize(glGetInteger(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE));

		poolIndex = (int) DRAW_INDIRECT_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
		// The Indirect draw buffers are rather small.
		// One mesh has maybe 4 DrawCommands, each 32 bytes -> 128 bytes.
		// Set let's say we allow 100 meshes to be drawn in one indirect draw call,
		// so 100 * 128 = 12.5 KB.
		pools[poolIndex]->set_minSize(12800); // 12800 bytes = 12.5 KB
	}
}

void BufferObject::destroyMemoryPools() {
	auto *x = bufferPools();
	for (int i = 0; i < (int) BufferTarget::TARGET_LAST * (int) BUFFER_STORAGE_MODE_LAST; ++i) {
		delete x[i];
		x[i] = nullptr;
	}
}

ref_ptr<BufferReference> &BufferObject::adoptBufferRangeInPool(uint32_t numBytes, BufferPool *memoryPool) {
	if (numBytes == 0) {
		REGEN_WARN("Attempting to allocate buffer of 0 bytes.");
		return BufferReference::nullReference();
	}
	// get an allocator
	BufferPool::Node *allocator = memoryPool->chooseAllocator(numBytes);
	if (allocator == nullptr) {
		allocator = memoryPool->createAllocator(numBytes);
	}
	if (allocator == nullptr) {
		REGEN_ERROR("BufferObject::createReference: no allocator found for " << numBytes/1024.0 << " KB for " <<
			"buffer target " << flags_.target << ", access mode " << flags_.accessMode <<
			" and map mode " << flags_.mapMode << ".");
		return BufferReference::nullReference();
	}

	ref_ptr<BufferReference> ref = ref_ptr<BufferReference>::alloc();
	ref->poolReference_ = memoryPool->alloc(allocator, numBytes);
	if (ref->poolReference_.allocatorNode == nullptr) {
		return BufferReference::nullReference();
	}
	if (allocator->mapped) {
		// store persistent mapped data pointer with the reference
		ref->mappedData_ = (((byte*)allocator->mapped) + ref->address());
	}

	allocations_.push_back(ref);
	ref->allocatedSize_ = numBytes;
	ref->bufferObject_ = this;

	allocatedSize_ += numBytes;
	return allocations_.back();
}

ref_ptr<BufferReference> &BufferObject::adoptBufferRange(uint32_t numBytes) {
	BufferStorageMode storageMode = getBufferStorageMode(flags_);
	BufferPool *memoryPool = bufferPool(flags_.target, storageMode);
	return adoptBufferRangeInPool(numBytes, memoryPool);
}

ref_ptr<BufferReference> BufferObject::adoptBufferRange(uint32_t numBytes, BufferPool *memoryPool) {
	if (numBytes == 0) {
		REGEN_WARN("Attempting to allocate buffer of 0 bytes.");
		return BufferReference::nullReference();
	}
	// get an allocator
	BufferPool::Node *allocator = memoryPool->chooseAllocator(numBytes);
	if (allocator == nullptr) {
		allocator = memoryPool->createAllocator(numBytes);
	}
	if (allocator == nullptr) {
		return BufferReference::nullReference();
	}

	ref_ptr<BufferReference> ref = ref_ptr<BufferReference>::alloc();
	ref->poolReference_ = memoryPool->alloc(allocator, numBytes);
	if (ref->poolReference_.allocatorNode == nullptr) {
		return BufferReference::nullReference();
	}
	if (allocator->mapped) {
		// store persistent mapped data pointer with the reference
		ref->mappedData_ = (((byte*)allocator->mapped) + ref->address());
	}
	ref->allocatedSize_ = numBytes;

	return ref;
}

void BufferObject::orphanBufferRange(BufferReference *ref) {
	if (ref->bufferObject_ != nullptr) {
		auto *bo = (BufferObject *) ref->bufferObject_;
		bo->allocatedSize_ -= ref->allocatedSize_;
		for (auto it = bo->allocations_.begin(); it != bo->allocations_.end(); ++it) {
			if (it->get() == ref) {
				bo->allocations_.erase(it);
				break;
			}
		}
		ref->bufferObject_ = nullptr;
	}
}

// FIXME: Below code won't run well with implicit staging + multi-buffering.
//        The writes will always go to the first segment of the ring buffer,
//        no matter which segment is currently active for writing!
//        Also in this case we might want to interact with fences etc.
//        - could also push to flush queue if we would have access to staging buffer below.

void BufferObject::setBufferData(const void *data, const ref_ptr<BufferReference> &ref) {
	if (!flags_.isWritable()) {
		// CPU is not allowed to write to this buffer, so we cannot set data directly.
		// But we can copy data to a temporary buffer and then copy it to the target buffer.
		auto tempRef = BufferObject::adoptBufferRange(
				ref->allocatedSize(),
				bufferPool(flags_.target, BUFFER_MODE_STATIC_WRITE));
		glNamedBufferSubData(
				tempRef->bufferID(),
				tempRef->address(),
				tempRef->allocatedSize(),
				data);
		glCopyNamedBufferSubData(
				tempRef->bufferID(),
				ref->bufferID(),
				tempRef->address(),
				ref->address(),
				tempRef->allocatedSize());
	} else if(ref->mappedData()) {
		// the buffer is write-mapped, so we can write directly to it.
		// this might be the case for implicit staging buffers.
		std::memcpy(ref->mappedData(), data, ref->allocatedSize());
		if (flags_.mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
			glFlushMappedNamedBufferRange(ref->bufferID(),
				0, ref->allocatedSize());
		}
	} else {
		// the buffer is writable, but not mapped persistently
		glNamedBufferSubData(
				ref->bufferID(),
				ref->address(),
				ref->allocatedSize(),
				data);
	}
}

void BufferObject::setBufferData(const void *data) {
	setBufferData(data, allocations_[0]);
}

void BufferObject::setBufferData(const BufferObject &other) {
	auto &ref0 = allocations_[0];
	auto &ref1 = other.allocations_[0];
	glCopyNamedBufferSubData(
			ref1->bufferID(),
			ref0->bufferID(),
			ref1->address(),
			ref0->address(),
			ref1->allocatedSize());
}

void BufferObject::setBufferData(uint32_t readBufferID, uint32_t readAddress, uint32_t readSize) {
	auto &ref = allocations_[0];
	glCopyNamedBufferSubData(
			readBufferID,
			ref->bufferID(),
			readAddress,
			ref->address(),
			readSize);
}

void BufferObject::setBuffersToZero() {
	for (auto &ref: allocations_) {
		setBufferToZero(ref);
	}
}

inline void setToZero(const ref_ptr<BufferReference> &ref) {
	void *mappedData = glMapNamedBufferRange(
			ref->bufferID(),
			ref->address(),
			ref->allocatedSize(),
			GL_MAP_WRITE_BIT);
	if (mappedData) {
		std::memset(mappedData, 0, ref->allocatedSize());
		glUnmapNamedBuffer(ref->bufferID());
	} else {
		REGEN_ERROR("Failed to map buffer " << ref->bufferID() << " for writing.");
	}
}

void BufferObject::setBufferToZero(const ref_ptr<BufferReference> &ref) {
	if (!flags_.isWritable() || !flags_.isMappable()) {
		// CPU is not allowed to write to this buffer, so we cannot set data directly.
		// But we can copy data to a temporary buffer and then copy it to the target buffer.
		auto tempRef = adoptBufferRange(ref->allocatedSize(),
				bufferPool(flags_.target, BUFFER_MODE_CPU_W_MAP_TEMPORARY));
		setToZero(tempRef);
		glCopyNamedBufferSubData(
				tempRef->bufferID(),
				ref->bufferID(),
				tempRef->address(),
				ref->address(),
				tempRef->allocatedSize());
	} else if(ref->mappedData()) {
		// the buffer is write-mapped, so we can write directly to it.
		memset(ref->mappedData(), 0, ref->allocatedSize());
		if (flags_.mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
			glFlushMappedNamedBufferRange(ref->bufferID(),
				0, ref->allocatedSize());
		}
	} else {
		// temporarily map the buffer to write zeroes to it.
		setToZero(ref);
	}
}

void BufferObject::copy(
		uint32_t from,
		uint32_t to,
		uint32_t size,
		uint32_t offset,
		uint32_t toOffset) {
	glCopyNamedBufferSubData(
			from,
			to,
			offset,
			toOffset,
			size);
}

void BufferObject::setBufferSubData(uint32_t localOffset, uint32_t dataSize, const void *data) {
	auto &ref = allocations_[0];
	if (!flags_.isWritable()) {
		// CPU is not allowed to write to this buffer, so we cannot set data directly.
		// But we can copy data to a temporary buffer and then copy it to the target buffer.
		auto tempRef = adoptBufferRange(dataSize,
				bufferPool(flags_.target, BUFFER_MODE_STATIC_WRITE));
		glNamedBufferSubData(
				tempRef->bufferID(),
				tempRef->address(),
				dataSize,
				data);
		glCopyNamedBufferSubData(
				tempRef->bufferID(),
				ref->bufferID(),
				tempRef->address(),
				ref->address() + localOffset,
				dataSize);
	} else if (ref->mappedData()) {
		// the buffer is write-mapped, so we can write directly to it.
		std::memcpy(ref->mappedData() + localOffset, data, dataSize);
		if (flags_.mapMode == BUFFER_MAP_PERSISTENT_FLUSH) {
			glFlushMappedNamedBufferRange(ref->bufferID(),
				localOffset, dataSize);
		}
	} else {
		glNamedBufferSubData(
				ref->bufferID(),
				ref->address() + localOffset,
				dataSize,
				data);
	}
}

void BufferObject::readBufferSubData(uint32_t localOffset, uint32_t dataSize, byte *data) {
	auto &ref = allocations_[0];
	if (!flags_.isReadable()) {
		// CPU is not allowed to read from this buffer, so we cannot read data directly.
		// But we can copy data to a temporary buffer and then copy it to the target buffer.
		auto tempRef = adoptBufferRange(dataSize,
				bufferPool(flags_.target, BUFFER_MODE_STATIC_READ));
		glCopyNamedBufferSubData(
				ref->bufferID(),
				tempRef->bufferID(),
				ref->address() + localOffset,
				tempRef->address(),
				dataSize);
		auto *tempData = glMapNamedBufferRange(
				tempRef->bufferID(),
				tempRef->address(),
				dataSize,
				GL_MAP_READ_BIT);
		if (tempData) {
			std::memcpy(data, tempData, dataSize);
			glUnmapNamedBuffer(tempRef->bufferID());
		}
	} else if (ref->mappedData()) {
		// the buffer is mapped, so we can read directly from it.
		std::memcpy(data, ref->mappedData() + localOffset, dataSize);
	} else {
		glGetNamedBufferSubData(
				ref->bufferID(),
				ref->address() + localOffset,
				dataSize,
				data);
	}
}

void *BufferObject::map(uint32_t relativeOffset, uint32_t mappedSize, uint32_t accessFlags) {
	auto &ref = allocations_[0];
	if (ref->mappedData()) {
		return ref->mappedData() + relativeOffset;
	} else {
		return glMapNamedBufferRange(
				ref->bufferID(),
				ref->address() + relativeOffset,
				mappedSize,
				accessFlags);
	}
}

void *BufferObject::map(uint32_t accessFlags) {
	return map(allocations_[0], accessFlags);
}

void *BufferObject::map(const ref_ptr<BufferReference> &ref, uint32_t accessFlags) {
	if (ref->mappedData()) {
		return ref->mappedData();
	} else {
		return glMapNamedBufferRange(
				ref->bufferID(),
				ref->address(),
				ref->allocatedSize(),
				accessFlags);
	}
}

void BufferObject::unmap() const {
	auto &ref = allocations_[0];
	if (!ref->mappedData()) {
		// only unmap if the buffer is not mapped persistently
		glUnmapNamedBuffer(ref->bufferID());
	}
}

uint32_t BufferObject::attributeSize(const std::vector<ref_ptr<ShaderInput> > &attributes) {
	if (!attributes.empty()) {
		uint32_t structSize = 0;
		for (const auto &attribute: attributes) {
			structSize += attribute->inputSize();
		}
		return structSize;
	}
	return 0;
}
