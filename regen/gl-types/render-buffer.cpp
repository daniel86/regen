#include "render-buffer.h"

using namespace regen;

RenderBuffer::RenderBuffer(GLuint numBuffers)
		: GLRectangle(glGenRenderbuffers, glDeleteRenderbuffers, numBuffers),
		  format_(GL_RGBA) {
}

void RenderBuffer::set_format(GLenum format) { format_ = format; }

void RenderBuffer::begin(RenderState *rs) { rs->renderBuffer().push(id()); }

void RenderBuffer::end(RenderState *rs) { rs->renderBuffer().pop(); }

void RenderBuffer::storageMS(GLuint numMultisamples) const {
	glRenderbufferStorageMultisample(
			GL_RENDERBUFFER, numMultisamples, format_, width(), height());
}

void RenderBuffer::storage() const {
	glRenderbufferStorage(
			GL_RENDERBUFFER, format_, width(), height());
}
