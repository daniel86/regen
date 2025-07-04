#include "buffer-pool.h"
#include "buffer-usage.h"

using namespace regen;

GLuint BufferAllocator::createAllocator(GLuint poolIndex, GLuint size) {
	auto usage = glBufferUsage((BufferUsage)(poolIndex % BUFFER_USAGE_LAST));
	//auto target = glBufferTarget((BufferTarget)(poolIndex / BUFFER_USAGE_LAST));
	GLuint ref;

	glCreateBuffers(1, &ref);
	glNamedBufferData(ref, size, nullptr, usage);
	return ref;
}

void BufferAllocator::deleteAllocator(GLuint poolIndex, GLuint ref) {
	glDeleteBuffers(1, &ref);
}
