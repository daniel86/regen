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

unsigned int BufferReference::address() const {
	// virtual address is the virtual allocator reference
	return poolReference_.allocatorRef;
}

unsigned int BufferReference::bufferID() const {
	// GL buffer handle is the actual allocator reference
	return poolReference_.allocatorNode->allocatorRef;
}
