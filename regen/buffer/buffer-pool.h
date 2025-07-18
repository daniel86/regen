#ifndef REGEN_BUFFER_POOL_H_
#define REGEN_BUFFER_POOL_H_

#include "regen/utility/memory-allocator.h"

namespace regen {
	/**
	 * \brief Interface for allocating GPU memory.
	 */
	struct BufferAllocator {
		/**
		 * Generate buffer object name and
		 * create and initialize the buffer object's data store to the named size.
		 */
		static GLuint createAllocator(GLuint poolIndex, GLuint size);

		/**
		 * Map the buffer object to the CPU memory.
		 * The poolIndex is used to determine the storage mode.
		 * @param poolIndex the index of the pool.
		 * @param size the size of the buffer to map.
		 * @param ref the reference to the buffer object.
		 * @return pointer to mapped memory, or nullptr if mapping is not possible.
		 */
		static void* mapAllocator(GLuint poolIndex, GLuint size, GLuint ref);

		/**
		 * Unmap the buffer object from the CPU memory.
		 * The poolIndex is used to determine the storage mode.
		 * @param poolIndex the index of the pool.
		 * @param ref the reference to the buffer object.
		 */
		static void unmapAllocator(GLuint poolIndex, GLuint ref);

		/**
		 * Delete named buffer object.
		 * After a buffer object is deleted, it has no contents,
		 * and its name is free for reuse.
		 */
		static void deleteAllocator(GLuint poolIndex, GLuint ref);

		/**
		 * Invalidate a buffer range.
		 * @param ref the reference to the buffer object.
		 * @param offset the offset in the buffer object.
		 * @param size the size of the data to invalidate.
		 */
		static void orphanAllocatorRange(GLuint ref, GLuint offset, GLuint size);
	};

	/**
	 * \brief A pool of VBO memory allocators.
	 */
	typedef AllocatorPool<
			BufferAllocator, GLuint,   // actual allocator and reference type
			BuddyAllocator, GLuint     // virtual allocator and reference type
	> BufferPool;
} // namespace

#endif /* REGEN_BUFFER_POOL_H_ */
