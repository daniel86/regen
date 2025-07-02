#ifndef REGEN_BUFFER_OBJECT_H_
#define REGEN_BUFFER_OBJECT_H_

#include <regen/gl-types/buffer-target.h>
#include <regen/gl-types/buffer-usage.h>
#include <regen/gl-types/buffer-reference.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
	/**
	 * \brief Base class for OpenGL buffer objects.
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
	 */
	class BufferObject : public Resource {
	public:
		explicit BufferObject(BufferTarget target, BufferUsage=BUFFER_USAGE_DYNAMIC_DRAW);

		~BufferObject() override;

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 */
		BufferObject(const BufferObject &other);

		/**
		 * Provides info how the buffer object is going to be used.
		 */
		BufferUsage usage() const { return usage_; }

		/**
		 * Allocated VRAM in bytes.
		 */
		GLuint allocatedSize() const { return allocatedSize_; }

		/**
		 * Allocate a block in the VBO memory.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference> &allocBytes(GLuint size);

		/**
		 * Free previously allocated block of GPU memory.
		 * Actually this will mark the space as free so that others
		 * can allocate it again -- but only if you do not keep a reference
		 * on the Reference instance somewhere. The allocated space is not marked as
		 * free as long as you are referencing the allocated block.
		 */
		static void free(BufferReference *ref);

		/**
		 * @return the list of all allocated buffers.
		 */
		auto &allocations() const { return allocations_; }

		unsigned int bufferID(unsigned int index = 0) const { return allocations_[index]->bufferID(); }

		/**
		 * Bind the buffer object to a binding point.
		 * @param index the binding point.
		 */
		void bind(GLuint index) const;

		/**
		* Copy vertex data to the buffer object. Sets part of data.
		* Replaces only existing data, no new memory allocated for the buffer.
		* Make sure to bind before.
		*/
		static void setBufferData(const ref_ptr<BufferReference> &ref, const GLuint *data);

		/**
		* Map a range of the buffer object into client's memory.
		* If OpenGL is able to map the buffer object into client's address space,
		* map returns the pointer to the buffer. Otherwise it returns NULL.
		* Make sure to bind before.
		*/
		//static GLvoid *map(GLenum target, GLuint offset, GLuint size, GLenum accessFlags);

		GLvoid *map(GLuint relativeOffset, GLuint mappedSize, GLenum accessFlags);

		GLvoid *map(GLenum accessFlags);

		/**
		* Map a range of the buffer object into client's memory.
		* If OpenGL is able to map the buffer object into client's address space,
		* map returns the pointer to the buffer. Otherwise it returns NULL.
		* Make sure to bind before.
		*/
		static GLvoid *map(const ref_ptr<BufferReference> &ref, GLenum accessFlags) ;

		/**
		* Unmaps previously mapped data.
		*/
		void unmap() const;

		/**
		 * Copy the VBO data to another buffer.
		 * @param from the VBO handle containing the data
		 * @param to the VBO handle to copy the data to
		 * @param size size of data to copy in bytes
		 * @param offset offset in data VBO
		 * @param toOffset in destination VBO
		 */
		static void copy(GLuint from, GLuint to, GLuint size, GLuint offset, GLuint toOffset);

		/**
		 * Calculates the struct size for the attributes in bytes.
		 */
		static GLuint attributeSize(const std::list<ref_ptr<ShaderInput> > &attributes);

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
		static BufferPool *bufferPool(BufferTarget target, BufferUsage usage);

	protected:
		BufferUsage usage_;
		BufferTarget target_;
		GLenum glTarget_;

		std::vector<ref_ptr<BufferReference> > allocations_;
		// sum of allocated bytes
		GLuint allocatedSize_;

		ref_ptr<BufferReference> &createReference(GLuint numBytes);

		static ref_ptr<BufferReference> &nullReference();

		static BufferPool **bufferPools();

		friend struct BufferReference;
	};

	template<BufferTarget target>
	class BufferObjectT : public BufferObject {
	public:
		explicit BufferObjectT(BufferUsage usage) : BufferObject(target, usage) {}
	};
} // namespace

#endif /* REGEN_BUFFER_OBJECT_H_ */
