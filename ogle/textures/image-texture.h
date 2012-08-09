/*
 * image-texture.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _IMAGE_TEXTURE_H_
#define _IMAGE_TEXTURE_H_

#include <stdexcept>

#include <ogle/exceptions/io-exceptions.h>
#include <ogle/gl-types/texture.h>

class ImageError : public runtime_error
{
public:
  ImageError(const string &message)
  : runtime_error(message)
  {
  }
};

/**
 * A texture that can load data from common image file formats.
 * The formats supported depend on the used library DevIL.
 * @see http://openil.sourceforge.net/features.php
 */
class ImageTexture : public Texture {
public:
  /**
   * Default constructor, does not load image data.
   */
  ImageTexture();

  /**
   * Constructor that loads image data.
   */
  ImageTexture(
      const string &file,
      GLint width,
      GLint height,
      GLint depth,
      GLboolean useMipmap=true)
  throw (ImageError, FileNotFoundException);

  /**
   * Constructor that loads image data.
   */
  ImageTexture(
      const string &file,
      GLboolean useMipmap=true)
  throw (ImageError, FileNotFoundException);

  /**
   * Loads image data from file and uploads it to GL.
   * If mipmaps were generated before then mipmaps are generated
   * for the new image data too.
   */
  void set_file(
      const string &file,
      GLint width=0,
      GLint height=0,
      GLint depth=0,
      GLboolean useMipmap=true,
      GLenum mimpmapFlag=GL_DONT_CARE)
  throw (ImageError, FileNotFoundException);

  // overrides
  virtual void texSubImage(GLubyte *subData, GLint layer=0) const;
  virtual void texImage() const;
  virtual string samplerType() const;

protected:
  string samplerType_;
  GLint depth_;

private:
  static GLboolean devilInitialized_;
  ImageTexture(const ImageTexture&);
  void init();
};

#endif /* _IMAGE_TEXTURE_H_ */
