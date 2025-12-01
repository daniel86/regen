#include "render-buffer.h"

using namespace regen;

RenderBuffer::RenderBuffer(uint32_t numBuffers)
		: GLRectangle(glCreateRenderbuffers, glDeleteRenderbuffers, numBuffers),
		  format_(GL_RGBA) {
}

RenderBuffer::RenderBuffer(
		GLenum format,
		uint32_t width,
		uint32_t height,
		uint32_t numBuffers)
		: GLRectangle(glCreateRenderbuffers, glDeleteRenderbuffers, numBuffers),
		  format_(format) {
	set_rectangleSize(width, height);
	RenderState::get()->renderBuffer().push(id());
	storage();
	RenderState::get()->renderBuffer().pop();
}

void RenderBuffer::storageMS(uint32_t numSamples) const {
	glNamedRenderbufferStorageMultisample(id(), numSamples, format_, width(), height());
}

void RenderBuffer::storage() const {
	glNamedRenderbufferStorage(id(), format_, width(), height());
}
