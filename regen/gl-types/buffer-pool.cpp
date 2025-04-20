#include "buffer-pool.h"
#include "buffer-target.h"
#include "buffer-usage.h"

using namespace regen;

GLuint BufferAllocator::createAllocator(GLuint poolIndex, GLuint size) {
	RenderState *rs = RenderState::get();
	GLuint ref;
	glGenBuffers(1, &ref);
	rs->copyWriteBuffer().push(ref);
	glBufferData(GL_COPY_WRITE_BUFFER, size, nullptr,
		glBufferUsage((BufferUsage)(poolIndex % BUFFER_USAGE_LAST)));
	rs->copyWriteBuffer().pop();
	return ref;
}

void BufferAllocator::deleteAllocator(GLuint poolIndex, GLuint ref) {
	glDeleteBuffers(1, &ref);
}
