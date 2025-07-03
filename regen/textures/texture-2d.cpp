#include "texture-2d.h"

using namespace regen;

Texture2D::Texture2D(GLuint numTextures)
		: Texture(numTextures) {
	dim_ = 2;
	texBind_.target_ = GL_TEXTURE_2D;
	samplerType_ = "sampler2D";
}

void Texture2D::updateTextureStorage() {
	auto w = static_cast<int32_t>(width());
	auto h = static_cast<int32_t>(height());
	numTexel_ = w*h;

	int32_t maxNumLevels = 1 + (int)floor(log2(std::max({w, h})));
	int32_t levels = maxNumLevels; // FIXME

	glTextureStorage2D(id(),
					   levels,
					   internalFormat_,
					   w,
					   h);
	glTextureSubImage2D(id(),
						0, // mipmap level
						0, // x offset
						0, // y offset
						w,
						h,
						format_,
						pixelType_,
						textureData_);

	if (levels > 1) {
		glGenerateTextureMipmap(id());
	}
	/**
	glTexImage2D(texBind_.target_,
				 0, // mipmap level
				 internalFormat_,
				 width(),
				 height(),
				 border_,
				 format_,
				 pixelType_,
				 textureData_);
	 */
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
		GLboolean fixedSampleLocations)
		: Texture2D(numTextures) {
	texBind_.target_ = GL_TEXTURE_2D_MULTISAMPLE;
	fixedSampleLocations_ = fixedSampleLocations;
	samplerType_ = "sampler2DMS";
	set_numSamples(numSamples);
}

void Texture2DMultisample::updateTextureStorage() {
	/**
	glTexImage2DMultisample(texBind_.target_,
							numSamples(),
							internalFormat_,
							width(),
							height(),
							fixedSampleLocations_);
	 */
	auto w = static_cast<int32_t>(width());
	auto h = static_cast<int32_t>(height());
	// NOTE: no data can be uploaded from CPU to a multisample texture
	glTextureStorage2DMultisample(
			id(),
			numSamples(),
			internalFormat_,
			w,
			h,
			fixedSampleLocations_);
	numTexel_ = w*h;
}

Texture2DMultisampleDepth::Texture2DMultisampleDepth(
		GLsizei numSamples,
		GLboolean fixedSampleLocations)
		: Texture2DDepth() {
	internalFormat_ = GL_DEPTH_COMPONENT24;
	texBind_.target_ = GL_TEXTURE_2D_MULTISAMPLE;
	fixedsamplelocations_ = fixedSampleLocations;
	set_numSamples(numSamples);
}

void Texture2DMultisampleDepth::updateTextureStorage() {
	/**
	glTexImage2DMultisample(texBind_.target_,
							numSamples(),
							internalFormat_,
							width(),
							height(),
							fixedsamplelocations_);
	**/
	auto w = static_cast<int32_t>(width());
	auto h = static_cast<int32_t>(height());
	// NOTE: no data can be uploaded from CPU to a multisample texture
	glTextureStorage2DMultisample(
			id(),
			numSamples(),
			internalFormat_,
			w,
			h,
			fixedsamplelocations_);
	numTexel_ = w*h;
}
