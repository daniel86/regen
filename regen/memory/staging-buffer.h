#ifndef REGEN_STAGING_BUFFER_H_
#define REGEN_STAGING_BUFFER_H_

#include "regen/gl/gl-object.h"
#include "buffer-reference.h"
#include "buffer-enums.h"
#include "buffer-object.h"
#include "regen/shader/shader-input.h"
#include "regen/gl/gpu-fence.h"

namespace regen {
	/**
	 * \brief A staging buffer is used to store data that is accessed by the CPU.
	 *
	 * This implementation supports different modes for buffering, including
	 * adaptive ring buffers, and fixed-size buffers.
	 * Furthermore, implicit and explicit staging modes are supported,
	 * where in explicit mode, a separate staging buffer is used,
	 * and in implicit mode, the data is written directly to the draw buffer.
	 * However, in implicit mode, the draw buffer can be multi-buffered too,
	 * however, the buffer range adoption must then be done manually.
	 * Finally, we support here "global" as well as "local" staging buffers,
	 * where local ones are managed per buffer object while global ones
	 * are managed by the staging system.
	 */
	class StagingBuffer {
	public:
		// The maximum acceptable stall rate for the staging buffer.
		// In case of adaptive ring buffers, this is the threshold for increasing the number of ring segments.
		// 0.1 might be a good value, meaning that 10% of frames can stall.
		static float MAX_ACCEPTABLE_STALL_RATE;
		static uint32_t MIN_SIZE_MEDIUM;
		static uint32_t MIN_SIZE_LARGE;
		static uint32_t MIN_SIZE_VERY_LARGE;

		/**
		 * Set the minimum size for medium sized buffers.
		 * @param size the minimum size in bytes.
		 */
		static void setMediumBufferMinSize(uint32_t size) { MIN_SIZE_MEDIUM = size; }

		/**
		 * Set the minimum size for large sized buffers.
		 * @param size the minimum size in bytes.
		 */
		static void setLargeBufferMinSize(uint32_t size) { MIN_SIZE_LARGE = size; }

		/**
		 * Set the minimum size for very large sized buffers.
		 * @param size the minimum size in bytes.
		 */
		static void setVeryLargeBufferMinSize(uint32_t size) { MIN_SIZE_VERY_LARGE = size; }

		/**
		 * Compute the size class for the given size in bytes.
		 * The classification is done specifically for staging buffers.
		 * @param size the size in bytes.
		 * @return the size class.
		 */
		static BufferSizeClass getBufferSizeClass(uint32_t size);

		/**
		 * Default constructor.
		 * @param bufferFlags the flags used for staging.
		 */
		explicit StagingBuffer(const BufferFlags &bufferFlags);

		virtual ~StagingBuffer();

		// delete copy constructor
		StagingBuffer(const StagingBuffer &) = delete;

		/**
		 * @return the buffer flags used for staging.
		 */
		const BufferFlags &stagingFlags() const { return flags_; }

		/**
		 * Set whether the buffer should swap on each access.
		 * @param v true if the buffer should swap on each access, false otherwise.
		 */
		void setSwappingOnAccess(bool v) { useSwappingOnAccess_ = v; };

		/**
		 * Set whether the buffer should clear its segments on resize.
		 * Default is true. When not being cleared, then the segments may contain garbage data
		 * the first frames. By default all values will be cleared to zero (a little better than garbage, maybe).
		 * However, best to rotate through segments to avoid artifacts in first frames.
		 * @param v true if the buffer should clear its segments on resize, false otherwise.
		 */
		void setClearBufferOnResize(bool v) { clearBufferOnResize_ = v; }

		/**
		 * Advance read and write indices to the next segments.
		 * This is used to switch the buffers after writing or reading.
		 */
		void swapBuffers();

		/**
		 * Resize the buffer to the given segment size and number of ring segments.
		 * This may reallocate the buffer segments and reset the read/write indices.
		 * @param segmentSize the size of each segment in bytes.
		 * @param initialNumRingSegments the initial number of ring segments.
		 * @return true if the buffer was resized successfully, false otherwise.
		 */
		bool resizeBuffer(uint32_t segmentSize, uint32_t initialNumRingSegments);

