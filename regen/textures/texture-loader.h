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
#include <regen/algebra/vector.h>

namespace ogle {
/**
 * \brief Handles loading of some special Texture types.
 */
class TextureLoader {
public:
  /**
   * \brief An error occurred loading the Texture.
   */
  class Error : public runtime_error {
  public:
    /**
     * @param message the error message.
     */
    Error(const string &message) : runtime_error(message) {}
  };
  /**
   * Load a Texture from file. Guess if it is a Texture2D or Texture3D.
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
  /**
   * Load a Texture2DArray from file.
   * Force specified internal format.
   * Scale to forced size (if forced size != 0).
   * Setup mipmapping after loading the file.
   */
  static ref_ptr<Texture2DArray> loadArray(
      const string &textureDirectory,
      const string &textureNamePattern,
      GLenum mipmapFlag=GL_DONT_CARE,
      GLenum forcedInternalFormat=GL_NONE,
      GLenum forcedFormat=GL_NONE,
      GLenum forcedType=GL_NONE,
      const Vec3ui &forcedSize=Vec3ui(0u));
  /**
   * Load a TextureCube from file.
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
   * Loads RAW Texture from file.
   */
  static ref_ptr<Texture> loadRAW(
      const string &path,
      const Vec3ui &size,
      GLuint numComponents,
      GLuint bytesPerComponent);
  /**
   * 1D texture that contains a color spectrum.
   */
  static ref_ptr<Texture> loadSpectrum(
      GLdouble t1,
      GLdouble t2,
      GLint numTexels,
      GLenum mimpmapFlag=GL_DONT_CARE);
};
} // end ogle namespace

#endif /* TEXTURE_LOADER_H_ */
