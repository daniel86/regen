/*
 * rbo.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _RBO_H_
#define _RBO_H_

#include <ogle/gl-types/buffer-object.h>

/**
 * Renderbuffer Objects are OpenGL Objects that contain images.
 * They are created and used specifically with Framebuffer Objects.
 * They are optimized for being used as render targets,
 * while Textures may not be.
 */
class RenderBufferObject : public RectBufferObject
{
public:
  RenderBufferObject(GLuint numBuffers=1);

  /**
   * Specifies the internal format to use
   * for the renderbuffer object's image.
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
  inline void storageMS(GLuint numMultisamples) const {
    glRenderbufferStorageMultisample(
        GL_RENDERBUFFER, numMultisamples, format_, width_, height_);
  }
  /**
   * Establish data storage, format and dimensions of a
   * renderbuffer object's image
   */
  inline void storage() const {
    glRenderbufferStorage(
        GL_RENDERBUFFER, format_, width_, height_);
  }
  /**
   * Bind the renderbuffer to a renderbuffer target
   */
  inline void bind() const {
    glBindRenderbuffer(targetType_, ids_[bufferIndex_]);
  }
protected:
  GLenum targetType_;
  GLenum format_;
};


#endif /* GL_FBO_H_ */
