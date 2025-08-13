#ifndef REGEN_CLIENT_BUFFER_H_
#define REGEN_CLIENT_BUFFER_H_

#include <atomic>
#include <array>
#include <vector>
#include <thread>
#include <regen/regen.h>
#include <regen/utility/ref-ptr.h>
#include <regen/buffer/client-data-base.h>
#include <regen/utility/dirty-list.h>
#include "buffer-enums.h"

namespace regen {
	/**
	 * \brief A client buffer that can be used to store data on the client side.
	 * This class is used to manage client-side data for shader inputs.
	 * It supports double-buffering and provides thread-safe methods for mapping and unmapping data
	 * for reading and writing. Reading will never block, but only one thread can write to the data at a time.
	 *
	 * The buffer can be frame-locked, meaning that the written data is only flushe once per frame,
	 * i.e. made available for reading in the next frame.
	 */
	class ClientBuffer {
	public:
		/**
		 * @brief The mode of buffering for the client buffer.
		 */
		enum Mode { SingleBuffer, DoubleBuffer, AdaptiveBuffer };

		/**
		 * @brief Constructs a client buffer with the specified mode.
		 * @param clientBufferMode the mode of buffering to use.
		 */
		ClientBuffer();

		virtual ~ClientBuffer();

		ClientBuffer(const ClientBuffer &) = delete;

		/**
		 * @brief Sets the memory layout of the client buffer.
		 * @param layout the memory layout to use.
		 */
		void setMemoryLayout(BufferMemoryLayout layout) { memoryLayout_ = layout; }

		/**
		 * @return the memory layout of the client buffer.
		 */
		BufferMemoryLayout memoryLayout() const { return memoryLayout_; }

		/**
		 * @return the mode of buffering for the client buffer.
		 */
		Mode clientBufferMode() const { return clientBufferMode_; }

		/**
		 * Sets the mode of buffering for the client buffer.
		 * @param mode the mode of buffering to use.
		 */
		void setClientBufferMode(Mode mode) { clientBufferMode_ = mode; }

		/**
		 * @return true if client data is available, i.e. the first data slot is not null.
		 */
		inline bool hasClientData() const { return dataSlots_[0] != nullptr; }

		/**
		 * @return true if client data is available in the second slot, i.e. double-buffering is used.
		 */
		bool hasTwoSlots() const { return dataSlots_[1] != nullptr; }

		/**
		 * @return true if the buffer data is owned by this instance.
		 */
		inline bool isDataOwner() const { return dataOwner_ == this; }

		/**
		 * Frame-locked buffers will not swap the data slots after a write operation,
		 * but only once per frame.
		 * @return true if the buffer is frame-locked.
		 */
		inline bool isFrameLocked() const { return isFrameLocked_; }

		/**
		 * Sets whether the buffer is frame-locked.
		 * If true, the data will not be swapped after each write operation,
		 * but only once per frame.
		 * @param frameLocked true if the buffer is frame-locked.
		 */
		void setFrameLocked(bool frameLocked);

		/**
		 * @return the size of the data in bytes (for a single slot).
		 */
		uint32_t dataSize() const { return dataSize_; }

		/**
		 * A block of this client buffer must start at a multiple of this alignment.
		 * @return the base alignment of the client buffer.
		 */
		uint32_t baseAlignment() const { return baseAlignment_; }

		/**
		 * Sets the base alignment of the client buffer.
		 * This is used to align the data in the buffer to a specific boundary.
		 * @param alignment the base alignment in bytes.
		 */
		void setBaseAlignment(uint32_t alignment) { baseAlignment_ = alignment; }

		/**
		 * Obtains the client data without locking.
		 * Be sure that no other thread is writing to the data at the same time.
		 * @return the client data.
		 */
		byte *clientData() const { return dataSlots_[lastDataSlot()]; }

		/**
		 * Obtains the client data for a specific slot without locking.
		 * Be sure that no other thread is writing to the data at the same time.
		 * If you intend to write initial data at construction, use the first slot (0).
		 * @param slot the slot index (0 or 1).
		 * @return the client data for the specified slot.
		 */
		byte *clientData(uint32_t slot) const { return dataSlots_[slot]; }

