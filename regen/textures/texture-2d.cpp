#include "texture-2d.h"

using namespace regen;

Texture2D::Texture2D(GLuint numTextures)
		: Texture(numTextures) {
	dim_ = 2;
	texBind_.target_ = GL_TEXTURE_2D;
	samplerType_ = "sampler2D";
}

void Texture2D::texImage() const {
	glTexImage2D(texBind_.target_,
				 0, // mipmap level
				 internalFormat_,
				 width(),
				 height(),
				 border_,
				 format_,
				 pixelType_,
				 textureData_);
}

TextureMips2D::TextureMips2D(GLuint numMips) : Texture2D(), numMips_(numMips) {
	mipTextures_.resize(numMips);
	mipRefs_.resize(numMips-1);

	mipTextures_[0] = this;
	for (auto i = 1u; i < numMips; ++i) {
		mipRefs_[i-1] = ref_ptr<Texture2D>::alloc();
		mipTextures_[i] = mipRefs_[i-1].get();
	}
}

TextureRectangle::TextureRectangle(GLuint numTextures)
		: Texture2D(numTextures) {
	texBind_.target_ = GL_TEXTURE_RECTANGLE;
	samplerType_ = "sampler2DRect";
}

Texture2DDepth::Texture2DDepth(GLuint numTextures)
		: Texture2D(numTextures) {
	format_ = GL_DEPTH_COMPONENT;
	internalFormat_ = GL_DEPTH_COMPONENT;
	pixelType_ = GL_UNSIGNED_BYTE;
}

Texture2DMultisample::Texture2DMultisample(
		GLsizei numSamples,
		GLuint numTextures,
		GLboolean fixedSampleLaocations)
		: Texture2D(numTextures) {
	texBind_.target_ = GL_TEXTURE_2D_MULTISAMPLE;
	fixedsamplelocations_ = fixedSampleLaocations;
	samplerType_ = "sampler2DMS";
	set_numSamples(numSamples);
}

void Texture2DMultisample::texImage() const {
	glTexImage2DMultisample(texBind_.target_,
							numSamples(),
							internalFormat_,
							width(),
							height(),
							fixedsamplelocations_);
}

Texture2DMultisampleDepth::Texture2DMultisampleDepth(
		GLsizei numSamples,
		GLboolean fixedSampleLaocations)
		: Texture2DDepth() {
	texBind_.target_ = GL_TEXTURE_2D_MULTISAMPLE;
	fixedsamplelocations_ = fixedSampleLaocations;
	set_numSamples(numSamples);
}

void Texture2DMultisampleDepth::texImage() const {
	glTexImage2DMultisample(texBind_.target_,
							numSamples(),
							internalFormat_,
							width(),
							height(),
							fixedsamplelocations_);
}
