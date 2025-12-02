#include "buffer-allocator.h"
#include "buffer-enums.h"

using namespace regen;

uint32_t BufferAllocator::createAllocator(uint32_t poolIdx, uint32_t size) {
	// create a fresh GL buffer object.
	uint32_t ref;
	glCreateBuffers(1, &ref);
	// allocate the storage for the buffer object.
	const uint32_t flags = glStorageFlags((BufferStorageMode)(poolIdx % BUFFER_STORAGE_MODE_LAST));
    glNamedBufferStorage(ref, size, nullptr, flags);
	return ref;
}

void BufferAllocator::deleteAllocator(uint32_t /*poolIndex*/, uint32_t ref) {
	glDeleteBuffers(1, &ref);
}

void* BufferAllocator::mapAllocator(uint32_t poolIdx, uint32_t size, uint32_t ref) {
	const uint32_t flags = glAccessFlags((BufferStorageMode)(poolIdx % BUFFER_STORAGE_MODE_LAST));
	if (flags & GL_MAP_PERSISTENT_BIT) {
		return glMapNamedBufferRange(ref, 0, size, flags);
	} else {
		return nullptr;
	}
}

void BufferAllocator::unmapAllocator(uint32_t poolIdx, uint32_t ref) {
	const uint32_t flags = glAccessFlags((BufferStorageMode)(poolIdx % BUFFER_STORAGE_MODE_LAST));
	if (flags & GL_MAP_PERSISTENT_BIT) {
		glUnmapNamedBuffer(ref);
	}
}

void BufferAllocator::orphanAllocatorRange(uint32_t ref, uint32_t offset, uint32_t size) {
	glInvalidateBufferSubData(ref, offset, size);
}
