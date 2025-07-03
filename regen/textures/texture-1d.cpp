#include "texture-1d.h"

using namespace regen;

Texture1D::Texture1D(GLuint numTextures)
		: Texture(numTextures) {
	dim_ = 1;
	texBind_.target_ = GL_TEXTURE_1D;
	samplerType_ = "sampler1D";
}

void Texture1D::allocateTextureStorage(int numLevels) {
	auto w = static_cast<int32_t>(width());
	glTextureStorage1D(id(),
					   numLevels,
					   internalFormat_,
					   w);
	glTextureSubImage1D(id(),
						0, // mipmap level
						0, // x offset
						w,
						format_,
						pixelType_,
						textureData_);
	/**
	glTexImage1D(
			texBind_.target_,
			0, // mipmap level
			internalFormat_,
			width(),
			border_,
			format_,
			pixelType_,
			textureData_);
	 */
}
