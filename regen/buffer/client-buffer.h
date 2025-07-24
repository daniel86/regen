#ifndef REGEN_CLIENT_BUFFER_H_
#define REGEN_CLIENT_BUFFER_H_

#include <atomic>
#include <array>
#include <regen/regen.h>
#include <regen/buffer/mapped-client-data.h>

namespace regen {
	class ClientBuffer {
	public:
		ClientBuffer();

		~ClientBuffer() = default;

		/**
		 * Returns true if this attribute is allocated in RAM.
		 */
		bool hasClientData() const { return dataSlots_[0] != nullptr; }

		bool hasTwoSlots() const { return dataSlots_[1] != nullptr; }

		/**
		 * Obtains the client data without locking.
		 * Be sure that no other thread is writing to the data at the same time.
		 * @return the client data.
		 */
		byte *clientData() const { return dataSlots_[lastDataSlot()]; }

		/**
		 * Compare stamps to check if the input data changed.
		 */
		uint32_t stamp() const;

		/**
		 * Increment the stamp.
		 */
		void nextStamp();

		MappedData mapClientData(int mapMode) const;

		void unmapClientData(int mapMode, int slotIndex) const;

		void resizeClientBuffer(
				size_t bufferSize,
				size_t itemSize,
				const byte *initialData = nullptr);

		bool writeClientData(const byte *newData);

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
		uint32_t inputSize_ = 0u;
		uint32_t itemSize_ = 0u;

		// Note: marked as mutable because client data mapping must be allowed in const functions
		//       for reading data, but mapping interacts with locks. Hence, locks must be mutable.
		mutable std::array<byte *, 2> dataSlots_ = {nullptr, nullptr};
		// active slot for readers
		mutable std::atomic<int> lastDataSlot_{0};
		// per-slot reader/writer count
		mutable std::atomic<uint32_t> readerCounts_[2] = {0u, 0u};
		// protects against simultaneous writers
		mutable std::atomic_flag writerFlags_[2] = {ATOMIC_FLAG_INIT, ATOMIC_FLAG_INIT};
		mutable std::atomic<unsigned int> dataStamp_ = 0;

		// TODO remove these
		mutable bool requiresReUpload_ = false;
		bool hasServerData_ = false;

		//ClientBuffer* parentBuffer_ = nullptr;
		//ClientBuffer& dataOwner();

		int readLock() const;

		bool readLock_SingleBuffer() const;

		void readUnlock(int slotIndex) const;

		int writeLock() const;

		bool writeLock_SingleBuffer() const;

		void writeUnlock(int slotIndex, bool hasDataChanged) const;

		int lastDataSlot() const;

		void allocateSecondSlot() const;
	};
} // namespace

#endif /* REGEN_CLIENT_BUFFER_H_ */