		/**
		 * Check if the buffer has adopted a range for staging.
		 * This is true if the buffer is using explicit staging and has a valid staging reference
		 * obtained by calling resizeBuffer().
		 * @return true if the buffer has adopted a range, false otherwise.
		 */
		bool hasAdoptedRange() { return stagingRef_.get() != nullptr; }

		/**
		 * The size of each buffer segment in bytes.
		 * @return the size of each segment in bytes.
		 */
		uint32_t segmentSize() const { return segmentSize_; }

		/**
		 * The number of buffer segments in the ring buffer.
		 * This is the number of ring segments in case of RING_BUFFER buffering mode,
		 * or e.g. 2 for DOUBLE_BUFFER.
		 * @return the number of buffer segments.
		 */
		uint32_t numBufferSegments() const { return static_cast<uint32_t>(bufferSegments_.size()); }

		/**
		 * The maximum number of ring segments in the ring buffer.
		 * This is used for adaptive resizing of the ring buffer.
		 * Only used in case of RING_BUFFER buffering mode.
		 * @return the maximum number of ring segments.
		 */
		uint32_t maxRingSegments() const { return maxRingSegments_; }

		/**
		 * Set the maximum number of ring segments in the ring buffer.
		 * This is used for adaptive resizing of the ring buffer.
		 * Only used in case of RING_BUFFER buffering mode.
		 * @param maxSegments the maximum number of ring segments to set.
		 */
		void setMaxRingSegments(uint32_t maxSegments) { maxRingSegments_ = maxSegments; }

		/**
		 * @return the buffer segment index that will next be written to.
		 */
		uint32_t nextWriteIndex() const { return writeBufferIndex_; }

		/**
		 * @return the buffer segment index that will next be read from.
		 */
		uint32_t nextReadIndex() const { return readBufferIndex_; }

		/**
		 * Get the stall rate of the current write segment.
		 * @return the stall rate as a percentage of frames that stalled.
		 */
		float getStallRate() const;

		/**
		 * Reset the stall rate of all segments in the buffer.
		 * This is used to reset the stall history of each fence.
		 */
		void resetStallRate();

		/**
		 * Mark the draw buffer as accessed.
		 * This is used to mark the segments that were accessed by the GPU.
		 * It is a no-op if the buffer is not using implicit staging + persistent mapping.
		 * @param drawBuffer the draw buffer range that was accessed.
		 */
		void markDrawAccessed(BufferRange &drawBuffer);

		/**
		 * Get the current staging buffer reference.
		 * This is used to access the CPU-accessible storage buffer.
		 * @return a reference to the staging buffer.
		 */
		inline ref_ptr<BufferReference> stagingRef() { return stagingRef_; }

		/**
		 * Get the offset in the ring buffer for the specified segment index.
		 * @param segmentIdx the index of the segment to get the offset for.
		 * @return the offset in bytes for the specified segment index.
		 */
		inline uint32_t segmentOffset(uint32_t segmentIdx) const { return bufferSegments_[segmentIdx].offset; }

		/**
		 * Push the dirty segments to the flush queue.
		 * This is used to mark the segments that need to be flushed to the GPU.
		 * It is a no-op if the buffer is not using explicit flushing.
		 * @param dirtySegments the dirty segments to push to the flush queue.
		 * @param numDirtySegments the number of dirty segments.
		 */
		void pushToFlushQueue(const BufferRange2ui *dirtySegments, uint32_t numDirtySegments);

		/**
		 * \brief Begin writing to the next segment of the ring buffer.
		 * Blocks if the GPU is still using this segment.
		 */
		byte *beginMappedWrite(
				const ref_ptr<BufferReference> &drawBufferRef,
				bool isPartialWrite,
				uint32_t localOffset,
				uint32_t mappedSize);

		/**
		 * \brief Finish writing.
		 */
		void endMappedWrite(
				const ref_ptr<BufferReference> &drawBufferRef,
				BufferRange &nextDrawBuffer,
				uint32_t localSegmentOffset);

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
		void endNonMappedWrite(
				const ref_ptr<BufferReference> &drawBufferRef,
				BufferRange &nextDrawBuffer,
				uint32_t localSegmentOffset);

