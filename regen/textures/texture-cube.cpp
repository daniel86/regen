#include "texture-cube.h"

using namespace regen;

TextureCube::TextureCube(GLuint numTextures)
		: Texture2D(numTextures),
		  cubeData_() {
	samplerType_ = "samplerCube";
	texBind_.target_ = GL_TEXTURE_CUBE_MAP;
	dim_ = 3;
	for (int i = 0; i < 6; ++i) { cubeData_[i] = nullptr; }
}

void TextureCube::set_data(CubeSide side, void *data) {
	cubeData_[side] = data;
}

void **TextureCube::cubeData() {
	return cubeData_;
}

void TextureCube::texImage() const {
	cubeTexImage(LEFT);
	cubeTexImage(RIGHT);
	cubeTexImage(TOP);
	cubeTexImage(BOTTOM);
	cubeTexImage(FRONT);
	cubeTexImage(BACK);
}

void TextureCube::cubeTexImage(CubeSide side) const {
	glTexImage2D(glenum::cubeMapLayer(side),
				 0, // mipmap level
				 internalFormat_,
				 width(),
				 height(),
				 border_,
				 format_,
				 pixelType_,
				 cubeData_[side]);
}

TextureCubeDepth::TextureCubeDepth(GLuint numTextures)
		: TextureCube(numTextures) {
	format_ = GL_DEPTH_COMPONENT;
	internalFormat_ = GL_DEPTH_COMPONENT;
	pixelType_ = GL_UNSIGNED_BYTE;
}
