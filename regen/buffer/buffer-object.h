#ifndef REGEN_BUFFER_OBJECT_H_
#define REGEN_BUFFER_OBJECT_H_

#include "buffer-enums.h"
#include "buffer-reference.h"
#include "regen/glsl/shader-input.h"

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
		BufferObject(BufferTarget target, const BufferUpdateFlags &hints);

		~BufferObject() override;

		/**
		 * Copy constructor. Does not copy GPU data, both objects will share the same buffer.
		 * @param other another buffer object
		 */
		BufferObject(const BufferObject &other);

		/**
		 * Get the buffer target.
		 * @return the buffer target.
		 */
		BufferTarget bufferTarget() const { return flags_.target; }

		/**
		 * Provides info how the buffer object is going to be used.
		 */
		const BufferUpdateFlags& bufferUpdateHints() const { return flags_.updateHints; }

		/**
		 * Get the mapping mode for the buffer object.
		 * @return the mapping mode.
		 */
		BufferMapMode bufferMapMode() const { return flags_.mapMode; }

		/**
		 * Get the access mode for the buffer object.
		 * @return the access mode.
		 */
		ClientAccessMode clientAccessMode() const { return flags_.accessMode; }

		/**
		 * Set the buffer update hint.
		 * This will determine how the buffer can be updated.
		 * @param hint the update hint to set.
		 */
		void setBufferUpdateHint(const BufferUpdateFlags &hints) { flags_.updateHints = hints; }

		/**
		 * Set the mapping mode for the buffer object.
		 * Note that mapping will not be possible if the buffer when
		 * map mode is set to BUFFER_MAP_DISABLED.
		 * @param mode the mapping mode to set.
		 */
		void setBufferMapMode(BufferMapMode mode) { flags_.mapMode = mode; }

		/**
		 * Set the access mode for the buffer object.
		 * This will determine how the buffer can be accessed by the CPU and GPU.
		 * Note that GPU_ONLY buffers cannot be modified at all by the CPU,
		 * no copying or mapping is possible!
		 * @param mode the access mode to set.
		 */
		void setClientAccessMode(ClientAccessMode mode);

		/**
		 * Allocated VRAM in bytes.
		 */
		uint32_t allocatedSize() const { return allocatedSize_; }

		/**
		 * Allocate a block in the VBO memory.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 */
		ref_ptr<BufferReference>& adoptBufferRange(uint32_t numBytes);

		/**
		 * Allocate a block in the VBO memory.
		 * Note that as long as you keep a reference the allocated storage
		 * is marked as used.
		 * @param numBytes the size of the buffer to allocate in bytes.
		 * @param memoryPool the memory pool to use for allocation.
		 * @return a reference to the allocated buffer.
		 */
		static ref_ptr<BufferReference> adoptBufferRange(uint32_t numBytes, BufferPool *memoryPool);

		/**
		 * Orphan previously adopted block of GPU memory.
		 * Actually this will mark the space as free so that others
		 * can allocate it again -- but only if you do not keep a reference
		 * on the Reference instance somewhere. The allocated space is not marked as
		 * free as long as you are referencing the allocated block.
		 */
		static void orphanBufferRange(BufferReference *ref);

		/**
		 * @return the list of all allocated buffers.
		 */
		auto &allocations() const { return allocations_; }

		/**
		 * Get the buffer name for a specific index of the allocations.
		 * @param index the index of the allocation.
		 * @return the buffer ID.
		 */
		uint32_t bufferID(uint32_t index = 0) const { return allocations_[index]->bufferID(); }

		/**
		* Copy vertex data to the buffer object. Sets part of data.
		* Replaces only existing data, no new memory allocated for the buffer.
		*/
		void setBufferData(const void *data, const ref_ptr<BufferReference> &ref);

		/**
		 * Copy client data to the buffer object.
		 * This will replace the existing data in the buffer.
		 * Note: This will copy to main even if the buffer has a staging buffer.
		 * @param data pointer to the data to copy.
		 */
		void setBufferData(const void *data);

		/**
		 * Copy client data to the buffer object.
		 * This will replace the existing data in the buffer.
		 * Note: This will copy to main even if the buffer has a staging buffer.
		 * @param other another buffer object to copy from.
		 */
		void setBufferData(const BufferObject &other);

		/**
		 * Copy data from another buffer object to this one.
		 * Note: This will copy to main even if the buffer has a staging buffer.
		 * @param readBufferID the ID of the buffer to read from.
		 * @param readAddress the address in the buffer to read from.
		 * @param readSize the size of the data to read in bytes.
		 */
		void setBufferData(uint32_t readBufferID, uint32_t readAddress, uint32_t readSize);

		/**
		 * Read data from the buffer object.
		 * This will copy the data from the buffer to the provided pointer.
		 * @param localOffset relative offset in bytes from the start of the buffer.
		 * @param dataSize size of the data to read in bytes.
		 * @param data pointer to the memory where to copy the data.
		 */
		void readBufferSubData(uint32_t localOffset, uint32_t dataSize, byte *data);

		/**
		 * Set all allocated buffers to zero.
		 * This will replace the existing data in the buffer with zeroes.
		 * Note: This will copy to main even if the buffer has a staging buffer.
		 */
		void setBuffersToZero();

		/**
		 * Set the buffer to zero.
		 * This will replace the existing data in the buffer with zeroes.
		 * @param ref the buffer reference to set to zero.
		 */
		void setBufferToZero(const ref_ptr<BufferReference> &ref);

		/**
		 * Copy part of the data to the adopted buffer range of the *main/draw buffer*.
		 * Note: This will copy to main even if the buffer has a staging buffer.
		 * @param data pointer to the data to copy.
		 * @param relativeOffset relative offset in bytes from the start of the buffer.
		 * @param dataSize size of the data to copy in bytes.
		 */
		void setBufferSubData(uint32_t relativeOffset, uint32_t dataSize, const void *data);

		/**
		 * Map the buffer object to CPU memory.
		 * Note that accessFlags must be compatible with the access mode of the buffer.
		 * @param relativeOffset relative offset in bytes from the start of the buffer.
		 * @param mappedSize size of the data to map in bytes.
		 * @param accessFlags access flags for the mapping operation.
		 * @return pointer to the mapped data, or nullptr if mapping failed.
		 */
		void *map(uint32_t relativeOffset, uint32_t mappedSize, uint32_t accessFlags);

		/**
		 * Map the buffer object to CPU memory.
		 * Note that accessFlags must be compatible with the access mode of the buffer.
		 * @param accessFlags access flags for the mapping operation.
		 * @return pointer to the mapped data, or nullptr if mapping failed.
		 */
		void *map(uint32_t accessFlags);

		/**
		 * Map the buffer object to CPU memory.
		 * Note that accessFlags must be compatible with the access mode of the buffer.
		 * @param ref the buffer reference to map.
		 * @param accessFlags access flags for the mapping operation.
		 * @return pointer to the mapped data, or nullptr if mapping failed.
		 */
		static void *map(const ref_ptr<BufferReference> &ref, uint32_t accessFlags);

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
		static void copy(uint32_t from, uint32_t to, uint32_t size, uint32_t offset, uint32_t toOffset);

		/**
		 * Calculates the struct size for the attributes in bytes.
		 */
		static uint32_t attributeSize(const std::list<ref_ptr<ShaderInput> > &attributes);

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
		BufferFlags flags_;
		GLenum glTarget_;

		std::vector<ref_ptr<BufferReference> > allocations_;
		// sum of allocated bytes
		uint32_t allocatedSize_;

		ref_ptr<BufferReference> &adoptBufferRangeInPool(uint32_t numBytes, BufferPool *memoryPool);

		static BufferPool **bufferPools();

		friend struct BufferReference;
	};

	/**
	 * Template class for buffer objects with a specific target.
	 * This allows for easier instantiation of buffer objects with different targets.
	 */
	template<BufferTarget target>
	class BufferObjectT : public BufferObject {
	public:
		explicit BufferObjectT(const BufferUpdateFlags &hints) : BufferObject(target, hints) {}
	};
} // namespace

#endif /* REGEN_BUFFER_OBJECT_H_ */