		/**
		 * @return the stamp of the data that is currently being written to.
		 */
		inline uint32_t stampOfWriteData() const { return dataStamps_[1 - lastDataSlot()]; }

		/**
		 * @return the stamp of the data that is currently being read.
		 */
		inline uint32_t stampOfReadData() const { return dataStamps_[lastDataSlot()]; }

		/**
		 * Returns the current read slot index.
		 * @return the current read slot index (0 or 1).
		 */
		inline uint32_t currentReadSlot() const {
			return dataOwner_->lastDataSlot_.load(std::memory_order_acquire);
		}

		/**
		 * Returns the current write slot index.
		 * This is the slot that will be written to next.
		 * @return the current write slot index (0 or 1).
		 */
		inline uint32_t currentWriteSlot() const {
			return 1 - currentReadSlot();
		}

		/**
		 * Increment the stamp.
		 */
		void nextStamp(uint32_t slotIdx) const;

		/**
		 * Assigns a list of segments to this client buffer, replacing any existing segments.
		 * This makes this client buffer a composed client buffer, that manages multiple segments
		 * in contiguous memory.
		 * @param segments the list of segments to assign.
		 */
		void setSegments(const std::vector<ref_ptr<ClientBuffer>> &segments);

		/**
		 * Adds a segment to this client buffer.
		 * This makes this client buffer a composed client buffer, that manages multiple segments
		 * in contiguous memory.
		 * @param segment the segment to add.
		 */
		void addSegment(const ref_ptr<ClientBuffer> &segment);

		/**
		 * Removes a segment from this client buffer.
		 * This makes this client buffer a composed client buffer, that manages multiple segments
		 * in contiguous memory.
		 * @param segment the segment to remove.
		 */
		void removeSegment(const ref_ptr<ClientBuffer> &segment);

		/**
		 * Checks if this client buffer has segments.
		 * @return true if the client buffer has segments, false otherwise.
		 */
		inline bool hasSegments() const { return !bufferSegments_.empty(); }

		/**
		 * Checks if this client buffer has a parent buffer.
		 * @return true if the client buffer has a parent buffer, false otherwise.
		 */
		inline bool hasParentBuffer() const { return parentBuffer_ != nullptr; }

		/**
		 * Maps the client data for reading or writing.
		 * @param mapMode the mapping mode, i.e. a ClientMappingMode flag.
		 * @param offset the offset in bytes from the start of the buffer.
		 * @param size the size in bytes to map.
		 * @return a MappedClientData object containing the mapped data.
		 */
		MappedClientData mapRange(
				int32_t mapMode,
				uint32_t offset,
				uint32_t size) const;

		/**
		 * Unmaps the client data after it has been mapped for writing.
		 * @param mapMode the mapping mode, i.e. a ClientMappingMode flag.
		 * @param offset the offset in bytes from the start of the buffer.
		 * @param size the size in bytes that was written.
		 * @param slotIndex the index of the data slot that was written to (0 or 1).
		 */
		void unmapRange(
				int32_t mapMode,
				uint32_t offset,
				uint32_t size,
				int32_t slotIndex) const;

		/**
		 * Marks the data as written to in the current frame.
		 * This is used to track which data ranges were written to in the current frame,
		 * so that they can be flushed to the staging buffer later.
		 * It is assumed the data was mapped for writing before this call.
		 * @param slotIdx the index of the data slot (0 or 1).
		 * @param offset the offset in bytes from the start of the buffer.
		 * @param size the size in bytes that was written.
		 */
		void markWrittenTo(uint32_t slotIdx, uint32_t offset, uint32_t size) const;

		/**
		 * Resize the client buffer.
		 * @param bufferSize the new size of the buffer in bytes.
		 * @param initialData optional initial data to fill the buffer with.
		 */
		void resize(size_t bufferSize, const byte *initialData = nullptr);

		/**
		 * Deallocates data pointer owned by this instance.
		 * If no data is owned by this instance, iit will just set the data pointers to nullptr.
		 * This is e.g. used if vertex data is static and only initially uploaded to the GPU.
		 */
		void deallocateClientData();

