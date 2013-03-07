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

namespace ogle {

/**
 * \brief One-dimensional arrays of texels whose storage
 * comes from an attached buffer object.
 *
 * When a buffer object is bound to a buffer texture,
 * a format is specified, and the data in the buffer object
 * is treated as an array of texels of the specified format.
 */
class TextureBufferObject : public Texture {
public:
  /**
   * Accepted values are GL_ALPHA*, GL_INTENSITY*, GL_R*,
   * GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
   * GL_LUMINANCE*, GL_SRGB*, GL_SLUMINANCE*, GL_COMPRESSED_*.
   */
  TextureBufferObject(GLenum texelFormat);

  /**
   * Attach VBO to TBO and keep a reference on the VBO.
   */
  void attach(const ref_ptr<VertexBufferObject> &vbo);
  /**
   * Attach the storage for a buffer object to the active buffer texture.
   */
  void attach(GLuint storage);

private:
  GLenum texelFormat_;
  ref_ptr<VertexBufferObject> attachedVBO_;

  // override
  virtual void texImage() const;
};

} // end ogle namespace

#endif /* TBO_H_ */
