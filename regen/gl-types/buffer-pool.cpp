#include "buffer-pool.h"
#include "buffer-usage.h"

using namespace regen;

GLuint BufferAllocator::createAllocator(GLuint poolIdx, GLuint size) {
	GLuint ref;
	glCreateBuffers(1, &ref);

	const uint32_t flags = glStorageFlags((BufferStorageMode)(poolIdx % BUFFER_STORAGE_MODE_LAST));
	//auto target = glBufferTarget((BufferTarget)(poolIdx / BUFFER_USAGE_LAST));
    glNamedBufferStorage(ref, size, nullptr, flags);

	return ref;

}

void BufferAllocator::deleteAllocator(GLuint /*poolIndex*/, GLuint ref) {
	glDeleteBuffers(1, &ref);
}

void* BufferAllocator::mapAllocator(GLuint poolIdx, GLuint size, GLuint ref) {
	const uint32_t flags = glAccessFlags((BufferStorageMode)(poolIdx % BUFFER_STORAGE_MODE_LAST));
	if (flags & GL_MAP_PERSISTENT_BIT) {
		return glMapNamedBufferRange(ref, 0, size, flags);
	} else {
		return nullptr;
	}
}

void BufferAllocator::unmapAllocator(GLuint poolIdx, GLuint ref) {
	const uint32_t flags = glAccessFlags((BufferStorageMode)(poolIdx % BUFFER_STORAGE_MODE_LAST));
	if (flags & GL_MAP_PERSISTENT_BIT) {
		glUnmapNamedBuffer(ref);
	}
}
