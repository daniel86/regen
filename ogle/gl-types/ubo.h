/*
 * ubo.h
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#ifndef UBO_H_
#define UBO_H_

#include <ogle/gl-types/buffer-object.h>

/**
 * A Buffer Object that is used to store uniform data for a shader program
 * is called a Uniform Buffer Object. They can be used to share
 * uniforms between different programs, as well as quickly change
 * between sets of uniforms for the same program object.
 *
 * The term "Uniform Buffer Object" refers to the OpenGL buffer object
 * that is used to provide storage for uniforms. The term "uniform blocks"
 * refer to the GLSL language grouping of uniforms that must have buffer
 * objects as storage.
 */
class UniformBufferObject : public BufferObject
{
public:
  enum Layout { SHARED, STD_140, PACKED };

  UniformBufferObject();
  ~UniformBufferObject();

  Layout layout() const;
  void set_layout(Layout layout);

  /**
   * retrieve the index of a named uniform block
   */
  static inline GLuint getBlockIndex(GLuint shader, char* blockName)
  {
    return glGetUniformBlockIndex(shader, blockName);
  }

  /**
   * assign a binding point to an active uniform block
   */
  static inline void bindBlock(
      GLuint shader, GLuint blockIndex, GLuint bindingPoint)
  {
    glUniformBlockBinding(shader, blockIndex, bindingPoint);
  }

  /**
   * bind this UBO to GL_UNIFORM_BUFFER
   */
  inline void bindBuffer()
  {
    glBindBuffer(GL_UNIFORM_BUFFER, id_);
  }
  /**
   * unbind previously bound buffer
   */
  static inline void unbindBuffer()
  {
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  /**
   * bind this UBO to an indexed buffer target
   */
  inline void bindBufferBase(long bindingPoint)
  {
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, id_);
  }
  /**
   * unbind previously bound buffer
   */
  static inline void unbindBufferBase(long bindingPoint)
  {
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, 0);
  }

  /**
   * creates and initializes a buffer object's data store.
   * If the data pointer is a NULL pointer only space is allocated.
   */
  inline void setData(float* pData, long size)
  {
    blockSize_ = size;
    glBufferData(GL_UNIFORM_BUFFER, size, pData, GL_DYNAMIC_DRAW);
  }

  /**
   * Sets data for a part of the buffer.
   */
  static inline void setSubData(
      float* pData, long offset, long size)
  {
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, pData);
  }

protected:
  GLuint id_;
  GLsizei blockSize_;
  Layout layout_;
};

#endif /* UBO_H_ */
