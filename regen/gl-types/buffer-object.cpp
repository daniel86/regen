#include "buffer-object.h"

#define USE_SHARED_TBO_BUFFER
#define USE_SHARED_UBO_BUFFER
#define USE_SHARED_SSBO_BUFFER

using namespace regen;

BufferObject::BufferObject(BufferTarget target, BufferUsage usage) :
		Resource(),
		usage_(usage),
		target_(target),
		glTarget_(glBufferTarget(target)),
		allocatedSize_(0) {
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

BufferObject::BufferObject(const BufferObject &other) :
		Resource(),
		usage_(other.usage_),
		target_(other.target_),
		glTarget_(glBufferTarget(target_)),
		allocations_(other.allocations_),
		allocatedSize_(other.allocatedSize_) {
}

BufferPool **BufferObject::bufferPools() {
	static std::array<BufferPool *, (int) BufferTarget::TARGET_LAST * (int) BufferUsage::USAGE_LAST> bufferPools;
	return bufferPools.data();
}

BufferPool *BufferObject::bufferPool(BufferTarget target, BufferUsage usage) {
	auto *x = bufferPools();
	auto poolIndex = (int) target * BufferUsage::USAGE_LAST + (int) usage;
	return x[poolIndex];
}

void BufferObject::createMemoryPools() {
	auto *pools = bufferPools();
	for (int i = 0; i < (int) BufferTarget::TARGET_LAST * (int) BufferUsage::USAGE_LAST; ++i) {
		if (pools[i] == nullptr) {
			pools[i] = new BufferPool();
			pools[i]->set_index(i);
		}
	}
	// some buffer semantics need special attention as they require
	// alignment to be set, i.e. when using shared buffers consecutive
	// allocations need to be aligned to the size of the buffer.
	for (int i = 0;  i < BufferUsage::USAGE_LAST; ++i) {
		int poolIndex;
		poolIndex = (int) TEXTURE_BUFFER * (int) BufferUsage::USAGE_LAST + i;
#ifdef USE_SHARED_TBO_BUFFER
		pools[poolIndex]->set_alignment(getGLInteger(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// Meaning: Max number of texels, not bytes!
		pools[poolIndex]->set_maxSize(getGLInteger(GL_MAX_TEXTURE_BUFFER_SIZE) * 16);
		poolIndex = (int) UNIFORM_BUFFER * (int) BufferUsage::USAGE_LAST + i;
#ifdef USE_SHARED_UBO_BUFFER
		pools[poolIndex]->set_alignment(getGLInteger(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// common: ~64KB
		pools[poolIndex]->set_maxSize(getGLInteger(GL_MAX_UNIFORM_BLOCK_SIZE));
		poolIndex = (int) SHADER_STORAGE_BUFFER * (int) BufferUsage::USAGE_LAST + i;
#ifdef USE_SHARED_SSBO_BUFFER
		pools[poolIndex]->set_alignment(getGLInteger(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT));
#else
		pools[poolIndex]->set_minSize(1);
#endif
		// common: ~2GB
		pools[poolIndex]->set_maxSize(getGLInteger(GL_MAX_SHADER_STORAGE_BLOCK_SIZE));
		// common: ~4KB
		poolIndex = (int) ATOMIC_COUNTER_BUFFER * (int) BufferUsage::USAGE_LAST + i;
		pools[poolIndex]->set_maxSize(getGLInteger(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE));
	}
}

void BufferObject::destroyMemoryPools() {
	auto *x = bufferPools();
	for (int i = 0; i < BufferUsage::USAGE_LAST * BufferUsage::USAGE_LAST; ++i) {
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
	BufferPool *memoryPool_ = bufferPool(target_, usage_);
	// get an allocator
	BufferPool::Node *allocator = memoryPool_->chooseAllocator(numBytes);
	if (allocator == nullptr) {
		allocator = memoryPool_->createAllocator(numBytes);
	}
	if (allocator == nullptr) {
		REGEN_ERROR("BufferObject::createReference: no allocator found for " << numBytes/1024.0 << " KB for " <<
			"buffer target " << target_ << " and usage " << usage_);
		return nullReference();
	}

	ref_ptr<BufferReference> ref = ref_ptr<BufferReference>::alloc();
	ref->poolReference_ = memoryPool_->alloc(allocator, numBytes);
	if (ref->poolReference_.allocatorNode == nullptr) { return nullReference(); }

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

void BufferObject::bind(GLuint index) const {
	auto &ref = allocations_[0];
	RenderState::get()->bufferRange(glTarget_).apply(index, BufferRange(
		ref->bufferID(),
		ref->address(),
		ref->allocatedSize()));
}

void BufferObject::setBufferData(const ref_ptr<BufferReference> &ref, const GLuint *data) {
	RenderState::get()->copyWriteBuffer().push(ref->bufferID());
	glBufferSubData(GL_COPY_WRITE_BUFFER,
					ref->address(),
					ref->allocatedSize(),
					data);
	RenderState::get()->copyWriteBuffer().pop();
}

GLvoid *BufferObject::map(GLenum target, GLuint offset, GLuint size, GLenum accessFlags) {
	return glMapBufferRange(target, offset, size, accessFlags);
}

GLvoid *BufferObject::map(const ref_ptr<BufferReference> &ref, GLenum accessFlags) const {
	return glMapBufferRange(
			glTarget_,
			ref->address(),
			ref->allocatedSize(),
			accessFlags);
}

void BufferObject::unmap(GLenum target) {
	glUnmapBuffer(target);
}

void BufferObject::unmap() const {
	glUnmapBuffer(glTarget_);
}

void BufferObject::copy(
		GLuint from,
		GLuint to,
		GLuint size,
		GLuint offset,
		GLuint toOffset) {
	RenderState *rs = RenderState::get();
	rs->copyReadBuffer().push(from);
	rs->copyWriteBuffer().push(to);
	glCopyBufferSubData(
			GL_COPY_READ_BUFFER,
			GL_COPY_WRITE_BUFFER,
			offset,
			toOffset,
			size);
	rs->copyReadBuffer().pop();
	rs->copyWriteBuffer().pop();
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
