#include "texture-array.h"

using namespace regen;

Texture2DArray::Texture2DArray(GLuint numTextures) : Texture3D(numTextures) {
	samplerType_ = "sampler2DArray";
	texBind_.target_ = GL_TEXTURE_2D_ARRAY;
}
