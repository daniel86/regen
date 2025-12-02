#ifndef REGEN_BUFFER_REFERENCE_H_
#define REGEN_BUFFER_REFERENCE_H_

#include <list>
#include "regen/regen.h"
#include "regen/scene/resource.h"
#include "regen/utility/ref-ptr.h"
#include "buffer-allocator.h"

#ifndef REGEN_BUFFER_OFFSET
#define REGEN_BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

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

	struct BufferRange2ui {
		uint32_t offset = 0; // offset in the buffer
		uint32_t size = 0; // size of the range in bytes
	};

	/**
	 * \brief A range of data to copy from one buffer to another.
	 * This is used for scheduling buffer copies in the staging system.
	 */
	struct BufferCopyRange {
		// The buffer name of the source buffer.
		uint32_t srcBufferID = 0;
		// The buffer name of the destination buffer.
		uint32_t dstBufferID = 0;
		// The offset in the source buffer in bytes.
		uint32_t srcOffset = 0;
		// The offset in the destination buffer in bytes.
		uint32_t dstOffset = 0;
		// The size of the data to copy in bytes.
		uint32_t size = 0;
	};

	std::ostream &operator<<(std::ostream &out, const BufferRange2ui &v);

	std::ostream &operator<<(std::ostream &out, const BufferCopyRange &v);
} // namespace

#endif /* REGEN_BUFFER_REFERENCE_H_ */
