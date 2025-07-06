#include "buffer-pool.h"
#include "buffer-usage.h"
#include "buffer-target.h"

using namespace regen;

GLuint BufferAllocator::createAllocator(GLuint poolIdx, GLuint size) {
	GLuint ref;
	glCreateBuffers(1, &ref);

	auto usage = glBufferUsage((BufferStorageMode)(poolIdx % BUFFER_STORAGE_MODE_LAST));
	//auto target = glBufferTarget((BufferTarget)(poolIdx / BUFFER_USAGE_LAST));
    glNamedBufferStorage(ref, size, nullptr, usage);

	return ref;

}

void BufferAllocator::deleteAllocator(GLuint poolIndex, GLuint ref) {
	glDeleteBuffers(1, &ref);
}
