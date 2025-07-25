#ifndef REGEN_CLIENT_BUFFER_H_
#define REGEN_CLIENT_BUFFER_H_

#include <atomic>
#include <array>
#include <vector>
#include <regen/regen.h>
#include <regen/utility/ref-ptr.h>
#include <regen/buffer/mapped-client-data.h>

namespace regen {
	class ClientBuffer {
	public:
		ClientBuffer();

		virtual ~ClientBuffer();

		/**
		 * Returns true if this attribute is allocated in RAM.
		 */
		inline bool hasClientData() const { return dataSlots_[0] != nullptr; }

		bool hasTwoSlots() const { return dataSlots_[1] != nullptr; }

		inline bool isDataOwner() const { return dataOwner_ == this; }

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

		MappedData map(int mapMode) const;

		void unmap(int mapMode, int slotIndex) const;

		void resize(
				size_t bufferSize,
				size_t itemSize,
				const byte *initialData = nullptr);

		void writeLockAll() const;

		void writeUnlockAll(bool hasDataChanged) const;

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
		uint32_t itemSize_ = 0u;
		bool isFrameLocked_ = false;

		// Note: marked as mutable because client data mapping must be allowed in const functions
		//       for reading data, but mapping interacts with locks. Hence, locks must be mutable.
		mutable std::array<byte *, 2> dataSlots_ = {nullptr, nullptr};

		// active slot for readers
		mutable std::atomic<int> lastDataSlot_{0};
		// per-slot reader/writer count
		std::atomic<uint32_t> readerCounts_[2] = {0u, 0u};
		// protects against simultaneous writers
		std::atomic_flag writerFlags_[2] = {ATOMIC_FLAG_INIT, ATOMIC_FLAG_INIT};
		// indicator to writes to the data slots
		mutable uint32_t dataStamps_[2] = {0u,0u};

		// TODO remove these
		mutable bool requiresReUpload_ = false;
		bool hasServerData_ = false;

		mutable ClientBuffer* dataOwner_;
		ClientBuffer* parentBuffer_ = nullptr;
		std::vector<ref_ptr<ClientBuffer>> bufferSegments_;

		int readLock();

		bool readLock_SingleBuffer();

		void readUnlock(int slotIndex);

		int writeLock();

		bool writeLock_SingleBuffer();

		void writeUnlock(int slotIndex, bool hasDataChanged) const;

		void markWrittenTo(uint32_t offset, uint32_t size) const;

		MappedData mapClientData_SingleBuffer() const;

		MappedData mapClientData_DoubleBuffer(int mapMode) const;

		MappedData mapClientData_ReadOnly() const;

		int lastDataSlot() const;

		void createSecondSlot();

		void setDataPointer(byte *dataPtr, uint32_t slotIdx) const;

		void ownerResize();

		void resize_(
				const byte *oldDataPtr,
				byte *newDataPtr);

		void resize_(
				const byte *oldDataPtr0,
				const byte *oldDataPtr1,
				byte *newDataPtr0,
				byte *newDataPtr1);
	};
} // namespace

#endif /* REGEN_CLIENT_BUFFER_H_ */
