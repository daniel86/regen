/*
 * texture-loader.h
 *
 *  Created on: 21.12.2012
 *      Author: daniel
 */

#ifndef TEXTURE_LOADER_H_
#define TEXTURE_LOADER_H_

#include <stdexcept>

#include <ogle/exceptions/io-exceptions.h>
#include <ogle/gl-types/texture.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/vector.h>

class ImageError : public runtime_error
{
public:
  ImageError(const string &message)
  : runtime_error(message)
  {
  }
};

/**
 * Loads textures from file.
 */
class TextureLoader {
public:
  /**
   * Load a texture from file. Guess if it is a 2D or 3D texture.
   * Force specified internal format.
   * Scale to forced size (if forced size != 0).
   * Setup mipmapping after loading the file.
   */
  static ref_ptr<Texture> load(
      const string &file,
      GLenum mipmapFlag=GL_DONT_CARE,
      GLenum forcedInternalFormat=GL_NONE,
      GLenum forcedFormat=GL_NONE,
      GLenum forcedType=GL_NONE,
      const Vec3ui &forcedSize=Vec3ui(0u));
  static ref_ptr<Texture2DArray> loadArray(
      const string &textureDirectory,
      const string &textureNamePattern,
      GLenum mipmapFlag=GL_DONT_CARE,
      GLenum forcedInternalFormat=GL_NONE,
      GLenum forcedFormat=GL_NONE,
      GLenum forcedType=GL_NONE,
      const Vec3ui &forcedSize=Vec3ui(0u));
  /**
   * Load a cube texture from file.
   * The file is expected to be a regular 2D image containing
   * multiple faces arranged next to each other.
   * Force specified internal format.
   * Scale to forced size (if forced size != 0).
   * Setup mipmapping after loading the file.
   */
  static ref_ptr<TextureCube> loadCube(
      const string &file,
      GLboolean flipBackFace=GL_FALSE,
      GLenum mipmapFlag=GL_DONT_CARE,
      GLenum forcedInternalFormat=GL_NONE,
      GLenum forcedFormat=GL_NONE,
      GLenum forcedType=GL_NONE,
      const Vec3ui &forcedSize=Vec3ui(0u));
  /**
   * Loads RAW texture from file.
   */
  static ref_ptr<Texture> loadRAW(
      const string &path,
      const Vec3ui &size,
      GLuint numComponents,
      GLuint bytesPerComponent);
  /**
   * 1 dimensional texture that contains a spectrum.
   */
  static ref_ptr<Texture> loadSpectrum(
      GLdouble t1,
      GLdouble t2,
      GLint numTexels,
      GLenum mimpmapFlag=GL_DONT_CARE);
};

#endif /* TEXTURE_LOADER_H_ */
