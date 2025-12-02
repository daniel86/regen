#include <GL/glew.h>
#include <IL/il.h>

#include <filesystem>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

#include <cstring>
#include <fstream>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/external/spectrum.h>
#include <regen/gl/gl-enum.h>
#include <regen/gl/render-state.h>

#include "texture-loader.h"
#include "devil-loader.h"

using namespace regen;
using namespace regen::textures;

GLenum regenImageFormat(GLenum format) {
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

void unsetData(ImageDataArray &imageData, const ref_ptr<Texture> &tex, bool keepData) {
	if (keepData && imageData.size()==1) {
		// NOTE: this is exactly the format of data that was uploaded to GPU.
		tex->setTextureData(imageData[0]);
	}
}

void TextureLoaderRegistry::registerLoader(std::unique_ptr<ITextureLoader> loader) {
	instance().loaders_.emplace_back(std::move(loader));
}

ImageDataArray TextureLoaderRegistry::load(std::string_view file, const TextureConfig& cfg) {
	std::string ext = std::filesystem::path(file).extension().string();
	for (const auto &loader : instance().loaders_) {
		if (loader->canLoad(ext)) {
			try {
				return loader->load(file, cfg);
			} catch (const Error &e) {
				REGEN_WARN("Loader failed for image file '" << file << "': " << e.what());
			}
		}
	}
	throw Error(REGEN_STRING("No loader found for image file '" << file << "'."));
}

ref_ptr<Texture> textures::load(std::string_view file, const TextureConfig &texCfg) {
	ImageDataArray imgData = TextureLoaderRegistry::load(file, texCfg);
	if (imgData.empty()) {
		throw Error("No image data loaded");
	}
	auto &firstImage = imgData[0];
	const uint32_t depth = firstImage->depth;
	uint32_t numImages = imgData.size();

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
	tex->set_rectangleSize(firstImage->width, firstImage->height);
	tex->set_pixelType(firstImage->pixelType);
	tex->set_format(regenImageFormat(firstImage->format));
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
	tex->set_wrapping(TextureWrapping::create(GL_REPEAT));
	tex->set_filter(TextureFilter::create(GL_LINEAR));
	if (numImages > 1) {
		auto *tex3d = dynamic_cast<Texture3D *>(tex.get());
		for (uint32_t i = 0u; i < numImages; ++i) {
			tex3d->updateSubImage(i, imgData[i]->pixels);
		}
	} else {
		tex->updateImage(firstImage->pixels);
	}
	if (texCfg.useMipmaps) {
		tex->updateMipmaps();
	}
	unsetData(imgData, tex, texCfg.keepData);
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<Texture> textures::load(
		uint32_t textureType,
		uint32_t numBytes,
		const void *rawData,
		const TextureConfig &texCfg) {
	uint32_t ilID;
	ilGenImages(1, &ilID);
	ilBindImage(ilID);
	if (ilLoadL(textureType, rawData, numBytes) == IL_FALSE) {
		throw Error("ilLoadL failed");
	}

	DevilLoader::scaleImage(texCfg.forcedSize.x, texCfg.forcedSize.y, texCfg.forcedSize.z);
	DevilLoader::convertImage(texCfg.forcedFormat, GL_NONE);
	int depth = ilGetInteger(IL_IMAGE_DEPTH);

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
	tex->set_format(regenImageFormat(ilGetInteger(IL_IMAGE_FORMAT)));
	tex->set_internalFormat(texCfg.forcedInternalFormat == GL_NONE ?
		glenum::textureInternalFormat(tex->format()) :
		texCfg.forcedInternalFormat);
	tex->allocTexture();
	tex->set_wrapping(TextureWrapping::create(GL_REPEAT));
	tex->set_filter(TextureFilter::create(GL_LINEAR));
	tex->updateImage((GLubyte *) ilGetData());
	if (texCfg.useMipmaps) {
		tex->updateMipmaps();
	}
	GL_ERROR_LOG();

	return tex;
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

	TextureConfig texCfg;
	texCfg.forcedSize = Vec3ui(tex->width(), tex->height(), textureDepth);
	texCfg.forcedFormat = tex->format();
	texCfg.forcedInternalFormat = tex->internalFormat();
	texCfg.useMipmaps = (tex->getNumMipmaps() > 1);
	texCfg.keepData = keepData;
	ImageDataArray imgData = TextureLoaderRegistry::load(file, texCfg);
	if (imgData.empty()) {
		throw Error("No image data loaded");
	}
	auto &firstImage = imgData[0];

	tex->set_textureFile(file);
	tex->set_rectangleSize(firstImage->width, firstImage->height);
	tex->set_pixelType(firstImage->pixelType);
	tex->allocTexture();
	if (imgData.size() > 1) {
		auto *tex3d = dynamic_cast<Texture3D *>(tex.get());
		for (uint32_t i = 0u; i < imgData.size(); ++i) {
			tex3d->updateSubImage(i, imgData[i]->pixels);
		}
	}
	else if (!imgData.empty()) {
		tex->updateImage(imgData[0]->pixels);
	}
	if (tex->getNumMipmaps() > 1) {
		tex->updateMipmaps();
	}
	unsetData(imgData, tex, keepData);
}

ref_ptr<Texture2DArray> textures::loadArray(
		const std::string &textureDirectory,
		const std::string &textureNamePattern,
		const TextureConfig &texCfg) {
	std::filesystem::path dir(textureDirectory);
	boost::regex pattern(textureNamePattern);

	std::vector<TextureDescription> accumulator;
	std::filesystem::directory_iterator it(dir), eod;
	BOOST_FOREACH(const std::filesystem::path &filePath, std::make_pair(it, eod)) {
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
		const TextureConfig &texCfg_) {
	TextureConfig arrayTexCfg = texCfg_;
	uint32_t numTextures = textureFiles.size();
	ref_ptr<Texture2DArray> tex = ref_ptr<Texture2DArray>::alloc();
	tex->set_depth(numTextures);

	ImageDataArray imgData;
	int arrayIndex = 0;
	for (auto & descr : textureFiles) {
		if (descr.isFile()) {
			imgData = TextureLoaderRegistry::load(descr.getFileName(), arrayTexCfg);
			if (imgData.empty()) {
				throw Error("No image data loaded");
			}
			arrayTexCfg.forcedFormat = imgData[0]->format;
		}

		if (arrayIndex == 0) {
			arrayTexCfg.forcedSize.x = imgData[0]->width;
			arrayTexCfg.forcedSize.y = imgData[0]->height;
			tex->set_rectangleSize(arrayTexCfg.forcedSize.x, arrayTexCfg.forcedSize.y);
			tex->set_pixelType(imgData[0]->pixelType);
			tex->set_format(regenImageFormat(imgData[0]->format));
			tex->set_internalFormat(arrayTexCfg.forcedInternalFormat == GL_NONE ?
				glenum::textureInternalFormat(tex->format()) :
				arrayTexCfg.forcedInternalFormat);
			tex->allocTexture();
		}

		if (descr.isFile()) {
			tex->updateSubImage(arrayIndex, imgData[0]->pixels);
		} else if (descr.isClear()) {
			glClearTexSubImage(tex->textureBind().id_,
				0, 0, 0, arrayIndex, // level, x, y, z offset
				static_cast<int>(arrayTexCfg.forcedSize.x),
				static_cast<int>(arrayTexCfg.forcedSize.y), 1, // width, height, depth
				tex->format(), tex->pixelType(),
				descr.getClearData());
		}
		arrayIndex += 1;
	}
	tex->set_wrapping(TextureWrapping::create(GL_REPEAT));
	tex->set_filter(TextureFilter::create(GL_LINEAR));
	if (arrayTexCfg.useMipmaps) {
		tex->updateMipmaps();
	}
	GL_ERROR_LOG();

	return tex;
}

ref_ptr<TextureCube> textures::loadCube(
		std::string_view file,
		bool flipBackFace,
		const TextureConfig &texCfg) {
	ImageDataArray imgData = TextureLoaderRegistry::load(file, texCfg);
	if (imgData.empty()) {
		throw Error("No image data loaded");
	}
	auto &firstImage = imgData[0];

	int faceWidth, faceHeight;
	int faces[12];
	// guess layout
	if (firstImage->width > firstImage->height) {
		faceWidth = firstImage->width / 4;
		faceHeight = firstImage->height / 3;
		int faces_[12] = {
				-1, TextureCube::TOP, -1, -1,
				TextureCube::LEFT, TextureCube::BACK, TextureCube::RIGHT, TextureCube::FRONT,
				-1, TextureCube::BOTTOM, -1, -1
		};
		for (int i = 0; i < 12; ++i) faces[i] = faces_[i];
	} else {
		faceWidth = firstImage->width / 3;
		faceHeight = firstImage->height / 4;
		int faces_[12] = {
				-1, TextureCube::TOP, -1,
				TextureCube::LEFT, TextureCube::BACK, TextureCube::RIGHT,
				-1, TextureCube::BOTTOM, -1,
				-1, TextureCube::FRONT, -1
		};
		for (int i = 0; i < 12; ++i) faces[i] = faces_[i];
	}
	const auto numRows = firstImage->height / faceHeight;
	const auto numCols = firstImage->width / faceWidth;
	const auto bpp = firstImage->bpp;
	const auto faceBytes = bpp * faceWidth * faceHeight;
	const auto rowBytes = faceBytes * numCols;

	auto tex = ref_ptr<TextureCube>::alloc();
	tex->set_textureFile(file);
	tex->set_rectangleSize(faceWidth, faceHeight);
	tex->set_pixelType(firstImage->pixelType);
	tex->set_format(regenImageFormat(firstImage->format));
	tex->set_internalFormat(texCfg.forcedInternalFormat == GL_NONE ?
		glenum::textureInternalFormat(tex->format()) :
		texCfg.forcedInternalFormat);
	tex->allocTexture();
	tex->set_filter(TextureFilter::create(GL_LINEAR));

	std::vector<byte> tmpFace(faceBytes);
	int index = 0;
	for (int row = 0; row < numRows; ++row) {
		int flippedRow = numRows - 1 - row;  // account for bottom-left origin
		byte *rowPtr = firstImage->pixels + flippedRow * rowBytes;

		byte *colData = rowPtr;
		for (int col = 0; col < numCols; ++col) {
			int mappedFace = faces[index];
			if (mappedFace != -1) {
				auto nextFace = (TextureCube::CubeSide) mappedFace;

				if (nextFace == TextureCube::TOP || nextFace == TextureCube::BOTTOM) {
					glPixelStorei(GL_UNPACK_ROW_LENGTH, faceWidth * numCols);
					tex->updateSubImage(nextFace, colData);
				}
				else if (flipBackFace && nextFace == TextureCube::FRONT) {
					glPixelStorei(GL_UNPACK_ROW_LENGTH, faceWidth * numCols);
					tex->updateSubImage(nextFace, colData);
				} else {
					glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
					auto *dst = tmpFace.data();
					for (int y = faceHeight - 1; y >= 0; --y) {
						auto *rowData = colData + y * faceWidth * bpp * numCols;
						for (int x = faceWidth - 1; x >= 0; --x) {
							byte *pixelData = rowData + x * bpp;
							memcpy(dst, pixelData, bpp);
							dst += bpp;
						}
					}
					tex->updateSubImage(nextFace, tmpFace.data());
				}
			}
			index += 1;
			colData += bpp * faceWidth;
		}
	}

	if (texCfg.useMipmaps) {
		tex->updateMipmaps();
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	return tex;
}

ref_ptr<Texture> textures::loadRAW(
		const std::string &path,
		const Vec3ui &size,
		uint32_t numComponents,
		uint32_t bytesPerComponent) {
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
	tex->set_filter(TextureFilter::create(GL_LINEAR));
	tex->set_wrapping(TextureWrapping::create(GL_REPEAT));
	tex->updateImage((GLubyte *) pixels);
	delete[] pixels;

	return tex;
}

ref_ptr<Texture> textures::loadSpectrum(
		double t1,
		double t2,
		int numTexels,
		bool useMipmaps) {
	auto *data = new unsigned char[numTexels * 4];
	spectrum(t1, t2, numTexels, data);

	ref_ptr<Texture> tex = ref_ptr<Texture1D>::alloc();
	tex->set_rectangleSize(numTexels, 1);
	tex->set_pixelType(GL_UNSIGNED_BYTE);
	tex->set_format(GL_RGBA);
	tex->set_internalFormat(GL_RGBA8);
	tex->allocTexture();
	tex->set_wrapping(TextureWrapping::create(GL_CLAMP_TO_EDGE));
	tex->set_filter(TextureFilter::create(GL_LINEAR));
	tex->updateImage((GLubyte *) data);
	if (useMipmaps) {
		tex->updateMipmaps();
	}
	delete[]data;

	return tex;
}

