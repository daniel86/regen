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
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/render-state.h>

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
	return (format == GL_COLOR_INDEX ? GL_RGBA : format);
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
		case GL_BGR:
			return GL_RGB;
		case GL_BGRA:
			return GL_RGBA;
		default:
			return format;
	}
}

static GLenum convertImage(GLenum format, GLenum type) {
	auto srcFormat = regenImageFormat1();
	auto srcType = static_cast<uint32_t>(ilGetInteger(IL_IMAGE_TYPE));
	auto origSrcFormat = static_cast<uint32_t>(ilGetInteger(IL_IMAGE_FORMAT));
	auto dstFormat = (format == GL_NONE ? srcFormat : format);
	auto dstType = (type == GL_NONE ? srcType : type);
	if (origSrcFormat != dstFormat || srcType != dstType) {
		if (ilConvertImage(dstFormat, dstType) == IL_FALSE) {
			throw Error("ilConvertImage failed");
		}
	}
	return dstFormat;
}

uint32_t textures::loadImage(std::string_view file) {
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
	if (ilLoadImage(file.data()) == IL_FALSE) {
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
		memcpy(data, (GLubyte *) ilGetData(), numBytes);
		tex->setTextureData(data, true);
	}
	ilDeleteImages(1, &ilID);
}

void textures::reload(const ref_ptr<Texture> &tex, std::string_view file) {
	bool keepData = false;
	unsigned int textureDepth = 1;
	if (tex->textureBind().target_ == GL_TEXTURE_3D) {
		auto tex3D = dynamic_cast<Texture3D *>(tex.get());
		if (tex3D) {
			textureDepth = tex3D->depth();
		}
	}
	else if (tex->textureBind().target_ == GL_TEXTURE_2D_ARRAY) {
		auto tex2DArray = dynamic_cast<Texture2DArray *>(tex.get());
		if (tex2DArray) {
			textureDepth = tex2DArray->depth();
		}
	}
	else if (tex->textureBind().target_ == GL_TEXTURE_CUBE_MAP) {
		textureDepth = 6;
	}

	auto ilID = loadImage(file);
	scaleImage(tex->width(), tex->height(), textureDepth);
	convertImage(tex->format(), GL_NONE);
	//auto depth = ilGetInteger(IL_IMAGE_DEPTH);
	auto numImages = ilGetInteger(IL_NUM_IMAGES);

	tex->set_textureFile(file);
	tex->set_rectangleSize(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
	tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
	tex->allocTexture();
	if (numImages > 1) {
		auto *tex3d = dynamic_cast<Texture3D *>(tex.get());
		for (auto i = 0; i < numImages; ++i) {
			ilBindImage(ilID);
			ilActiveImage(i);
			tex3d->updateSubImage(i, (GLubyte *) ilGetData());
		}
	}
	else {
		tex->updateImage((GLubyte *) ilGetData());
	}
	if (tex->getNumMipmaps() > 1) {
		tex->updateMipmaps();
	}
	unsetData(ilID, tex, keepData);
}

ref_ptr<Texture> textures::load(std::string_view file, const TextureConfig &texCfg) {
	auto ilID = loadImage(file);
	scaleImage(texCfg.forcedSize.x, texCfg.forcedSize.y, texCfg.forcedSize.z);
	convertImage(texCfg.forcedFormat, GL_NONE);
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
	tex->set_textureFile(file);
	tex->set_rectangleSize(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
	tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
	tex->set_format(regenImageFormat());
	if (texCfg.forcedInternalFormat == GL_NONE) {
		tex->set_internalFormat(glenum::textureInternalFormat(tex->format()));
	} else if (texCfg.forcedInternalFormat == GL_BGRA) {
		tex->set_internalFormat(GL_RGBA8);
	} else if (texCfg.forcedInternalFormat == GL_BGR) {
		tex->set_internalFormat(GL_RGB8);
	} else {
		tex->set_internalFormat(texCfg.forcedInternalFormat);
	}
	tex->allocTexture();
	tex->set_wrapping(GL_REPEAT);
	tex->set_filter(GL_LINEAR);
	if (numImages > 1) {
		auto *tex3d = dynamic_cast<Texture3D *>(tex.get());
		for (auto i = 0; i < numImages; ++i) {
			ilBindImage(ilID);
			ilActiveImage(i);
			tex3d->updateSubImage(i, (GLubyte *) ilGetData());
		}
	} else {
		tex->updateImage((GLubyte *) ilGetData());
	}
	if (texCfg.useMipmaps) {
		tex->updateMipmaps();
	}
	unsetData(ilID, tex, texCfg.keepData);
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<Texture> textures::load(
		GLuint textureType,
		GLuint numBytes,
		const void *rawData,
		const TextureConfig &texCfg) {
	GLuint ilID;
	ilGenImages(1, &ilID);
	ilBindImage(ilID);
	if (ilLoadL(textureType, rawData, numBytes) == IL_FALSE) {
		throw Error("ilLoadL failed");
	}

	scaleImage(texCfg.forcedSize.x, texCfg.forcedSize.y, texCfg.forcedSize.z);
	convertImage(texCfg.forcedFormat, GL_NONE);
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
	tex->set_internalFormat(texCfg.forcedInternalFormat == GL_NONE ?
		glenum::textureInternalFormat(tex->format()) :
		texCfg.forcedInternalFormat);
	tex->allocTexture();
	tex->set_wrapping(GL_REPEAT);
	tex->set_filter(GL_LINEAR);
	tex->updateImage((GLubyte *) ilGetData());
	if (texCfg.useMipmaps) {
		tex->updateMipmaps();
	}
	ilDeleteImages(1, &ilID);
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<Texture2DArray> textures::loadArray(
		const std::string &textureDirectory,
		const std::string &textureNamePattern,
		const TextureConfig &texCfg) {
	boost::filesystem::path dir(textureDirectory);
	boost::regex pattern(textureNamePattern);

	std::vector<TextureDescription> accumulator;
	boost::filesystem::directory_iterator it(dir), eod;
	BOOST_FOREACH(const boost::filesystem::path &filePath, std::make_pair(it, eod)) {
					std::string name = filePath.filename().string();
					if (boost::regex_match(name, pattern)) {
						accumulator.emplace_back(filePath.string());
					}
				}
	std::sort(accumulator.begin(), accumulator.end());

	return loadArray(accumulator, texCfg);
}

ref_ptr<Texture2DArray> textures::loadArray(
		const std::vector<TextureDescription> &textureFiles,
		const TextureConfig &texCfg) {
	GLuint numTextures = textureFiles.size();
	ref_ptr<Texture2DArray> tex = ref_ptr<Texture2DArray>::alloc();
	tex->set_depth(numTextures);

	Vec3ui forcedSize_ = texCfg.forcedSize;
	GLenum forcedFormat = texCfg.forcedFormat;
	int arrayIndex = 0;
	uint32_t ilID;
	for (auto & descr : textureFiles) {
		if (descr.isFile()) {
			ilID = loadImage(descr.getFileName());
			scaleImage(texCfg.forcedSize.x, texCfg.forcedSize.y, 1);
			forcedFormat = convertImage(forcedFormat, GL_NONE);
		}

		if (arrayIndex == 0) {
			forcedSize_.x = ilGetInteger(IL_IMAGE_WIDTH);
			forcedSize_.y = ilGetInteger(IL_IMAGE_HEIGHT);
			tex->set_rectangleSize(forcedSize_.x, forcedSize_.y);
			tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
			tex->set_format(regenImageFormat());
			tex->set_internalFormat(texCfg.forcedInternalFormat == GL_NONE ?
				glenum::textureInternalFormat(tex->format()) :
				texCfg.forcedInternalFormat);
			tex->allocTexture();
		}

		if (descr.isFile()) {
			tex->updateSubImage(arrayIndex, (GLubyte *) ilGetData());
			ilDeleteImages(1, &ilID);
		} else if (descr.isClear()) {
			glClearTexSubImage(tex->textureBind().id_,
				0, 0, 0, arrayIndex, // level, x, y, z offset
				static_cast<int>(forcedSize_.x),
				static_cast<int>(forcedSize_.y), 1, // width, height, depth
				tex->format(), tex->pixelType(),
				descr.getClearData());
		}
		arrayIndex += 1;
	}
	tex->set_wrapping(GL_REPEAT);
	tex->set_filter(GL_LINEAR);
	if (texCfg.useMipmaps) {
		tex->updateMipmaps();
	}
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<TextureCube> textures::loadCube(
		std::string_view file,
		bool flipBackFace,
		const TextureConfig &texCfg) {
	auto ilID = loadImage(file);
	scaleImage(texCfg.forcedSize.x, texCfg.forcedSize.y, texCfg.forcedSize.z);
	convertImage(texCfg.forcedFormat, GL_NONE);

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
				TextureCube::LEFT, TextureCube::FRONT, TextureCube::RIGHT, TextureCube::BACK,
				-1, TextureCube::BOTTOM, -1, -1
		};
		for (ILint i = 0; i < 12; ++i) faces[i] = faces_[i];
	} else {
		faceWidth = width / 3;
		faceHeight = height / 4;
		GLint faces_[12] = {
				-1, TextureCube::TOP, -1,
				TextureCube::LEFT, TextureCube::FRONT, TextureCube::RIGHT,
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
	tex->set_textureFile(file);
	tex->set_rectangleSize(faceWidth, faceHeight);
	tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
	tex->set_format(regenImageFormat());
	tex->set_internalFormat(texCfg.forcedInternalFormat == GL_NONE ?
		glenum::textureInternalFormat(tex->format()) :
		texCfg.forcedInternalFormat);
	tex->allocTexture();

	tex->set_filter(GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, faceWidth * numCols);

	auto *imageData = (GLbyte *) ilGetData();
	ILint index = 0;
	for (ILint row = 0; row < numRows; ++row) {
		GLbyte *colData = imageData;
		for (ILint col = 0; col < numCols; ++col) {
			ILint mappedFace = faces[index];
			if (mappedFace != -1) {
				auto nextFace = (TextureCube::CubeSide) mappedFace;

				if (flipBackFace && nextFace == TextureCube::BACK) {
					glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
					auto *flippedFace = new GLbyte[faceBytes];
					auto *faceData = (GLbyte *) colData;
					auto *dst = flippedFace;
					for (ILint row = faceWidth - 1; row >= 0; --row) {
						auto *rowData = faceData + row * faceWidth * numCols * bpp;
						for (ILint col = faceHeight - 1; col >= 0; --col) {
							GLbyte *pixelData = rowData + col * bpp;
							memcpy(dst, pixelData, bpp);
							dst += bpp;
						}
					}
					tex->updateSubImage(nextFace, (GLubyte *) flippedFace);
					delete[] flippedFace;
					glPixelStorei(GL_UNPACK_ROW_LENGTH, faceWidth * numCols);
				} else {
					tex->updateSubImage(nextFace, (GLubyte *) colData);
				}
			}
			index += 1;
			colData += bpp * faceWidth;
		}
		imageData += rowBytes;
	}

	if (texCfg.useMipmaps) {
		tex->updateMipmaps();
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	ilDeleteImages(1, &ilID);

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
	tex->set_rectangleSize(size.x, size.y);
	tex->set_pixelType(GL_UNSIGNED_BYTE);
	tex->set_format(format_);
	tex->set_internalFormat(internalFormat_);
	tex->allocTexture();
	tex->set_filter(GL_LINEAR);
	tex->set_wrapping(GL_REPEAT);
	tex->updateImage((GLubyte *) pixels);
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
	tex->set_rectangleSize(numTexels, 1);
	tex->set_pixelType(GL_UNSIGNED_BYTE);
	tex->set_format(GL_RGBA);
	tex->set_internalFormat(GL_RGBA8);
	tex->allocTexture();
	tex->set_wrapping(GL_CLAMP_TO_EDGE);
	tex->set_filter(GL_LINEAR);
	tex->updateImage((GLubyte *) data);
	if (mipmapFlag != GL_NONE) {
		tex->updateMipmaps();
	}
	delete[]data;

	return tex;
}

