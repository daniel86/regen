#ifndef REGEN_CLIENT_BUFFER_H_
#define REGEN_CLIENT_BUFFER_H_

#include <atomic>
#include <array>
#include <vector>
#include <regen/regen.h>
#include <regen/utility/ref-ptr.h>
#include <regen/buffer/client-data-base.h>
#include <regen/utility/dirty-list.h>

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
		ClientBuffer();

		virtual ~ClientBuffer();

		ClientBuffer(const ClientBuffer &) = delete;

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
		 * @return the size of the data in bytes (for a single slot).
		 */
		uint32_t dataSize() const { return dataSize_; }

		/**
		 * Obtains the client data without locking.
		 * Be sure that no other thread is writing to the data at the same time.
		 * @return the client data.
		 */
		byte *clientData() const { return dataSlots_[lastDataSlot()]; }

		/**
		 * Compare stamps to check if the input data changed.
		 */
		inline uint32_t stamp() const { return dataStamps_[lastDataSlot()]; }

		/**
		 * Increment the stamp.
		 */
		void nextStamp() const;

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
		 * Resize the client buffer.
		 * @param bufferSize the new size of the buffer in bytes.
		 * @param initialData optional initial data to fill the buffer with.
		 */
		void resize(size_t bufferSize, const byte *initialData = nullptr);

		/**
		 * Flush the client buffer.
		 * This will first ensure that the current write slot has all the most recent data,
		 * and secondly, it swaps the read and write slots.
		 */
		void flush();

		void writeLockAll() const;

		void writeUnlockAll(uint32_t writeOffset, uint32_t writeSize) const;

		/**
		 * Deallocates data pointer owned by this instance.
		 * This is e.g. used if vertex data is static and only initially uploaded to the GPU.
		 */
		// TODO: reconsider
		void deallocateClientData();

		// TODO: remove
		bool requiresReUpload() const { return requiresReUpload_; }

		// TODO: remove
		void setRequiresReUpload(bool v) const { requiresReUpload_ = v; }

		// TODO: remove
		void setHasServerData(bool v) { hasServerData_ = v; }

	protected:
		uint32_t dataSize_ = 0u;
		uint32_t allocatedSize_ = 0u;
		uint32_t dataOffset_ = 0u;
		bool isFrameLocked_ = false;

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
		// indicator to writes to the data slots
		mutable uint32_t dataStamps_[2] = {0u,0u};
		// stores the ranges written to in the current and last frame if frame-locked
		DirtyList dirtyLists_[2] = {};

		// TODO remove these
		mutable bool requiresReUpload_ = false;
		bool hasServerData_ = false;

		ClientBuffer* parentBuffer_ = nullptr;
		std::vector<ref_ptr<ClientBuffer>> bufferSegments_;

		int readLock();

		bool readLock_SingleBuffer();

		void readUnlock(int slotIndex);

		int writeLock_DoubleBuffer();

		bool writeLock_SingleBuffer();

		void writeUnlock(int32_t slotIndex, uint32_t writeOffset, uint32_t writeSize) const;

		void markWrittenTo(uint32_t slotIdx, uint32_t offset, uint32_t size) const;

		MappedClientData mapRange_SingleBuffer(uint32_t offset, uint32_t size) const;

		MappedClientData mapRange_DoubleBuffer(uint32_t offset, uint32_t size) const;

		MappedClientData mapRange_ReadOnly(uint32_t offset, uint32_t size) const;

		int lastDataSlot() const;

		void createSecondSlot();

		void setDataPointer(ClientBuffer *owner, byte *dataPtr, uint32_t slotIdx) const;

		void ownerResize();

		void resize_(
				ClientBuffer *owner,
				const byte *oldDataPtr,
				byte *newDataPtr);

		void resize_(
				ClientBuffer *owner,
				const byte *oldDataPtr0,
				const byte *oldDataPtr1,
				byte *newDataPtr0,
				byte *newDataPtr1);
	};
} // namespace

#endif /* REGEN_CLIENT_BUFFER_H_ */
