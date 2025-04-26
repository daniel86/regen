#include "buffer-pool.h"
#include "buffer-target.h"
#include "buffer-usage.h"

using namespace regen;

GLuint BufferAllocator::createAllocator(GLuint poolIndex, GLuint size) {
	auto usage = glBufferUsage((BufferUsage)(poolIndex % BUFFER_USAGE_LAST));
	auto target = glBufferTarget((BufferTarget)(poolIndex / BUFFER_USAGE_LAST));
	GLuint ref;

	glGenBuffers(1, &ref);
	RenderState::get()->buffer(target).apply(ref);
	glBufferData(target, size, nullptr, usage);
	return ref;
}

void BufferAllocator::deleteAllocator(GLuint poolIndex, GLuint ref) {
	glDeleteBuffers(1, &ref);
}
