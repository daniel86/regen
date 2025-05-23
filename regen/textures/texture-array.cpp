#include "texture-array.h"

using namespace regen;

Texture2DArray::Texture2DArray(GLuint numTextures) : Texture3D(numTextures) {
	samplerType_ = "sampler2DArray";
	texBind_.target_ = GL_TEXTURE_2D_ARRAY;
}

Texture2DArrayDepth::Texture2DArrayDepth(GLuint numTextures) : Texture2DArray(numTextures) {
	format_ = GL_DEPTH_COMPONENT;
	internalFormat_ = GL_DEPTH_COMPONENT;
	pixelType_ = GL_UNSIGNED_BYTE;
}

Texture2DArrayMultisample::Texture2DArrayMultisample(
		GLsizei numSamples,
		GLuint numTextures,
		GLboolean fixedSampleLocations)
		: Texture2DArray(numTextures),
		  numSamples_(numSamples),
		  fixedSampleLocations_(fixedSampleLocations) {
	samplerType_ = "sampler2DMSArray";
	texBind_.target_ = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
	set_numSamples(numSamples);
}

Texture2DArrayMultisampleDepth::Texture2DArrayMultisampleDepth(
		GLsizei numSamples,
		GLuint numTextures,
		GLboolean fixedSampleLocations)
		: Texture2DArray(numTextures),
		  numSamples_(numSamples),
		  fixedSampleLocations_(fixedSampleLocations) {
	samplerType_ = "sampler2DMSArray";
	texBind_.target_ = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
	set_numSamples(numSamples);
}

