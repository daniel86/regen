#include "texture-3d.h"

using namespace regen;

Texture3D::Texture3D(GLuint numTextures)
		: Texture(numTextures),
		  numTextures_(1) {
	dim_ = 3;
	texBind_.target_ = GL_TEXTURE_3D;
	samplerType_ = "sampler3D";
}

void Texture3D::set_depth(GLuint numTextures) {
	numTextures_ = numTextures;
}

void Texture3D::texImage() const {
	glTexImage3D(texBind_.target_,
				 0, // mipmap level
				 internalFormat_,
				 width(),
				 height(),
				 numTextures_,
				 border_,
				 format_,
				 pixelType_,
				 textureData_);
}

void Texture3D::texSubImage(GLint layer, GLubyte *subData) const {
	glTexSubImage3D(
			texBind_.target_,
			0, 0, 0, // offset
			layer,
			width(),
			height(),
			1,
			format_,
			pixelType_,
			subData);
}

Texture3DDepth::Texture3DDepth(GLuint numTextures) : Texture3D(numTextures) {
	format_ = GL_DEPTH_COMPONENT;
	internalFormat_ = GL_DEPTH_COMPONENT;
}
