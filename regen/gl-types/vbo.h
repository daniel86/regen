/*
 * vbo.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _VBO_H_
#define _VBO_H_

#include <list>

#include <regen/gl-types/buffer-object.h>
#include <regen/utility/ref-ptr.h>
#include <regen/utility/memory-allocator.h>

namespace regen {

class VertexAttribute; // forward declaration

/**
 * Macro for buffer access with offset.
 */
#ifndef BUFFER_OFFSET
  #define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

/**
 * \brief Buffer object that is used for vertex data.
 *
 * Since the storage for buffer objects
 * is allocated by OpenGL, vertex buffer objects are a mechanism
 * for storing vertex data in "fast" memory (i.e. video RAM or AGP RAM,
 * and in the case of PCI Express, video RAM or RAM),
 * thereby allowing for significant increases in vertex throughput
 * between the application and the GPU.
 *
 * Vertex data can be either vertex attributes and/or indices.
 */
class VertexBufferObject : public BufferObject
{
public:
  typedef AllocatorPool<BuddyAllocator,unsigned int> VBOPool;
  typedef VBOPool::Reference Reference;

  /**
   * \brief Flag indicating the usage of the data in the vbo
   */
  enum Usage {
    USAGE_DYNAMIC = GL_DYNAMIC_DRAW,
    USAGE_STATIC = GL_STATIC_DRAW,
    USAGE_STREAM = GL_STREAM_DRAW
  };

  /////////////////////
  /////////////////////

  /**
   * @param v default usage hint.
   */
  static void set_defaultUsage(Usage v);

  /**
   * Allocate space in a VBO from the pool. Add the attributes interleaved to the vbo.
   */
  static Reference allocateInterleaved(
      const list< ref_ptr<VertexAttribute> > &attributes, Usage bufferUsage);
  /**
   * Allocate space in a VBO from the pool. Add the attributes sequential to the vbo.
   */
  static Reference allocateSequential(
      const list< ref_ptr<VertexAttribute> > &attributes, Usage bufferUsage);
  static Reference allocateSequential(
      const ref_ptr<VertexAttribute> &attribute, Usage bufferUsage);

  /**
   * Calculates the struct size for the attributes in bytes.
   */
  static GLuint attributeSize(
      const list< ref_ptr<VertexAttribute> > &attributes);

  /**
   * Free previously allocated space in the VBO.
   */
  static void free(Reference &it);

  /**
   * Copy the vbo data to another buffer.
   * @param from the vbo handle containing the data
   * @param to the vbo handle to copy the data to
   * @param size size of data to copy in bytes
   * @param offset offset in data vbo
   * @param toOffset in destination vbo
   */
  static void copy(GLuint from, GLuint to,
      GLuint size, GLuint offset, GLuint toOffset);

  /////////////////////
  /////////////////////

  /**
   * @param bufferSize size in bytes.
   * @param usage usage hint.
   * @param useMemoryPool if true the vbo is added to the memory pool.
   */
  VertexBufferObject(
      Usage usage, GLuint bufferSize);
  VertexBufferObject(
      Usage usage, VBOPool::Node *n);
  ~VertexBufferObject();

  /**
   * Resize VBO to contain at least bufferSize bytes.
   * The VBO will be empty after calling this.
   * All stored data will be lost.
   */
  void resize(GLuint bufferSize);

  /**
   * Allocated VRAM in bytes.
   */
  GLuint bufferSize() const;

  /**
   * Try to allocate space in this VBO for the given
   * attributes. Add the attributes interleaved to the vbo.
   */
  Reference allocateInterleaved(const list< ref_ptr<VertexAttribute> > &attributes);
  /**
   * Try to allocate space in this VBO for the given
   * attributes. Add the attributes sequential to the vbo.
   */
  Reference allocateSequential(const list< ref_ptr<VertexAttribute> > &attributes);
  /**
   * Try to allocate space in this VBO for the given
   * attribute. Add the attributes sequential to the vbo.
   */
  Reference allocateSequential(const ref_ptr<VertexAttribute> &att);

  /**
   * usage is a performance hint.
   * provides info how the buffer object is going to be used:
   * static, dynamic or stream, and read, copy or draw
   */
  Usage usage() const;

  /**
  * Copy vertex data to the buffer object. Sets part of data.
  * Make sure to bind before.
  * Replaces only existing data, no new memory allocated for the buffer.
  */
  void set_bufferSubData(GLenum target, GLuint offset, GLuint size, void *data) const
  { glBufferSubData(target, offset, size, data); }
  /**
   * Get the buffer data.
   */
  void data(GLenum target, GLuint offset, GLuint size, void *data) const
  { glGetBufferSubData(target, offset, size, data); }

  /**
  * map the buffer object into client's memory.
  * Make sure to bind before.
  *
  * If OpenGL is able to map the buffer object into client's address space,
  * map returns the pointer to the buffer. Otherwise it returns NULL.
  *
  * accessFlags can be one of GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE
  *
  * NOTE: causes synchronizing issues. Until mapped no gl* calls allowed.
  */
  GLvoid* map(GLenum target, GLenum accessFlags) const
  { return glMapBuffer(target, accessFlags); }
  /**
  * map a range of the buffer object into client's memory.
  * Make sure to bind before.
  *
  * If OpenGL is able to map the buffer object into client's address space,
  * map returns the pointer to the buffer. Otherwise it returns NULL.
  *
  * NOTE: causes synchronizing issues. Until mapped no gl* calls allowed.
  */
  GLvoid* map(GLenum target, GLuint offset, GLuint size, GLenum accessFlags) const
  { return glMapBufferRange(target, offset, size, accessFlags); }

  /**
  * Unmaps previously mapped data.
  */
  void unmap(GLenum target) const
  { glUnmapBuffer(target); }

protected:
  static VBOPool staticDataPool_;
  static VBOPool dynamicDataPool_;
  static VBOPool streamDataPool_;
  static VertexBufferObject::Usage defaultUsage_;

  Usage usage_;
  GLuint bufferSize_;
  VBOPool::Node *allocatorNode_;

  void uploadInterleaved(
      GLuint startByte,
      GLuint endByte,
      const list< ref_ptr<VertexAttribute> > &attributes,
      Reference blockIterator);
  void uploadSequential(
      GLuint startByte,
      GLuint endByte,
      const list< ref_ptr<VertexAttribute> > &attributes,
      Reference blockIterator);
  void allocateGPUMemory();

  static VBOPool* allocatorPool(Usage bufferUsage);
  static VBOPool::Node* getAllocator(GLuint size, Usage bufferUsage);
};
} // namespace

#endif /* _VBO_H_ */
