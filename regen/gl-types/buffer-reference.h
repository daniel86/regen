#ifndef REGEN_BUFFER_REFERENCE_H_
#define REGEN_BUFFER_REFERENCE_H_

#include <list>
#include <regen/regen.h>
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
		uint32_t allocatedSize() const { return allocatedSize_; }

		/**
		 * @return the size of the full buffer, i.e. the size of the node in the allocator pool.
		 * This is not the same as allocatedSize() which returns the size of the allocated block.
		 */
		uint32_t fullBufferSize() const { return poolReference_.allocatorNode->allocator.size(); }

		/**
		 * @return virtual address to allocated block.
		 */
		uint32_t address() const;

		/**
		 * @return buffer object name.
		 */
		uint32_t bufferID() const;

		/**
		 * @return The associated BufferObject resource.
		 */
		Resource *bufferObject() const { return bufferObject_; }

		/**
		 * @return mapped data pointer, if any.
		 */
		byte* mappedData() const { return mappedData_; }

		/**
		 * @return a null reference.
		 */
		static ref_ptr<BufferReference> &nullReference();

	private:
		Resource *bufferObject_ = nullptr;
		BufferPool::Reference poolReference_ = {};
		uint32_t allocatedSize_ = 0u;
		byte *mappedData_ = nullptr; // pointer to mapped data, if any

		friend class BufferObject;
	};
} // namespace

#endif /* REGEN_BUFFER_REFERENCE_H_ */
