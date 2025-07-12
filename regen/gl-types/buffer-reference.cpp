#include "buffer-reference.h"
#include "buffer-object.h"

using namespace regen;

BufferReference::~BufferReference() {
	// memory in pool is marked as free when reference destructor is called
	if (BufferObject::bufferPools()[0] != nullptr && poolReference_.allocatorNode != nullptr) {
		BufferPool *pool = poolReference_.allocatorNode->pool;
		pool->free(poolReference_);
		poolReference_.allocatorNode = nullptr;
	}
}

uint32_t BufferReference::address() const {
	// virtual address is the virtual allocator reference
	return poolReference_.allocatorRef;
}

uint32_t BufferReference::bufferID() const {
	// GL buffer handle is the actual allocator reference
	return poolReference_.allocatorNode->allocatorRef;
}

ref_ptr<BufferReference> &BufferReference::nullReference() {
	static ref_ptr<BufferReference> ref;
	if (ref.get() == nullptr) {
		ref = ref_ptr<BufferReference>::alloc();
		ref->allocatedSize_ = 0;
		ref->bufferObject_ = nullptr;
		ref->poolReference_.allocatorNode = nullptr;
	}
	return ref;
}
