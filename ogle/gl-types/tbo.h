/*
 * tbo.h
 *
 *  Created on: 23.10.2011
 *      Author: daniel
 */

#ifndef TBO_H_
#define TBO_H_

#include <ogle/gl-types/texture.h>

/**
 * Buffer textures are one-dimensional arrays of texels whose storage
 * comes from an attached buffer object.
 * When a buffer object is bound to a buffer texture,
 * a format is specified, and the data in the buffer object
 * is treated as an array of texels of the specified format.
 */
class TextureBufferObject : public Texture {
public:
  TextureBufferObject(
      GLenum texelFormat=GL_RGBA32F,
      const string &samplerType="samplerBuffer");

  /**
   * Data storage for this TBO.
   */
  void attachStorage(GLuint storage);
  /**
   * Data storage for this TBO.
   */
  const GLuint lastStorage() const;

  // override
  virtual string samplerType() const;

private:
  string samplerType_;
  GLenum texelFormat_;
  GLuint lastStorage_;

  // override
  virtual void texImage() const;
};

#endif /* TBO_H_ */
