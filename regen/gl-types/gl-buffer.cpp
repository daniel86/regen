#include "gl-buffer.h"

using namespace regen;

GLBuffer::GLBuffer(GLenum bufferTarget) :
	GLObject(glGenBuffers, glDeleteBuffers),
	bufferTarget_(bufferTarget) {
}

void GLBuffer::bindBufferBase(GLuint bindingPoint) const {
	RenderState::get()->bindBufferBase(bufferTarget_, bindingPoint, id());
}