		/**
		 * Set sub-data in the current write segment.
		 * This will copy the data to the mapped pointer, or to the buffer directly
		 * if no mapping is used.
		 * This is only allowed in between beginWrite() and endWrite().
		 * @param localOffset the offset in the segment where to write the data.
		 * @param dataSize the size of the data to write.
		 * @param data pointer to the data to write.
		 */
		void setSubData(
				const ref_ptr<BufferReference> &drawBufferRef,
				uint32_t localOffset,
				uint32_t dataSize,
				const byte *data);

		/**
		 * Read data from an input buffer reference into client memory.
		 * Note that data may not be available immediately,
		 * check hasReadData() to see if the data is available.
		 */
		bool readBuffer(
				const ref_ptr<BufferReference> &drawBufferRef,
				BufferRange &nextDrawBuffer,
				uint32_t localOffset);

		/**
		 * It will take 1-3 frames until the data is available.
		 * @return true if the client data was loaded from storage.
		 */
		bool hasReadData() const { return bufferSegments_[readBufferIndex_].hasData; }

		/**
		 * @return the current client data, initially all zero.
		 */
		const byte *readData() const { return stagingReadData_; }

		/**
		 * \brief Get the fence of the specified segment.
		 * @param segmentIndex the index of the segment to get the fence for.
		 * @return a reference to the GPUFence of the specified segment.
		 */
		GPUFence &fence(uint32_t segmentIndex) { return bufferSegments_[segmentIndex].fence; }

		/**
		 * \brief Check if the fence of the specified segment is signaled.
		 * This is used to check if the GPU has finished processing the segment.
		 * @param segmentIndex the index of the segment to check.
		 * @return true if the fence is signaled, false otherwise.
		 */
		bool isFenceSignaled(uint32_t segmentIndex) {
			return bufferSegments_[segmentIndex].fence.isSignaled();
		}

	protected:
		const BufferFlags flags_;
		// buffer references to cpu-accessible storage
		ref_ptr<BufferReference> stagingRef_;
		BufferCopyRange stagingCopyRange_;
		// separate storage buffer for CPU access, if used
		ref_ptr<BufferObject> stagingBO_;
		const BufferStorageMode storageMode_;
		// flags used to configure storage access.
		const GLbitfield storageFlags_;
		const GLbitfield accessFlags_;
		// the size of each segment in bytes in case of multi-buffering
		uint32_t segmentSize_ = 0;

		// if true then each read/write access automatically swaps the buffers.
		bool useSwappingOnAccess_ = true;
		// if true then the buffer will be cleared on resize.
		bool clearBufferOnResize_ = true;
		// maximum number of ring segments in the ring buffer for adaptive resizing.
		uint32_t maxRingSegments_ = 16u;

		struct RingSegment {
			// The offset in the ring buffer where this segment starts, in bytes.
			uint32_t offset = 0;
			GPUFence fence;
			std::vector<BufferRange2ui> dirtySegments;
			uint32_t numDirtySegments = 0;
			bool hasData = false;
		};
		std::vector<RingSegment> bufferSegments_;
		byte *stagingReadData_ = nullptr;
		uint32_t readBufferIndex_ = 0u;
		uint32_t writeBufferIndex_ = 0u;

		static BufferPool *getStagingAllocator(BufferStorageMode storageMode);

		static byte *getMappedSegment(
				const ref_ptr<BufferReference> &ref,
				uint32_t segmentOffset);
	};

	/**
	 * \brief A utility class for mapping single structs for reading or writing.
	 */
	template<typename T>
	class StagingStructBuffer : public StagingBuffer {
	public:
		explicit StagingStructBuffer(const BufferFlags &flags) : StagingBuffer(flags) {}

		/**
		 * @return a reference to the typed storage value.
		 */
		const T &stagingReadValue() {
			return *((const T *) stagingReadData_);
		}
	};

	/**
	 * \brief A utility class for mapping arrays of structs for reading or writing.
	 */
	template<typename T>
	class StagingArrayBuffer : public StagingBuffer {
	public:
		explicit StagingArrayBuffer(const BufferFlags &flags) : StagingBuffer(flags) {}

		/**
		 * @return a reference to the typed storage array.
		 */
		const T *stagingReadValue() {
			return ((const T *) stagingReadData_);
		}
	};
} // namespace

#endif /* REGEN_STAGING_BUFFER_H_ */
