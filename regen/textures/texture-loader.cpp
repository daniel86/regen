/*
 * texture-loader.cpp
 *
 *  Created on: 21.12.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <IL/il.h>
#include <IL/ilu.h>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

#include <cstring>
#include <fstream>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/external/spectrum.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/render-state.h>
#include <regen/textures/texture-1d.h>

#include "texture-loader.h"

using namespace regen;
using namespace regen::textures;

static void scaleImage(GLuint w, GLuint h, GLuint d) {
	GLuint width_ = ilGetInteger(IL_IMAGE_WIDTH);
	// scale image to desired size
	if (w > 0 && h > 0) {
		if (width_ > w) {
			// use bilinear filtering for down scaling
			iluImageParameter(ILU_FILTER, ILU_BILINEAR);
		} else {
			// use triangle filtering for up scaling
			iluImageParameter(ILU_FILTER, ILU_SCALE_TRIANGLE);
		}
		iluScale(w, h, d);
	}
}

GLenum regenImageFormat1() {
	GLenum format = ilGetInteger(IL_IMAGE_FORMAT);
	switch (format) {
		case GL_COLOR_INDEX:
			return GL_RGBA;
		default:
			return format;
	}
}

GLenum regenImageFormat() {
	GLenum format = ilGetInteger(IL_IMAGE_FORMAT);
	switch (format) {
		// handle deprecated formats
		case GL_LUMINANCE:
			return GL_RED;
		case GL_LUMINANCE_ALPHA:
			return GL_RG;
		case GL_COLOR_INDEX:
			return GL_RGBA;
		default:
			return format;
	}
}

static void convertImage(GLenum format, GLenum type) {
	auto srcFormat = regenImageFormat1();
	auto srcType = ilGetInteger(IL_IMAGE_TYPE);
	auto dstFormat = (format == GL_NONE ? srcFormat : format);
	auto dstType = (type == GL_NONE ? srcType : type);
	if (ilGetInteger(IL_IMAGE_FORMAT) != dstFormat || srcType != dstType) {
		if (ilConvertImage(dstFormat, dstType) == IL_FALSE) {
			throw Error("ilConvertImage failed");
		}
	}
}

GLuint textures::loadImage(const std::string &file) {
	static GLboolean devilInitialized_ = GL_FALSE;
	if (!devilInitialized_) {
		ilInit();
		devilInitialized_ = GL_TRUE;
	}

	if (!boost::filesystem::exists(file)) {
		throw Error(REGEN_STRING(
							"Unable to open image file at '" << file << "'."));
	}

	GLuint ilID;
	ilGenImages(1, &ilID);
	ilBindImage(ilID);
	if (ilLoadImage(file.c_str()) == IL_FALSE) {
		throw Error("ilLoadImage failed");
	}

	REGEN_DEBUG("Texture '" << file << "'" <<
							" format=" << regenImageFormat() <<
							" type=" << ilGetInteger(IL_IMAGE_TYPE) <<
							" bpp=" << ilGetInteger(IL_IMAGE_BPP) <<
							" channels=" << ilGetInteger(IL_IMAGE_CHANNELS) <<
							" images=" << ilGetInteger(IL_NUM_IMAGES) <<
							" width=" << ilGetInteger(IL_IMAGE_WIDTH) <<
							" height=" << ilGetInteger(IL_IMAGE_HEIGHT));

	return ilID;
}

void unsetData(GLuint ilID, const ref_ptr<Texture> &tex, bool keepData) {
	if (keepData) {
		// not sure how one could tell IL we take ownership of the data, so need to memcpy it.
        // make sure the data tpe is unsigned byte.
        // TODO: better to support other types than unsigned byte
        if (ilConvertImage(ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE) == IL_FALSE) {
            throw std::runtime_error("Failed to decompress image");
        }
		auto numBytes =
			ilGetInteger(IL_IMAGE_WIDTH) *
			ilGetInteger(IL_IMAGE_HEIGHT) *
			ilGetInteger(IL_IMAGE_BPP);
		auto *data = new GLubyte[numBytes];
		memcpy(data, tex->textureData(), numBytes);
		tex->set_textureData(data, true);
	}
	else {
		tex->set_textureData(nullptr);
	}
	ilDeleteImages(1, &ilID);
}

ref_ptr<Texture> textures::load(
		const std::string &file,
		GLenum mipmapFlag,
		GLenum forcedInternalFormat,
		GLenum forcedFormat,
		const Vec3ui &forcedSize,
		bool keepData) {
	auto ilID = loadImage(file);
	scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);
	convertImage(forcedFormat, GL_NONE);
	auto depth = ilGetInteger(IL_IMAGE_DEPTH);
	auto numImages = ilGetInteger(IL_NUM_IMAGES);

	ref_ptr<Texture> tex;
	if (depth > 1) {
		ref_ptr<Texture3D> tex3D = ref_ptr<Texture3D>::alloc();
		tex3D->set_depth(depth);
		tex = tex3D;
		numImages = 1;
	} else if (numImages > 1) {
		ref_ptr<Texture2DArray> tex2DArray = ref_ptr<Texture2DArray>::alloc();
		tex2DArray->set_depth(numImages);
		tex = tex2DArray;
	} else {
		tex = ref_ptr<Texture2D>::alloc();
	}
	tex->set_rectangleSize(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
	tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
	tex->set_format(regenImageFormat());
	if (forcedInternalFormat == GL_NONE) {
		forcedInternalFormat = tex->format();
	}
	if (forcedInternalFormat == GL_BGRA) {
		forcedInternalFormat = GL_RGBA;
	} else if (forcedInternalFormat == GL_BGR) {
		forcedInternalFormat = GL_RGB;
	}
	tex->set_internalFormat(forcedInternalFormat);
	if (numImages < 2) {
		tex->set_textureData((GLubyte *) ilGetData(), false);
	}
	tex->begin(RenderState::get());
	tex->texImage();
	if (numImages > 1) {
		auto *tex3d = dynamic_cast<Texture3D *>(tex.get());
		for (auto i = 0; i < numImages; ++i) {
			ilBindImage(ilID);
			ilActiveImage(i);
			tex3d->texSubImage(i, (GLubyte *) ilGetData());
		}
	}
	if (mipmapFlag != GL_NONE) {
		tex->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
		tex->setupMipmaps(mipmapFlag);
	} else {
		tex->filter().push(GL_LINEAR);
	}
	tex->wrapping().push(GL_REPEAT);
	tex->end(RenderState::get());
	unsetData(ilID, tex, keepData);
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<Texture> textures::load(
		GLuint textureType,
		GLuint numBytes,
		const void *rawData,
		GLenum mipmapFlag,
		GLenum forcedInternalFormat,
		GLenum forcedFormat,
		const Vec3ui &forcedSize) {
	GLuint ilID;
	ilGenImages(1, &ilID);
	ilBindImage(ilID);
	if (ilLoadL(textureType, rawData, numBytes) == IL_FALSE) {
		throw Error("ilLoadL failed");
	}

	scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);
	convertImage(forcedFormat, GL_NONE);
	GLint depth = ilGetInteger(IL_IMAGE_DEPTH);

	ref_ptr<Texture> tex;
	if (depth > 1) {
		ref_ptr<Texture3D> tex3D = ref_ptr<Texture3D>::alloc();
		tex3D->set_depth(depth);
		tex = tex3D;
	} else {
		tex = ref_ptr<Texture2D>::alloc();
	}
	tex->set_rectangleSize(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
	tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
	tex->set_format(regenImageFormat());
	tex->set_internalFormat(
			forcedInternalFormat == GL_NONE ? tex->format() : forcedInternalFormat);
	tex->set_textureData((GLubyte *) ilGetData());
	tex->begin(RenderState::get());
	tex->texImage();
	if (mipmapFlag != GL_NONE) {
		tex->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
		tex->setupMipmaps(mipmapFlag);
	} else {
		tex->filter().push(GL_LINEAR);
	}
	tex->wrapping().push(GL_REPEAT);
	tex->end(RenderState::get());
	tex->set_textureData(nullptr);

	ilDeleteImages(1, &ilID);
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<Texture2DArray> textures::loadArray(
		const std::string &textureDirectory,
		const std::string &textureNamePattern,
		GLenum mipmapFlag,
		GLenum forcedInternalFormat,
		GLenum forcedFormat,
		const Vec3ui &forcedSize) {
	GLuint numTextures = 0;
	std::list<std::string> textureFiles;

	boost::filesystem::path texturedir(textureDirectory);
	boost::regex pattern(textureNamePattern);

	std::vector<std::string> accumulator;
	boost::filesystem::directory_iterator it(texturedir), eod;
	BOOST_FOREACH(const boost::filesystem::path &filePath, std::make_pair(it, eod)) {
					std::string name = filePath.filename().string();
					if (boost::regex_match(name, pattern)) {
						accumulator.push_back(filePath.string());
						numTextures += 1;
					}
				}
	std::sort(accumulator.begin(), accumulator.end());

	ref_ptr<Texture2DArray> tex = ref_ptr<Texture2DArray>::alloc();
	tex->set_depth(numTextures);
	tex->begin(RenderState::get());

	GLint arrayIndex = 0;
	for (auto & textureFile : accumulator) {
		auto ilID = loadImage(textureFile);
		scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);
		convertImage(forcedFormat, GL_NONE);

		if (arrayIndex == 0) {
			tex->set_rectangleSize(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
			tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
			tex->set_format(regenImageFormat());
			tex->set_internalFormat(
					forcedInternalFormat == GL_NONE ? tex->format() : forcedInternalFormat);
			tex->set_textureData(nullptr);
			tex->texImage();
		}

		tex->texSubImage(arrayIndex, (GLubyte *) ilGetData());
		ilDeleteImages(1, &ilID);
		arrayIndex += 1;
	}

	if (mipmapFlag != GL_NONE) {
		tex->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
		tex->setupMipmaps(mipmapFlag);
	} else {
		tex->filter().push(GL_LINEAR);
	}
	tex->wrapping().push(GL_REPEAT);

	tex->end(RenderState::get());
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<TextureCube> textures::loadCube(
		const std::string &file,
		GLboolean flipBackFace,
		GLenum mipmapFlag,
		GLenum forcedInternalFormat,
		GLenum forcedFormat,
		const Vec3ui &forcedSize) {
	auto ilID = loadImage(file);
	scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);
	convertImage(forcedFormat, GL_NONE);

	GLint faceWidth, faceHeight;
	auto width = ilGetInteger(IL_IMAGE_WIDTH);
	auto height = ilGetInteger(IL_IMAGE_HEIGHT);
	GLint faces[12];
	// guess layout
	if (width > height) {
		faceWidth = width / 4;
		faceHeight = height / 3;
		GLint faces_[12] = {
				-1, TextureCube::TOP, -1, -1,
				TextureCube::RIGHT, TextureCube::FRONT, TextureCube::LEFT, TextureCube::BACK,
				-1, TextureCube::BOTTOM, -1, -1
		};
		for (ILint i = 0; i < 12; ++i) faces[i] = faces_[i];
	} else {
		faceWidth = width / 3;
		faceHeight = height / 4;
		GLint faces_[12] = {
				-1, TextureCube::TOP, -1,
				TextureCube::RIGHT, TextureCube::FRONT, TextureCube::LEFT,
				-1, TextureCube::BOTTOM, -1,
				-1, TextureCube::BACK, -1
		};
		for (ILint i = 0; i < 12; ++i) faces[i] = faces_[i];
	}
	const auto numRows = height / faceHeight;
	const auto numCols = width / faceWidth;
	const auto bpp = ilGetInteger(IL_IMAGE_BPP);
	const auto faceBytes = bpp * faceWidth * faceHeight;
	const auto rowBytes = faceBytes * numCols;

	auto tex = ref_ptr<TextureCube>::alloc();
	tex->begin(RenderState::get());
	tex->set_rectangleSize(faceWidth, faceHeight);
	tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
	tex->set_format(regenImageFormat());
	tex->set_internalFormat(
			forcedInternalFormat == GL_NONE ? tex->format() : forcedInternalFormat);

	auto *imageData = (GLbyte *) ilGetData();
	ILint index = 0;
	for (ILint row = 0; row < numRows; ++row) {
		GLbyte *colData = imageData;
		for (ILint col = 0; col < numCols; ++col) {
			ILint mappedFace = faces[index];
			if (mappedFace != -1) {
				tex->set_data((TextureCube::CubeSide) mappedFace, colData);
			}
			index += 1;
			colData += bpp * faceWidth;
		}
		imageData += rowBytes;
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, faceWidth * numCols);
	tex->cubeTexImage(TextureCube::LEFT);
	tex->cubeTexImage(TextureCube::RIGHT);
	tex->cubeTexImage(TextureCube::TOP);
	tex->cubeTexImage(TextureCube::BOTTOM);
	tex->cubeTexImage(TextureCube::FRONT);
	if (flipBackFace) {
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		auto *flippedFace = new GLbyte[faceBytes];
		auto *faceData = (GLbyte *) tex->cubeData()[TextureCube::BACK];
		auto *dst = flippedFace;

		for (ILint row = faceWidth - 1; row >= 0; --row) {
			auto *rowData = faceData + row * faceWidth * numCols * bpp;
			for (ILint col = faceHeight - 1; col >= 0; --col) {
				GLbyte *pixelData = rowData + col * bpp;
				memcpy(dst, pixelData, bpp);
				dst += bpp;
			}
		}

		tex->set_data(TextureCube::BACK, flippedFace);
		tex->cubeTexImage(TextureCube::BACK);
		delete[] flippedFace;
	} else {
		tex->cubeTexImage(TextureCube::BACK);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 4);
	}
	if (mipmapFlag != GL_NONE) {
		tex->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
		tex->setupMipmaps(mipmapFlag);
	} else {
		tex->filter().push(GL_LINEAR);
	}

	tex->end(RenderState::get());
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	ilDeleteImages(1, &ilID);
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<Texture> textures::loadRAW(
		const std::string &path,
		const Vec3ui &size,
		GLuint numComponents,
		GLuint bytesPerComponent) {
	std::ifstream f(path.c_str(),
					std::ios::in
					| std::ios::binary
					| std::ios::ate // start at end position
	);
	if (!f.is_open()) {
		throw Error(REGEN_STRING(
							"Unable to open data set file at '" << path << "'."));
	}

	auto numBytes = size.x * size.y * size.z * numComponents;
	auto *pixels = new char[numBytes];
	f.seekg(0, std::ios::beg);
	f.read(pixels, numBytes);
	f.close();

	auto format_ = glenum::textureFormat(numComponents);
	auto internalFormat_ = glenum::textureInternalFormat(GL_UNSIGNED_BYTE, numComponents, bytesPerComponent);

	ref_ptr<Texture> tex;
	if (size.z > 1) {
		ref_ptr<Texture3D> tex3D = ref_ptr<Texture3D>::alloc();
		tex3D->set_depth(size.z);
		tex = tex3D;
	} else {
		tex = ref_ptr<Texture2D>::alloc();
	}

	tex->begin(RenderState::get());
	tex->set_rectangleSize(size.x, size.y);
	tex->set_pixelType(GL_UNSIGNED_BYTE);
	tex->set_format(format_);
	tex->set_internalFormat(internalFormat_);
	tex->set_textureData((GLubyte *) pixels);
	tex->filter().push(GL_LINEAR);
	tex->wrapping().push(GL_REPEAT);
	tex->texImage();
	tex->end(RenderState::get());
	tex->set_textureData(nullptr);

	delete[] pixels;

	return tex;
}

ref_ptr<Texture> textures::loadSpectrum(
		GLdouble t1,
		GLdouble t2,
		GLint numTexels,
		GLenum mipmapFlag) {
	auto *data = new unsigned char[numTexels * 4];
	spectrum(t1, t2, numTexels, data);

	ref_ptr<Texture> tex = ref_ptr<Texture1D>::alloc();
	tex->begin(RenderState::get());
	tex->set_rectangleSize(numTexels, 1);
	tex->set_pixelType(GL_UNSIGNED_BYTE);
	tex->set_format(GL_RGBA);
	tex->set_internalFormat(GL_RGBA);
	tex->set_textureData((GLubyte *) data);
	tex->texImage();
	tex->wrapping().push(GL_CLAMP);
	if (mipmapFlag == GL_NONE) {
		tex->filter().push(GL_LINEAR);
	} else {
		tex->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
		tex->setupMipmaps(mipmapFlag);
	}
	tex->set_textureData(nullptr);
	delete[]data;
	tex->end(RenderState::get());

	return tex;
}

