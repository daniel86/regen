/*
 * tbo.h
 *
 *  Created on: 23.10.2011
 *      Author: daniel
 */

#ifndef TBO_H_
#define TBO_H_

#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/vbo.h>

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

  void attach(ref_ptr<VertexBufferObject> &vbo);
  void attach(GLuint storage);

  // override
  virtual string samplerType() const;

private:
  string samplerType_;
  GLenum texelFormat_;
  ref_ptr<VertexBufferObject> attachedVBO_;

  // override
  virtual void texImage() const;
};

#endif /* TBO_H_ */
