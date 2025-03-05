#include "texture-1d.h"

using namespace regen;

Texture1D::Texture1D(GLuint numTextures)
		: Texture(numTextures) {
	dim_ = 1;
	texBind_.target_ = GL_TEXTURE_1D;
	samplerType_ = "sampler1D";
}

void Texture1D::texImage() const {
	glTexImage1D(
			texBind_.target_,
			0, // mipmap level
			internalFormat_,
			width(),
			border_,
			format_,
			pixelType_,
			textureData_);
}
