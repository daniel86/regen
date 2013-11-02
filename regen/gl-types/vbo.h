/*
 * vbo.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _VBO_H_
#define _VBO_H_

#include <list>

#include <regen/gl-types/gl-object.h>
#include <regen/utility/ref-ptr.h>
#include <regen/utility/memory-allocator.h>

#ifndef byte
  typedef unsigned char byte;
#endif

namespace regen {
  class ShaderInput; // forward declaration

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
   * A memory pool of pre-allocated GPU storage is used in combination
   * with a memory manager to avoid fragmentation.
   * Each time alloc is called the memory pool is asked to provide a free block
   * that fits the size request. As a result you can not be sure to get contiguous memory
   * if you call alloc multiple times. Each allocation reserves a block of contiguous memory
   * but blocks do not have to follow each other, they may not even be part of the same
   * GL buffer object.
   *
   * In the destructor VBO's free all the storage that was allocated,
   * you do not have to call free if you keep the alloc for the VBO
   * lifetime.
   */
  class VBO
  {
  public:
    /**
     * \brief Interface for allocating GPU memory.
     */
    struct VBOAllocator {
      /**
       * Generate buffer object name and
       * create and initialize the buffer object's data store to the named size.
       */
      static GLuint createAllocator(GLuint poolIndex, GLuint size);
      /**
       * Delete named buffer object.
       * After a buffer object is deleted, it has no contents,
       * and its name is free for reuse.
       */
      static void deleteAllocator(GLuint poolIndex, GLuint ref);
    };
    /**
     * \brief A pool of VBO memory allocators.
     */
    typedef AllocatorPool<
        VBOAllocator, GLuint,   // actual allocator and reference type
        BuddyAllocator, GLuint  // virtual allocator and reference type
    > VBOPool;
    /**
     * \brief Reference to allocated data.
     */
    struct Reference {
      Reference() : vbo_(NULL), allocatedSize_(0) {}
      /**
       * @return true if this reference is not associated to an allocated block.
       */
      GLboolean isNullReference() const;
      /**
       * @return the allocated block size.
       */
      GLuint allocatedSize() const;
      /**
       * @return virtual address to allocated block.
       */
      GLuint address() const;
      /**
       * @return buffer object name.
       */
      GLuint bufferID() const;
      /**
       * @return The associated VBO.
       */
      VBO* vbo() const;

    private: // only VBO allowed to access
      VBO *vbo_;
      VBOPool::Reference poolReference_;
      GLuint allocatedSize_;
      list< ref_ptr<Reference> >::iterator it_;
      ~Reference();
      // no copy allowed
      Reference(const Reference&) {}
      Reference& operator=(const Reference&) { return *this; }
      friend class VBO;
      friend class ref_ptr<Reference>;
    };
    /**
     * \brief Flag indicating the usage of the data in the VBO
     */
    enum Usage {
      USAGE_DYNAMIC = 0,
      USAGE_STATIC,
      USAGE_STREAM,
      USAGE_FEEDBACK, /**< Transform feedback */
      USAGE_TEXTURE,  /**< TBO */
      USAGE_UNIFORM,  /**< UBO */
      USAGE_LAST
    };

    /////////////////////
    /////////////////////

    /**
     * Calculates the struct size for the attributes in bytes.
     */
    static GLuint attributeSize(
        const list< ref_ptr<ShaderInput> > &attributes);

    /**
     * Copy the VBO data to another buffer.
     * @param from the VBO handle containing the data
     * @param to the VBO handle to copy the data to
     * @param size size of data to copy in bytes
     * @param offset offset in data VBO
     * @param toOffset in destination VBO
     */
    static void copy(GLuint from, GLuint to,
        GLuint size, GLuint offset, GLuint toOffset);
    /**
     * Create memory pool instances for different usage hints.
     * GL context must be setup when calling this
     * because Get* functions are used to configure the pools.
     */
    static void createMemoryPools();
    /**
     * Destroy memory pools. Free all allocated memory.
     */
    static void destroyMemoryPools();
    /**
     * Get a memory pool for specified usage.
     * @param usage the usage hint.
     * @return memory pool.
     */
    static VBOPool* memoryPool(Usage usage);

    /////////////////////
    /////////////////////

    /**
     * Default-Constructor.
     * @param usage usage hint.
     */
    VBO(Usage usage);

    /**
     * Provides info how the buffer object is going to be used.
     */
    Usage usage() const;
    /**
     * Allocated VRAM in bytes.
     */
    GLuint allocatedSize() const;

    /**
     * Allocate a block in the VBO memory.
     * Note that as long as you keep a reference the allocated storage
     * is marked as used.
     */
    ref_ptr<Reference>& alloc(GLuint size);
    /**
     * Allocate a block in the VBO memory.
     * And copy the data from RAM to GPU.
     * Note that as long as you keep a reference the allocated storage
     * is marked as used.
     */
    ref_ptr<Reference>& alloc(const ref_ptr<ShaderInput> &att);
    /**
     * Allocate GPU memory for the given attributes.
     * And copy the data from RAM to GPU.
     * Note that as long as you keep a reference the allocated storage
     * is marked as used.
     */
    ref_ptr<Reference>& allocInterleaved(const list< ref_ptr<ShaderInput> > &attributes);
    /**
     * Allocate GPU memory for the given attributes.
     * And copy the data from RAM to GPU.
     * Note that as long as you keep a reference the allocated storage
     * is marked as used.
     */
    ref_ptr<Reference>& allocSequential(const list< ref_ptr<ShaderInput> > &attributes);
    /**
     * Free previously allocated block of GPU memory.
     * Actually this will mark the space as free so that others
     * can allocate it again -- but only if you do not keep a reference
     * on the Reference instance somewhere. The allocated space is not marked as
     * free as long as you are referencing the allocated block.
     */
    static void free(Reference *ref);

    /**
    * Copy vertex data to the buffer object. Sets part of data.
    * Make sure to bind before.
    * Replaces only existing data, no new memory allocated for the buffer.
    */
    void set_bufferSubData(GLenum target, GLuint offset, GLuint size, void *data) const
    { glBufferSubData(target, offset, size, data); }

    /**
    * Map a range of the buffer object into client's memory.
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
    static VBOPool *dataPools_;

    list< ref_ptr<Reference> > allocations_;

    Usage usage_;
    // sum of allocated blocks
    GLuint allocatedSize_;

    void uploadInterleaved(
        GLuint startByte,
        GLuint endByte,
        const list< ref_ptr<ShaderInput> > &attributes,
        ref_ptr<Reference> &ref);
    void uploadSequential(
        GLuint startByte,
        GLuint endByte,
        const list< ref_ptr<ShaderInput> > &attributes,
        ref_ptr<Reference> &ref);

    ref_ptr<Reference>& createReference(GLuint numBytes);
    ref_ptr<Reference>& nullReference();
  };

  ostream& operator<<(ostream &out, const VBO::Usage &v);
  istream& operator>>(istream &in, VBO::Usage &v);

  typedef ref_ptr<VBO::Reference> VBOReference;
} // namespace

#endif /* _VBO_H_ */
