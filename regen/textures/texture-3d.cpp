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

void Texture3D::updateTextureStorage() {
/**
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
				 **/

	auto w = static_cast<int32_t>(width());
	auto h = static_cast<int32_t>(height());
	auto d = static_cast<int32_t>(depth());
	numTexel_ = w*h*d;

	int32_t maxNumLevels = 1 + (int)floor(log2(std::max({w, h})));
	int32_t levels = maxNumLevels; // FIXME

	glTextureStorage3D(id(),
					   levels,
					   internalFormat_,
					   w,
					   h,
					   d);

	if (textureData_ != nullptr) {
		for (int32_t layerIdx=0; layerIdx<d; layerIdx++) {
			glTextureSubImage3D(id(),
								0, // mipmap level
								0, // x offset
								0, // y offset
								layerIdx, // z offset
								w,
								h,
								1,
								format_,
								pixelType_,
								textureData_ + XXX);
		}
	}

	//if (levels > 1) {
	//	glGenerateTextureMipmap(id());
	//}
}

void Texture3D::texSubImage(GLint layer, GLubyte *subData) const {
	glTextureSubImage3D(
			texBind_.target_,
			0,
			0,
			0, // offset
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
