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
		/**
		 * Create a buffer object.
		 * @param target the buffer target.
		 * @param hint the buffer update hint.
		 */
		BufferObject(BufferTarget target, BufferUpdateHint hint);

		~BufferObject() override;

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 */
		BufferObject(const BufferObject &other);

		/**
		 * Provides info how the buffer object is going to be used.
		 */
		BufferUpdateHint bufferUpdateHint() const { return updateHint_; }

		/**
		 * Set the mapping mode for the buffer object.
		 * Note that mapping will not be possible if the buffer when
		 * map mode is set to BUFFER_MAP_DISABLED.
		 * @param mode the mapping mode to set.
		 */
		void setBufferMapMode(BufferMapMode mode) { mapMode_ = mode; }

		/**
		 * Get the mapping mode for the buffer object.
		 * @return the mapping mode.
		 */
		BufferMapMode bufferMapMode() const { return mapMode_; }

		/**
		 * Set the access mode for the buffer object.
		 * This will determine how the buffer can be accessed by the CPU and GPU.
		 * Note that GPU_ONLY buffers cannot be modified at all by the CPU,
		 * no copying or mapping is possible!
		 * @param mode the access mode to set.
		 */
		void setBufferAccessMode(BufferAccessMode mode);

		/**
		 * Get the access mode for the buffer object.
		 * @return the access mode.
		 */
		BufferAccessMode bufferAccessMode() const { return accessMode_; }

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

		/**
		 * Get the buffer name for a specific index of the allocations.
		 * @param index the index of the allocation.
		 * @return the buffer ID.
		 */
		unsigned int bufferID(unsigned int index = 0) const { return allocations_[index]->bufferID(); }

		/**
		* Copy vertex data to the buffer object. Sets part of data.
		* Replaces only existing data, no new memory allocated for the buffer.
		*/
		static void setBufferData(const void *data, const ref_ptr<BufferReference> &ref);

		/**
		 * Copy client data to the buffer object.
		 * This will replace the existing data in the buffer.
		 * @param data pointer to the data to copy.
		 */
		void setBufferData(const void *data);

		/**
		 * Copy client data to the buffer object.
		 * This will replace the existing data in the buffer.
		 * @param other another buffer object to copy from.
		 */
		void setBufferData(const BufferObject &other);

		/**
		 * Copy part of the data to the buffer object.
		 * This will replace only part of the existing data in the buffer.
		 * @param data pointer to the data to copy.
		 * @param relativeOffset relative offset in bytes from the start of the buffer.
		 * @param dataSize size of the data to copy in bytes.
		 */
		void setBufferSubData(const void *data, GLuint relativeOffset, GLuint dataSize);

		/**
		 * Map the buffer object to CPU memory.
		 * Note that accessFlags must be compatible with the access mode of the buffer.
		 * @param relativeOffset relative offset in bytes from the start of the buffer.
		 * @param mappedSize size of the data to map in bytes.
		 * @param accessFlags access flags for the mapping operation.
		 * @return pointer to the mapped data, or nullptr if mapping failed.
		 */
		GLvoid *map(GLuint relativeOffset, GLuint mappedSize, uint32_t accessFlags);

		/**
		 * Map the buffer object to CPU memory.
		 * Note that accessFlags must be compatible with the access mode of the buffer.
		 * @param accessFlags access flags for the mapping operation.
		 * @return pointer to the mapped data, or nullptr if mapping failed.
		 */
		GLvoid *map(uint32_t accessFlags);

		/**
		 * Map the buffer object to CPU memory.
		 * Note that accessFlags must be compatible with the access mode of the buffer.
		 * @param ref the buffer reference to map.
		 * @param accessFlags access flags for the mapping operation.
		 * @return pointer to the mapped data, or nullptr if mapping failed.
		 */
		static GLvoid *map(const ref_ptr<BufferReference> &ref, uint32_t accessFlags);

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
		static BufferPool *bufferPool(BufferTarget target, BufferStorageMode mode);

	protected:
		BufferUpdateHint updateHint_;
		BufferMapMode mapMode_ = BUFFER_MAP_DISABLED;
		BufferAccessMode accessMode_ = BUFFER_GPU_ONLY;
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
		explicit BufferObjectT(BufferUpdateHint hint) : BufferObject(target, hint) {}
	};
} // namespace

#endif /* REGEN_BUFFER_OBJECT_H_ */