		/**
		 * Swaps the data between the two slots.
		 * This will first ensure that the current write slot has all the most recent data,
		 * and secondly, it swaps the read and write slots.
		 */
		uint32_t swapData(bool force=false);

		/**
		 * Locks all data slots, effectively preventing any read or write access
		 * to the data until the locks are released.
		 * This is useful for operations that need to ensure no other thread
		 * is accessing the data while it is being modified, e.g. during a resize operation.
		 */
		void writeLockAll() const;

		/**
		 * This will release the locks on all data slots, allowing other threads
		 * to access the data again.
		 * @param writeOffset the offset in bytes from the start of the buffer that was written to.
		 * @param writeSize the size in bytes that was written.
		 */
		void writeUnlockAll(uint32_t writeOffset, uint32_t writeSize) const;

	protected:
		Mode clientBufferMode_ = AdaptiveBuffer;
		uint32_t dataSize_ = 0u;
		uint32_t allocatedSize_ = 0u;
		// absolute offset wrt. the data owner.
		uint32_t dataOffset_ = 0u;
		uint32_t lastOffset_ = 0u;
		uint32_t baseAlignment_ = 1u;
		bool isFrameLocked_ = false;
		BufferMemoryLayout memoryLayout_ = BUFFER_MEMORY_PACKED;

		// Note: marked as mutable because client data mapping must be allowed in const functions
		//       for reading data, but mapping interacts with locks. Hence, locks must be mutable.
		mutable std::array<byte *, 2> dataSlots_ = {nullptr, nullptr};
		// the instance that owns the data slots, i.e. either this instance or a parent buffer.
		mutable ClientBuffer* dataOwner_;

		// active slot for readers
		mutable std::atomic<int> lastDataSlot_{0};
		// per-slot reader/writer count
		std::atomic<uint32_t> readerCounts_[2] = {0u, 0u};
		// protects against simultaneous writers
		std::atomic_flag writerFlags_[2] = {ATOMIC_FLAG_INIT, ATOMIC_FLAG_INIT};
		std::thread::id writerThreads_[2] = {std::thread::id(), std::thread::id()};
		// indicator to writes to the data slots
		mutable uint32_t dataStamps_[2] = {0u,0u};
		// stores the ranges written to in the current and last frame if frame-locked
		DirtyList dirtyLists_[2] = {};

		ClientBuffer* parentBuffer_ = nullptr;
		std::vector<ref_ptr<ClientBuffer>> bufferSegments_;

		int readLock() const;

		bool readLock_SingleBuffer() const;

		void readUnlock(int slotIndex) const;

		int writeLock_DoubleBuffer() const;

		bool writeLock_SingleBuffer() const;

		void writeUnlock(int32_t slotIndex, uint32_t writeOffset, uint32_t writeSize) const;

		MappedClientData writeRange_SingleBuffer(uint32_t offset, uint32_t size) const;

		MappedClientData writeRange_DoubleBuffer(uint32_t offset, uint32_t size) const;

		MappedClientData readRange_SingleBuffer(uint32_t offset, uint32_t size) const;

		MappedClientData readRange_DoubleBuffer(uint32_t offset, uint32_t size) const;

		int lastDataSlot() const;

		void nextStamp() const;

		bool isCurrentThreadOwnerOfWriteLock(int dataSlot) const;

		void setOwnerOfWriteLock(int dataSlot) const;

		void nextSegmentStamp(uint32_t dataSlot, uint32_t updatedOffset, uint32_t updateSize) const;

		void createSecondSlot();

		void setDataPointer(ClientBuffer *owner, byte *dataPtr, uint32_t slotIdx) const;

		void ownerResize();

		void updateBufferSize();

		void resize_SingleBuffer(
				ClientBuffer *owner,
				const byte *oldDataPtr,
				byte *newDataPtr);

		void resize_DoubleBuffer(
				ClientBuffer *owner,
				const byte *oldDataPtr0,
				const byte *oldDataPtr1,
				byte *newDataPtr0,
				byte *newDataPtr1);
	};
} // namespace

#endif /* REGEN_CLIENT_BUFFER_H_ */
