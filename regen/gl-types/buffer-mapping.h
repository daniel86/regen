#ifndef REGEN_BUFFER_MAPPING_H_
#define REGEN_BUFFER_MAPPING_H_

#include <regen/gl-types/gl-object.h>
#include <regen/gl-types/buffer-reference.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
	/**
	 * The mapping flags.
	 */
	enum MappingFlag {
		DYNAMIC_STORAGE = GL_DYNAMIC_STORAGE_BIT,
		MAP_PERSISTENT = GL_MAP_PERSISTENT_BIT,
		MAP_COHERENT = GL_MAP_COHERENT_BIT,
		MAP_READ = GL_MAP_READ_BIT,
		MAP_WRITE = GL_MAP_WRITE_BIT,
		MAP_FLUSH_EXPLICIT = GL_MAP_FLUSH_EXPLICIT_BIT,
		MAP_INVALIDATE_RANGE = GL_MAP_INVALIDATE_RANGE_BIT,
		MAP_INVALIDATE_BUFFER = GL_MAP_INVALIDATE_BUFFER_BIT,
		MAP_UNSYNCHRONIZED = GL_MAP_UNSYNCHRONIZED_BIT,
	};

	/**
	  * The buffering mode, i.e. how many buffers are used.
	  */
	enum BufferingMode {
		SINGLE_BUFFER = 1,
		DOUBLE_BUFFER = 2,
		TRIPLE_BUFFER = 3
	};

	/**
	 * \brief A utility class for mapping buffer objects for reading or writing.
	 *
	 * To avoid synchronization issues, an additional buffer is used that can
	 * be single, double or triple buffered.
	 * If persistent mapping is used, the mapped data will be available by the
	 * pointers provided by this class.
	 */
	class BufferMapping {
	public:
		enum BufferType {
			RING_BUFFER = 0,
			MULTI_BUFFER
		};

		struct RingSegment {
			byte* mappedPtr = nullptr;
			// The offset in the ring buffer where this segment starts, in bytes.
			uint32_t offset = 0;
			GLsync writeFence = nullptr;
			bool hasData = false;
		};

		/**
		 * Ring-buffer mapping constructor.
		 */
		BufferMapping(
				const ref_ptr<BufferReference> &ref,
				GLbitfield storageFlags,
				BufferingMode storageBuffering);

		/**
		 * Multi-buffer mapping constructor.
		 * Storage buffering is derived from the number of references.
		 * @param refs the buffer references to map.
		 * @param storageFlags the OpenGL storage flags, e.g. MAP_READ | MAP_WRITE | MAP_PERSISTENT.
		 */
		BufferMapping(
				const std::vector<ref_ptr<BufferReference>> &refs,
				GLbitfield storageFlags);

		virtual ~BufferMapping();

		// delete copy constructor
		BufferMapping(const BufferMapping &) = delete;

		/**
		 * Sets a flag to allow frame dropping instead of blocking on the fence
		 * if persistent mapping is used.
		 * @param allow true if frame dropping is allowed, false to block on fence.
		 */
		void setAllowFrameDropping(bool allow) { allowFrameDropping_ = allow; }

		/**
		 * \brief Begin writing to the next segment of the ring buffer.
		 * Blocks if the GPU is still using this segment.
		 */
		void* beginWriteBuffer(bool isPartialWrite);

		/**
		 * \brief Finish writing.
		 */
		void endWriteBuffer(BufferRange &nextDrawBuffer);

		/**
		 * Read data from an input buffer reference into client memory.
		 * Note that data may not be available immediately,
		 * check hasReadData() to see if the data is available.
		 */
		void readBuffer(BufferRange &nextDrawBuffer);

		/**
		 * It will take 1-3 frames until the data is available.
		 * @return true if the client data was loaded from storage.
		 */
		bool hasReadData() const { return hasReadData_; }

		/**
		 * @return the current client data, initially all zero.
		 */
		const byte* clientData() const { return storageClientData_; }

	protected:
		std::vector<ref_ptr<BufferReference>> refs_;
		GLbitfield storageFlags_;
		BufferType bufferType_;
		BufferingMode storageBuffering_;
		uint32_t segmentSize_ = 0;
		bool hasReadData_ = false;
		// if true, will not block on write fence, but drop frames instead
		bool allowFrameDropping_ = false;

		std::vector<RingSegment> bufferSegments_;
		byte* mappedRing_ = nullptr;
		byte* storageClientData_ = nullptr;
		int readBufferIndex_ = 0;
		int writeBufferIndex_ = 0;
		int lastReadIndex_ = -1;

		bool initializeMapping();
	};

	/**
	 * \brief A utility class for mapping single structs for reading or writing.
	 */
	template<typename T> class BufferStructMapping : public BufferMapping {
	public:
		BufferStructMapping(
				const ref_ptr<BufferReference> &ref, uint32_t storageFlags,
				BufferingMode storageBuffering = DOUBLE_BUFFER)
				: BufferMapping(ref, storageFlags, storageBuffering) {
		}

		BufferStructMapping(
				const std::vector<ref_ptr<BufferReference>> &refs, GLbitfield storageFlags)
				: BufferMapping(refs, storageFlags) {
		}

		const T& storageValue() {
			return *((const T*)storageClientData_);
		}
	};

	/**
	 * \brief A utility class for mapping arrays of structs for reading or writing.
	 */
	template<typename T> class BufferArrayMapping : public BufferMapping {
	public:
		BufferArrayMapping(
					const ref_ptr<BufferReference> &ref,
					uint32_t storageFlags,
					BufferingMode storageBuffering = DOUBLE_BUFFER)
				: BufferMapping(ref, storageFlags, storageBuffering) {
		}

		BufferArrayMapping(
				const std::vector<ref_ptr<BufferReference>> &refs, GLbitfield storageFlags)
				: BufferMapping(refs, storageFlags) {
		}

		const T* storageArray() {
			return ((const T*)storageClientData_);
		}
	};
} // namespace

#endif /* REGEN_BUFFER_MAPPING_H_ */
