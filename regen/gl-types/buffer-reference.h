#ifndef REGEN_BUFFER_REFERENCE_H_
#define REGEN_BUFFER_REFERENCE_H_

#include <list>
#include <regen/scene/resource.h>
#include <regen/utility/ref-ptr.h>
#include <regen/gl-types/buffer-pool.h>

namespace regen {
	/**
	 * \brief Reference to allocated data.
	 */
	struct BufferReference {
		BufferReference() = default;

		~BufferReference();

		// no copy allowed
		BufferReference(const BufferReference &) = delete;

		/**
		 * @return true if this reference is not associated to an allocated block.
		 */
		bool isNullReference() const { return allocatedSize_ == 0u; }

		/**
		 * @return the allocated block size.
		 */
		unsigned int allocatedSize() const { return allocatedSize_; }

		/**
		 * @return the size of the full buffer, i.e. the size of the node in the allocator pool.
		 * This is not the same as allocatedSize() which returns the size of the allocated block.
		 */
		unsigned int fullBufferSize() const { return poolReference_.allocatorNode->allocator.size(); }

		/**
		 * @return virtual address to allocated block.
		 */
		unsigned int address() const;

		/**
		 * @return buffer object name.
		 */
		unsigned int bufferID() const;

		/**
		 * @return The associated VBO.
		 */
		Resource *bufferObject() const { return bufferObject_; }

	private:
		Resource *bufferObject_ = nullptr;
		BufferPool::Reference poolReference_ = {};
		unsigned int allocatedSize_ = 0;

		friend class BufferObject;
	};
} // namespace

#endif /* REGEN_BUFFER_REFERENCE_H_ */
