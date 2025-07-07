#include "buffer-object.h"

#define USE_SHARED_TBO_BUFFER
#define USE_SHARED_UBO_BUFFER
#define USE_SHARED_SSBO_BUFFER

using namespace regen;

BufferObject::BufferObject(BufferTarget target, BufferUpdateHint hint) :
		Resource(),
		updateHint_(hint),
		target_(target),
		glTarget_(glBufferTarget(target)),
		allocatedSize_(0) {
}

BufferObject::BufferObject(const BufferObject &other) :
		Resource(),
		updateHint_(other.updateHint_),
		mapMode_(other.mapMode_),
		accessMode_(other.accessMode_),
		target_(other.target_),
		glTarget_(glBufferTarget(target_)),
		allocations_(other.allocations_),
		allocatedSize_(other.allocatedSize_) {
}

BufferObject::~BufferObject() {
	while (!allocations_.empty()) {
		ref_ptr<BufferReference> ref = *allocations_.begin();
		if (ref->bufferObject_ != nullptr) {
			free(ref.get());
		} else {
			allocations_.erase(allocations_.begin());
		}
	}
}

void BufferObject::setBufferAccessMode(BufferAccessMode mode) {
	if (mode == BUFFER_GPU_ONLY) {
		return; // no need to set anything
	}
	else if (mode == BUFFER_CPU_READ) {
		if (accessMode_ == BUFFER_CPU_WRITE) {
			accessMode_ = BUFFER_CPU_READ_WRITE;
		}
		else if (accessMode_ == BUFFER_CPU_READ_WRITE || accessMode_ == BUFFER_CPU_READ) {
			// nothing to do, already set
			return;
		}
		else {
			accessMode_ = mode;
		}
	}
	else if (mode == BUFFER_CPU_WRITE) {
		if (accessMode_ == BUFFER_CPU_READ) {
			accessMode_ = BUFFER_CPU_READ_WRITE;
		}
		else if (accessMode_ == BUFFER_CPU_READ_WRITE || accessMode_ == BUFFER_CPU_WRITE) {
			// nothing to do, already set
			return;
		}
		else {
			accessMode_ = mode;
		}
	}
	else {
		accessMode_ = mode;
	}
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
		poolIndex = (int) TEXTURE_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
#ifdef USE_SHARED_TBO_BUFFER
		pools[poolIndex]->set_alignment(getGLInteger(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// Meaning: Max number of texels, not bytes!
		pools[poolIndex]->set_maxSize(getGLInteger(GL_MAX_TEXTURE_BUFFER_SIZE) * 16);
		poolIndex = (int) UNIFORM_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
#ifdef USE_SHARED_UBO_BUFFER
		pools[poolIndex]->set_alignment(getGLInteger(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// common: ~64KB
		pools[poolIndex]->set_maxSize(getGLInteger(GL_MAX_UNIFORM_BLOCK_SIZE));
		poolIndex = (int) SHADER_STORAGE_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
#ifdef USE_SHARED_SSBO_BUFFER
		pools[poolIndex]->set_alignment(getGLInteger(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// common: ~2GB
		pools[poolIndex]->set_maxSize(getGLInteger(GL_MAX_SHADER_STORAGE_BLOCK_SIZE));
		// common: ~4KB
		poolIndex = (int) ATOMIC_COUNTER_BUFFER * (int) BUFFER_STORAGE_MODE_LAST + i;
		pools[poolIndex]->set_maxSize(getGLInteger(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE));
	}
}

void BufferObject::destroyMemoryPools() {
	auto *x = bufferPools();
	for (int i = 0; i < (int) BufferTarget::TARGET_LAST * (int) BUFFER_STORAGE_MODE_LAST; ++i) {
		delete x[i];
		x[i] = nullptr;
	}
}

ref_ptr<BufferReference> &BufferObject::nullReference() {
	static ref_ptr<BufferReference> ref;
	if (ref.get() == nullptr) {
		ref = ref_ptr<BufferReference>::alloc();
		ref->allocatedSize_ = 0;
		ref->bufferObject_ = nullptr;
		ref->poolReference_.allocatorNode = nullptr;
	}
	return ref;
}

ref_ptr<BufferReference> &BufferObject::createReference(GLuint numBytes) {
	if (numBytes == 0) {
		REGEN_WARN("Attempting to allocate buffer of 0 bytes.");
		return nullReference();
	}
	BufferStorageMode storageMode = getBufferStorageMode(
		accessMode_, mapMode_, updateHint_);
	BufferPool *memoryPool_ = bufferPool(target_, storageMode);
	// get an allocator
	BufferPool::Node *allocator = memoryPool_->chooseAllocator(numBytes);
	if (allocator == nullptr) {
		allocator = memoryPool_->createAllocator(numBytes);
	}
	if (allocator == nullptr) {
		REGEN_ERROR("BufferObject::createReference: no allocator found for " << numBytes/1024.0 << " KB for " <<
			"buffer target " << target_ << ", access mode " << accessMode_ <<
			" and map mode " << mapMode_ << ".");
		return nullReference();
	}

	ref_ptr<BufferReference> ref = ref_ptr<BufferReference>::alloc();
	ref->poolReference_ = memoryPool_->alloc(allocator, numBytes);
	if (ref->poolReference_.allocatorNode == nullptr) {
		return nullReference();
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

void BufferObject::free(BufferReference *ref) {
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

ref_ptr<BufferReference> &BufferObject::allocBytes(GLuint numBytes) {
	return createReference(numBytes);
}

void BufferObject::setBufferData(const void *data, const ref_ptr<BufferReference> &ref) {
	// FIXME: what about ring buffer here? and also everywhere below?
	if(ref->mappedData()) {
		REGEN_WARN("interface might be broken!");
		memcpy(ref->mappedData(), data, ref->allocatedSize());
	} else {
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
	glCopyNamedBufferSubData(
			other.allocations_[0]->bufferID(),
			allocations_[0]->bufferID(),
			other.allocations_[0]->address(),
			allocations_[0]->address(),
			other.allocations_[0]->allocatedSize());
}

void BufferObject::copy(
		GLuint from,
		GLuint to,
		GLuint size,
		GLuint offset,
		GLuint toOffset) {
	glCopyNamedBufferSubData(
			from,
			to,
			offset,
			toOffset,
			size);
}

void BufferObject::setBufferSubData(const void *data, GLuint relativeOffset, GLuint dataSize) {
	auto &ref = allocations_[0];
	if (ref->mappedData()) {
		REGEN_WARN("interface might be broken!");
		memcpy(ref->mappedData() + relativeOffset, data, dataSize);
	} else {
		glNamedBufferSubData(
				ref->bufferID(),
				ref->address() + relativeOffset,
				dataSize,
				data);
	}
}

GLvoid *BufferObject::map(GLuint relativeOffset, GLuint mappedSize, uint32_t accessFlags) {
	auto &ref = allocations_[0];
	if (ref->mappedData()) {
		REGEN_WARN("interface might be broken!");
		return ref->mappedData() + relativeOffset;
	} else {
		return glMapNamedBufferRange(
				allocations_[0]->bufferID(),
				allocations_[0]->address() + relativeOffset,
				mappedSize,
				accessFlags);
	}
}

GLvoid *BufferObject::map(uint32_t accessFlags) {
	return BufferObject::map(allocations_[0], accessFlags);
}

GLvoid *BufferObject::map(const ref_ptr<BufferReference> &ref, uint32_t accessFlags) {
	if (ref->mappedData()) {
		REGEN_WARN("interface might be broken!");
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
		glUnmapNamedBuffer(allocations_[0]->bufferID());
	}
}

GLuint BufferObject::attributeSize(const std::list<ref_ptr<ShaderInput> > &attributes) {
	if (!attributes.empty()) {
		GLuint structSize = 0;
		for (const auto &attribute: attributes) {
			structSize += attribute->inputSize();
		}
		return structSize;
	}
	return 0;
}
