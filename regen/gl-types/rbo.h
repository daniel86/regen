/*
 * rbo.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _RBO_H_
#define _RBO_H_

#include <regen/gl-types/buffer-object.h>

namespace regen {
/**
 * \brief OpenGL Objects that contain images.
 *
 * They are created and used specifically with Framebuffer Objects.
 * They are optimized for being used as render targets,
 * while Textures may not be.
 */
class RenderBufferObject : public RectBufferObject
{
public:
  /**
   * @param numBuffers number of GL buffers.
   */
  RenderBufferObject(GLuint numBuffers=1);

  /**
   * Specifies the internal format to use for the renderbuffer object's image.
   * Accepted values are GL_ALPHA*, GL_INTENSITY*, GL_R*,
   * GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
   * GL_LUMINANCE*, GL_SRGB*, GL_SLUMINANCE*, GL_COMPRESSED_*.
   */
  void set_format(GLenum format);
  /**
   * Specifies the renderbuffer target of the binding operation.
   */
  GLenum targetType() const;

  /**
   * Establish data storage, format and dimensions
   * of a renderbuffer object's image using multisampling.
   */
  void storageMS(GLuint numMultisamples) const;
  /**
   * Establish data storage, format and dimensions of a
   * renderbuffer object's image
   */
  void storage() const;

  /**
   * Bind the renderbuffer to a renderbuffer target
   */
  void bind() const;

protected:
  GLenum targetType_;
  GLenum format_;
};
} // namespace

#endif /* GL_FBO_H_ */
