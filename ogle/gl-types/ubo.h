/*
 * ubo.h
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#ifndef UBO_H_
#define UBO_H_

#include <ogle/gl-types/buffer-object.h>

#ifndef byte
  typedef unsigned char byte;
#endif

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
  GLuint getBlockIndex(GLuint shader, char* blockName);

  /**
   * assign a binding point to an active uniform block
   */
  void bindBlock(GLuint shader, GLuint blockIndex, GLuint bindingPoint);

  /**
   * bind this UBO to GL_UNIFORM_BUFFER
   */
  void bind() const;
  /**
   * unbind previously bound buffer
   */
  void bindZero() const;

  /**
   * bind this UBO to an indexed buffer target
   */
  void bindBufferBase(GLuint bindingPoint);
  /**
   * unbind previously bound buffer
   */
  void unbindBufferBase(GLuint bindingPoint);

  /**
   * creates and initializes a buffer object's data store.
   * If the data pointer is a NULL pointer only space is allocated.
   */
  void setData(byte *data, GLuint size);

  /**
   * Sets data for a part of the buffer.
   */
  void setSubData(byte *data, GLuint offset, GLuint size);

protected:
  GLuint id_;
  GLsizei blockSize_;
  Layout layout_;
};

#endif /* UBO_H_ */
