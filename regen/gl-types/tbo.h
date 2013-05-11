/*
 * tbo.h
 *
 *  Created on: 23.10.2011
 *      Author: daniel
 */

#ifndef TBO_H_
#define TBO_H_

#include <regen/gl-types/texture.h>
#include <regen/gl-types/vbo.h>

namespace regen {
  /**
   * \brief One-dimensional arrays of texels whose storage
   * comes from an attached buffer object.
   *
   * When a buffer object is bound to a buffer texture,
   * a format is specified, and the data in the buffer object
   * is treated as an array of texels of the specified format.
   */
  class TBO : public Texture {
  public:
    /**
     * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
     * GL_SRGB*, GL_COMPRESSED_*.
     */
    TBO(GLenum texelFormat);

    /**
     * Attach VBO to TBO and keep a reference on the VBO.
     */
    void attach(const ref_ptr<VBO> &vbo, VBOReference &ref);
    /**
     * Attach the storage for a buffer object to the active buffer texture.
     */
    void attach(GLuint storage);
    /**
     * Attach the storage for a buffer object to the active buffer texture.
     */
    void attach(GLuint storage, GLuint offset, GLuint size);

  private:
    GLenum texelFormat_;
    ref_ptr<VBO> attachedVBO_;
    VBOReference attachedVBORef_;

    // override
    virtual void texImage() const;
  };
} // namespace

#endif /* TBO_H_ */