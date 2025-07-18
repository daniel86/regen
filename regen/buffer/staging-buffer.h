#ifndef REGEN_STAGING_BUFFER_H_
#define REGEN_STAGING_BUFFER_H_

#include "regen/gl-types/gl-object.h"
#include "buffer-reference.h"
#include "buffer-enums.h"
#include "buffer-object.h"
#include "regen/gl-types/shader-input.h"
#include "regen/gl-types/gpu-fence.h"

namespace regen {
	/**
	 * \brief A utility class for mapping buffer objects for reading or writing.
	 *
	 * To avoid synchronization issues, an additional buffer is used that can
	 * be single, double or triple buffered.
	 * If persistent mapping is used, the mapped data will be available by the
	 * pointers provided by this class.
	 */
	class StagingBuffer {
	public:
		enum BufferType {
			RING_BUFFER = 0,
			MULTI_BUFFER
		};

		/**
		 * Default constructor.
		 * If separate buffers are used, then a separate storage buffer is created
		 * for writing/reading on CPU side.
		 * In the other case, the write will be done directly on the draw buffer,
		 * which in this case should be a ring buffer.
		 * Default case is separation of buffers.
		 */
		StagingBuffer(
			const ref_ptr<BufferReference> &drawBufferRef,
			const BufferFlags &bufferFlags,
			BufferType bufferType = RING_BUFFER);

		virtual ~StagingBuffer();

		// delete copy constructor
		StagingBuffer(const StagingBuffer &) = delete;

		/**
		 * @return the buffer flags used for staging.
		 */
		const BufferFlags& stagingFlags() const { return flags_; }

		/**
		 * @return the size of each multi-buffered segment segment in bytes.
		 */
		uint32_t segmentSize() const { return segmentSize_; }

		/**
		 * @return the buffer segment index that is currently being written to.
		 */
		uint32_t nextWriteIndex() const { return writeBufferIndex_; }

		/**
		 * @return the buffer segment index that is currently being read from.
		 */
		uint32_t nextReadIndex() const { return readBufferIndex_; }

		/**
		 * Mark the next draw buffer as accessed for drawing.
		 * @param drawBuffer the buffer range to mark as accessed.
		 */
		void markDrawAccessed(BufferRange &drawBuffer);

		/**
		 * \brief Begin writing to the next segment of the ring buffer.
		 * Blocks if the GPU is still using this segment.
		 */
		void* beginMappedWrite(bool isPartialWrite, uint32_t localOffset, uint32_t mappedSize);

		/**
		 * \brief Finish writing.
		 */
		void endMappedWrite(BufferRange &nextDrawBuffer);

		/**
		 * \brief Begin writing without mapping.
		 * This is used for single-buffering mode, where no mapping is used.
		 * @return a pointer to the mapped data, or nullptr if the buffer is not mapped.
		 */
		void beginNonMappedWrite();

		/**
		 * \brief Finish writing without mapping.
		 * This is used for single-buffering mode, where no mapping is used.
		 */
		void endNonMappedWrite(BufferRange &nextDrawBuffer);

		/**
		 * Set sub-data in the current write segment.
		 * This will copy the data to the mapped pointer, or to the buffer directly
		 * if no mapping is used.
		 * This is only allowed in between beginWrite() and endWrite().
		 * @param localOffset the offset in the segment where to write the data.
		 * @param dataSize the size of the data to write.
		 * @param data pointer to the data to write.
		 */
		void setSubData(uint32_t localOffset, uint32_t dataSize, const void *data);

		/**
		 * Push the dirty segments to the flush queue.
		 * This is used to mark the segments that need to be flushed to the GPU.
		 * It is a no-op if the buffer is not using explicit flushing.
		 * @param dirtySegments the dirty segments to push to the flush queue.
		 * @param numDirtySegments the number of dirty segments.
		 */
		void pushToFlushQueue(const BufferRange2ui *dirtySegments, uint32_t numDirtySegments);

		/**
		 * Read data from an input buffer reference into client memory.
		 * Note that data may not be available immediately,
		 * check hasReadData() to see if the data is available.
		 */
		bool readBuffer(BufferRange &nextDrawBuffer);

		/**
		 * It will take 1-3 frames until the data is available.
		 * @return true if the client data was loaded from storage.
		 */
		bool hasReadData() const { return hasReadData_; }

		/**
		 * @return the current client data, initially all zero.
		 */
		const byte* readData() const { return stagingReadData_; }

	protected:
		const BufferFlags flags_;
		// the storage used for GPU-side access
		ref_ptr<BufferReference> refGPU_;
		// buffer references to cpu-accessible storage
		std::vector<ref_ptr<BufferReference>> refsCPU_;
		// separate storage buffer for CPU access, if used
		ref_ptr<BufferObject> bufferCPU_;
		const BufferStorageMode storageMode_;
		// flags used to configure storage access.
		const GLbitfield storageFlags_;
		const GLbitfield accessFlags_;
		const BufferType bufferType_ = RING_BUFFER;
		// the size of each segment in bytes in case of multi-buffering
		uint32_t segmentSize_ = 0;
		// true is read data is available
		bool hasReadData_ = false;

		struct RingSegment {
			byte* mappedPtr = nullptr;
			// The offset in the ring buffer where this segment starts, in bytes.
			uint32_t offset = 0;
			GPUFence writeFence;
			GPUFence readFence;
			std::vector<BufferRange2ui> dirtySegments;
			uint32_t numDirtySegments = 0;
			bool hasData = false;
		};
		std::vector<RingSegment> bufferSegments_;
		byte* mappedRing_ = nullptr;
		byte* stagingReadData_ = nullptr;
		uint32_t readBufferIndex_ = 0u;
		uint32_t writeBufferIndex_ = 0u;

		void swapBuffers();

		bool initializeMapping();
	};

	/**
	 * \brief A utility class for mapping single structs for reading or writing.
	 */
	template<typename T> class StagingStructBuffer : public StagingBuffer {
	public:
		StagingStructBuffer(const ref_ptr<BufferReference> &ref, const BufferFlags &flags)
				: StagingBuffer(ref, flags) {
		}
		/**
		 * @return a reference to the typed storage value.
		 */
		const T& stagingReadValue() {
			return *((const T*)stagingReadData_);
		}
	};

	/**
	 * \brief A utility class for mapping arrays of structs for reading or writing.
	 */
	template<typename T> class StagingArrayBuffer : public StagingBuffer {
	public:
		StagingArrayBuffer(const ref_ptr<BufferReference> &ref, const BufferFlags &flags)
				: StagingBuffer(ref, flags) {
		}
		/**
		 * @return a reference to the typed storage array.
		 */
		const T* stagingReadValue() {
			return ((const T*)stagingReadData_);
		}
	};
} // namespace

#endif /* REGEN_STAGING_BUFFER_H_ */
