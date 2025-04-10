#include "texture-buffer.h"
#include "regen/gl-types/gl-util.h"

using namespace regen;

TextureBuffer::TextureBuffer(GLenum texelFormat)
		: Texture() {
	texBind_.target_ = GL_TEXTURE_BUFFER;
	if (glenum::isSignedIntegerType(texelFormat)) {
		samplerType_ = "isamplerBuffer";
	} else if (glenum::isUnsignedIntegerType(texelFormat)) {
		samplerType_ = "usamplerBuffer";
	} else {
		samplerType_ = "samplerBuffer";
	}
	texelFormat_ = texelFormat;
}

void TextureBuffer::attach(const ref_ptr<BufferReference> &ref) {
	attachedVBORef_ = ref;
#ifdef GL_ARB_texture_buffer_range
	glTexBufferRange(
			texBind_.target_,
			texelFormat_,
			ref->bufferID(),
			ref->address(),
			ref->allocatedSize());
#else
	glTexBuffer(texBind_.target_, texelFormat_, ref->bufferID());
#endif
	GL_ERROR_LOG();
}

void TextureBuffer::attach(GLuint storage) {
	attachedVBORef_ = {};
	glTexBuffer(texBind_.target_, texelFormat_, storage);
	GL_ERROR_LOG();
}

void TextureBuffer::attach(GLuint storage, GLuint offset, GLuint size) {
	attachedVBORef_ = {};
#ifdef GL_ARB_texture_buffer_range
	glTexBufferRange(texBind_.target_, texelFormat_, storage, offset, size);
#else
	glTexBuffer(texBind_.target_, texelFormat_, storage);
#endif
	GL_ERROR_LOG();
}

unsigned int TextureBuffer::numTexel() const {
	return attachedVBORef_->allocatedSize() / sizeof(GLubyte);
}

void TextureBuffer::texImage() const {
}
