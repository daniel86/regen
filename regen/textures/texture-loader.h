/*
 * texture-loader.h
 *
 *  Created on: 21.12.2012
 *      Author: daniel
 */

#ifndef TEXTURE_LOADER_H_
#define TEXTURE_LOADER_H_

#include <stdexcept>

#include <regen/gl-types/texture.h>
#include <regen/utility/ref-ptr.h>
#include <regen/math/vector.h>

namespace regen {
	namespace textures {
		/**
		 * \brief An error occurred loading the Texture.
		 */
		class Error : public std::runtime_error {
		public:
			/**
			 * @param message the error message.
			 */
			explicit Error(const std::string &message) : std::runtime_error(message) {}
		};

		GLenum regenImageFormat();

		GLuint loadImage(const std::string &file);

		/**
		 * Load a Texture from file. Guess if it is a Texture2D or Texture3D.
		 * Force specified internal format.
		 * Scale to forced size (if forced size != 0).
		 * Setup mipmapping after loading the file.
		 */
		ref_ptr<Texture> load(
				const std::string &file,
				GLenum mipmapFlag = GL_DONT_CARE,
				GLenum forcedInternalFormat = GL_NONE,
				GLenum forcedFormat = GL_NONE,
				const Vec3ui &forcedSize = Vec3ui(0u),
				bool keepData = false);

		/**
		 * Load a Texture from RAW data. Guess if it is a Texture2D or Texture3D.
		 * Force specified internal format.
		 * Scale to forced size (if forced size != 0).
		 * Setup mipmapping after loading the file.
		 */
		ref_ptr<Texture> load(
				GLuint textureType,
				GLuint numBytes,
				const void *rawData,
				GLenum mipmapFlag = GL_DONT_CARE,
				GLenum forcedInternalFormat = GL_NONE,
				GLenum forcedFormat = GL_NONE,
				const Vec3ui &forcedSize = Vec3ui(0u));

		/**
		 * Load a Texture2DArray from file.
		 * Force specified internal format.
		 * Scale to forced size (if forced size != 0).
		 * Setup mipmapping after loading the file.
		 */
		ref_ptr<Texture2DArray> loadArray(
				const std::string &textureDirectory,
				const std::string &textureNamePattern,
				GLenum mipmapFlag = GL_DONT_CARE,
				GLenum forcedInternalFormat = GL_NONE,
				GLenum forcedFormat = GL_NONE,
				const Vec3ui &forcedSize = Vec3ui(0u));

		/**
		 * Load a TextureCube from file.
		 * The file is expected to be a regular 2D image containing
		 * multiple faces arranged next to each other.
		 * Force specified internal format.
		 * Scale to forced size (if forced size != 0).
		 * Setup mipmapping after loading the file.
		 */
		ref_ptr<TextureCube> loadCube(
				const std::string &file,
				GLboolean flipBackFace = GL_FALSE,
				GLenum mipmapFlag = GL_DONT_CARE,
				GLenum forcedInternalFormat = GL_NONE,
				GLenum forcedFormat = GL_NONE,
				const Vec3ui &forcedSize = Vec3ui(0u));

		/**
		 * Loads RAW Texture from file.
		 */
		ref_ptr<Texture> loadRAW(
				const std::string &path,
				const Vec3ui &size,
				GLuint numComponents,
				GLuint bytesPerComponent);

		/**
		 * 1D texture that contains a color spectrum.
		 */
		ref_ptr<Texture> loadSpectrum(
				GLdouble t1,
				GLdouble t2,
				GLint numTexels,
				GLenum mimpmapFlag = GL_DONT_CARE);
	};
} // namespace

#endif /* TEXTURE_LOADER_H_ */
