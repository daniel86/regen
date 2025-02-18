/*
 * texture.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <sstream>

#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/render-state.h>

using namespace regen;

#include "texture.h"

static inline void Regen_TextureFilter(GLenum target, const TextureFilter &v) {
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, v.x);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, v.y);
}

static inline void Regen_TextureLoD(GLenum target, const TextureLoD &v) {
	glTexParameterf(target, GL_TEXTURE_MIN_LOD, v.x);
	glTexParameterf(target, GL_TEXTURE_MAX_LOD, v.y);
}

static inline void Regen_TextureSwizzle(GLenum target, const TextureSwizzle &v) {
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, v.x);
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, v.y);
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, v.z);
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, v.w);
}

static inline void Regen_TextureWrapping(GLenum target, const TextureWrapping &v) {
	glTexParameteri(target, GL_TEXTURE_WRAP_S, v.x);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, v.y);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, v.z);
}

static inline void Regen_TextureCompare(GLenum target, const TextureCompare &v) {
	glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, v.x);
	glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, v.y);
}

static inline void Regen_TextureMaxLevel(GLenum target, const TextureMaxLevel &v) {
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, v);
}

static inline void Regen_TextureAniso(GLenum target, const TextureAniso &v) {
	glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, v);
}

Texture::Texture(GLuint numTextures)
		: GLRectangle(glGenTextures, glDeleteTextures, numTextures),
		  ShaderInput1i(REGEN_STRING("textureChannel" << id())),
		  dim_(2),
		  format_(GL_RGBA),
		  internalFormat_(GL_RGBA8),
		  pixelType_(GL_BYTE),
		  border_(0),
		  texBind_(GL_TEXTURE_2D, 0),
		  numSamples_(1),
		  textureData_(nullptr),
		  isTextureDataOwned_(false) {
	filter_ = new TextureParameterStack<TextureFilter> *[numObjects_];
	lod_ = new TextureParameterStack<TextureLoD> *[numObjects_];
	swizzle_ = new TextureParameterStack<TextureSwizzle> *[numObjects_];
	wrapping_ = new TextureParameterStack<TextureWrapping> *[numObjects_];
	compare_ = new TextureParameterStack<TextureCompare> *[numObjects_];
	maxLevel_ = new TextureParameterStack<TextureMaxLevel> *[numObjects_];
	aniso_ = new TextureParameterStack<TextureAniso> *[numObjects_];
	for (GLuint i = 0; i < numObjects_; ++i) {
		filter_[i] = new TextureParameterStack<TextureFilter>(texBind_, Regen_TextureFilter);
		lod_[i] = new TextureParameterStack<TextureLoD>(texBind_, Regen_TextureLoD);
		swizzle_[i] = new TextureParameterStack<TextureSwizzle>(texBind_, Regen_TextureSwizzle);
		wrapping_[i] = new TextureParameterStack<TextureWrapping>(texBind_, Regen_TextureWrapping);
		compare_[i] = new TextureParameterStack<TextureCompare>(texBind_, Regen_TextureCompare);
		maxLevel_[i] = new TextureParameterStack<TextureMaxLevel>(texBind_, Regen_TextureMaxLevel);
		aniso_[i] = new TextureParameterStack<TextureAniso>(texBind_, Regen_TextureAniso);
	}

	set_rectangleSize(2, 2);
	samplerType_ = "sampler2D";
	setUniformData(-1);
}

Texture::~Texture() {
	for (GLuint i = 0; i < numObjects_; ++i) {
		delete filter_[i];
		delete lod_[i];
		delete swizzle_[i];
		delete wrapping_[i];
		delete compare_[i];
		delete maxLevel_[i];
		delete aniso_[i];
	}
	delete[]filter_;
	delete[]lod_;
	delete[]swizzle_;
	delete[]wrapping_;
	delete[]compare_;
	delete[]maxLevel_;
	delete[]aniso_;

	if (isTextureDataOwned_ && textureData_) {
		delete[]textureData_;
		textureData_ = nullptr;
	}
}

GLint Texture::channel() const { return getVertex(0).r; }

GLenum Texture::targetType() const { return texBind_.target_; }

void Texture::set_targetType(GLenum targetType) { texBind_.target_ = targetType; }

const TextureBind &Texture::textureBind() {
	texBind_.id_ = id();
	return texBind_;
}

void Texture::set_textureData(GLubyte *textureData, bool owned) {
	if (textureData_ && isTextureDataOwned_) {
		delete[]textureData_;
	}
	textureData_ = textureData;
	isTextureDataOwned_ = owned;
}

void Texture::readTextureData() {
	ScopedTextureActivation sta(*this, RenderState::get());
	auto* pixels = new GLubyte[numTexel()*glenum::pixelComponents(format())];
	glGetTexImage(targetType(), 0, format(), GL_UNSIGNED_BYTE, pixels);
	set_textureData(pixels, true);
}

void Texture::ensureTextureData() {
	if (!textureData_) {
		readTextureData();
	}
}

void Texture::setupMipmaps(GLenum mode) const {
	// glGenerateMipmap was introduced in opengl3.0
	// before glBuildMipmaps or GL_GENERATE_MIPMAP was used, but these are not supported here.
	glGenerateMipmap(texBind_.target_);
}

void Texture::begin(RenderState *rs, GLint x) {
	set_active(GL_TRUE);
	setVertex(0, x);
	rs->activeTexture().push(GL_TEXTURE0 + x);
	rs->textures().push(x, textureBind());
}

void Texture::end(RenderState *rs, GLint x) {
	rs->textures().pop(x);
	rs->activeTexture().pop();
	setVertex(0, -1);
	// INVALID_VALUE is generated when texture uniform is enabled
	// with channel=-1. This flag should avoid calls to glUniform
	// for this texture.
	set_active(GL_FALSE);
}

Bounds<Vec2ui> Texture::getRegion(const Vec2f &texco, const Vec2f &regionTS) const {
	auto w = static_cast<float>(width());
	auto h = static_cast<float>(height());
	auto startX = static_cast<unsigned int>(std::floor(texco.x * w));
	auto startY = static_cast<unsigned int>(std::floor(texco.y * h));
	auto endX = static_cast<unsigned int>(std::ceil((texco.x + regionTS.x) * w));
	auto endY = static_cast<unsigned int>(std::ceil((texco.y + regionTS.y) * h));
	if (endX >= width()) endX = width()-1;
	if (endY >= height()) endY = height()-1;
	return {
		Vec2ui(startX, startY),
		Vec2ui(endX, endY)};
}

unsigned int Texture::texelIndex(const Vec2f &texco) const {
	auto x = static_cast<unsigned int>(std::round(texco.x * static_cast<float>(width())));
	auto y = static_cast<unsigned int>(std::round(texco.y * static_cast<float>(height())));
	// clamp to texture size
	switch(wrapping_[objectIndex_]->value().x) {
		case GL_REPEAT:
			x = x % width();
			y = y % height();
			break;
		case GL_MIRRORED_REPEAT:
			x = x % (2 * width());
			y = y % (2 * height());
			if (x >= width()) x = 2 * width() - x - 1;
			if (y >= height()) y = 2 * height() - y - 1;
			break;
		default: // GL_CLAMP_TO_EDGE:
			if (x >= width()) x = width()-1;
			if (y >= height()) y = height()-1;
			break;
	}
	return (y * width() + x);
}

///////////////

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

TextureCube::TextureCube(GLuint numTextures)
		: Texture2D(numTextures),
		  cubeData_() {
	samplerType_ = "samplerCube";
	texBind_.target_ = GL_TEXTURE_CUBE_MAP;
	dim_ = 3;
	for (int i = 0; i < 6; ++i) { cubeData_[i] = NULL; }
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

Texture2DArray::Texture2DArray(GLuint numTextures) : Texture3D(numTextures) {
	samplerType_ = "sampler2DArray";
	texBind_.target_ = GL_TEXTURE_2D_ARRAY;
}

///////////////
///////////////
///////////////

TextureBuffer::TextureBuffer(GLenum texelFormat)
		: Texture() {
	texBind_.target_ = GL_TEXTURE_BUFFER;
	samplerType_ = "samplerBuffer";
	texelFormat_ = texelFormat;
}

void TextureBuffer::attach(const ref_ptr<VBO> &vbo, VBOReference &ref) {
	attachedVBO_ = vbo;
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
	attachedVBO_ = ref_ptr<VBO>();
	attachedVBORef_ = VBOReference();
	glTexBuffer(texBind_.target_, texelFormat_, storage);
	GL_ERROR_LOG();
}

void TextureBuffer::attach(GLuint storage, GLuint offset, GLuint size) {
	attachedVBO_ = ref_ptr<VBO>();
	attachedVBORef_ = VBOReference();
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
