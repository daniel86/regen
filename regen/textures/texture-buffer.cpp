#include "texture-buffer.h"
#include "regen/gl/gl-util.h"

using namespace regen;

TextureBuffer::TextureBuffer(GLenum texelFormat)
		: Texture(GL_TEXTURE_BUFFER, 1) {
	if (glenum::isSignedIntegerType(texelFormat)) {
		samplerType_ = "isamplerBuffer";
	} else if (glenum::isUnsignedIntegerType(texelFormat)) {
		samplerType_ = "usamplerBuffer";
	} else {
		samplerType_ = "samplerBuffer";
	}
	texelFormat_ = texelFormat;
	allocTexture_ = &TextureBuffer::allocTexture_noop;
	updateImage_ = &TextureBuffer::updateImage_noop;
	updateSubImage_ = &TextureBuffer::updateSubImage_noop;
}

void TextureBuffer::attach(const ref_ptr<BufferReference> &ref) {
	attachedVBORef_ = ref;
	numTexel_ = attachedVBORef_->allocatedSize() / sizeof(GLubyte);
#ifdef GL_ARB_texture_buffer_range
	glTextureBufferRange(
			id(),
			texelFormat_,
			ref->bufferID(),
			ref->address(),
			ref->allocatedSize());
#else
	glTextureBuffer(id(), texelFormat_, ref->bufferID());
#endif
	GL_ERROR_LOG();
}

void TextureBuffer::attach(uint32_t storage) {
	attachedVBORef_ = {};
	glTextureBuffer(id(), texelFormat_, storage);
	GL_ERROR_LOG();
}

void TextureBuffer::attach(uint32_t storage, uint32_t offset, uint32_t size) {
	attachedVBORef_ = {};
#ifdef GL_ARB_texture_buffer_range
	glTextureBufferRange(id(), texelFormat_, storage, offset, size);
#else
	glTextureBuffer(id(), texelFormat_, storage);
#endif
	GL_ERROR_LOG();
}
