#include "client-buffer.h"
#include "regen/utility/threading.h"
#include <cstring>
// TODO move enum somewhere else
#include "regen/gl-types/shader-data.h"

using namespace regen;

ClientBuffer::ClientBuffer() {
	// initialize the data slots to nullptr
	dataSlots_[0] = nullptr;
	dataSlots_[1] = nullptr;
	// initialize the reader counts to 0
	readerCounts_[0].store(0, std::memory_order_relaxed);
	readerCounts_[1].store(0, std::memory_order_relaxed);
	// initialize the writer flags to not set
	writerFlags_[0].clear(std::memory_order_relaxed);
	writerFlags_[1].clear(std::memory_order_relaxed);
}

inline void spinWaitUntil1(std::atomic_flag &flag) {
    for (int i = 0; flag.test(std::memory_order_acquire) != 0; ++i) {
        if (i < 20) CPU_PAUSE();
        else std::this_thread::yield();
    }
}

inline void spinWaitUntil2(std::atomic<uint32_t> &count) {
    for (int i = 0; count.load(std::memory_order_acquire) != 0; ++i) {
        if (i < 20) CPU_PAUSE();
        else std::this_thread::yield();
    }
}

//ClientBuffer& ClientBuffer::dataOwner() {
//	return *dataOwner_;
//}

unsigned int ClientBuffer::stamp() const {
	return dataStamp_.load(std::memory_order_relaxed);
}

void ClientBuffer::nextStamp() {
	dataStamp_.fetch_add(1, std::memory_order_relaxed);
}

int ClientBuffer::lastDataSlot() const {
	return lastDataSlot_.load(std::memory_order_acquire);
}

int ClientBuffer::readLock() const {
	while (true) {
		// get the current slot index for reading.
		// note that every writer will flip the slot index, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try, e.g.
		// because there are active writers on the slot which in turn will flip the slot index once done.
		int dataSlot = lastDataSlot_.load(std::memory_order_acquire);

		// first step: increment the reader count for this slot.
		// this will prevent writers from setting the flag on this slot.
		readerCounts_[dataSlot].fetch_add(1, std::memory_order_relaxed);

		// however, maybe there is an active writer on this slot already, we need to check that.
		if (writerFlags_[dataSlot].test(std::memory_order_acquire) == 0) {
			// no writer has locked the slot, other ones are prevented from doing so,
			// hence we can safely read from this slot.
			return dataSlot;
		}
		else {
			// seems there is an active writer on this slot, we need to wait for them to finish.
			// but first decrement the reader count, so that we do not block writer in the meanwhile.
			readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
			// wait until there are no active writers on `dataSlot`.
			spinWaitUntil1(writerFlags_[dataSlot]);
		}
	}
}

bool ClientBuffer::readLock_SingleBuffer() const {
	// we are here in single buffer mode, and only quickly try to get a lock in the one
	// slot (with index 0), or else return false.
	// and the only thing preventing us from doing so would be a writer that is currently writing to the slot
	// which would be indicated by the writerFlags_[0] being set.
	readerCounts_[0].fetch_add(1, std::memory_order_relaxed);
	if (writerFlags_[0].test(std::memory_order_acquire) != 0) {
		readerCounts_[0].fetch_sub(1, std::memory_order_relaxed);
		return false; // Busy writing
	} else {
		return true;
	}
}

void ClientBuffer::readUnlock(int dataSlot) const {
	readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
}

int ClientBuffer::writeLock() const {
	while (true) {
		// get the current slot index for writing.
		// note that every writer will flip the slot index, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try.
		int dataSlot = 1 - lastDataSlot_.load(std::memory_order_acquire);

		// check if there are any active readers on the write slot.
		if (readerCounts_[dataSlot].load(std::memory_order_acquire) != 0) {
			// seems there are some remaining readers on the write slot, we need to wait for them to finish.
			spinWaitUntil2(readerCounts_[dataSlot]);
			continue; // try again
		}

		if (writerFlags_[dataSlot].test_and_set(std::memory_order_acquire)) {
			// seems someone else is writing to this slot, we need to wait for them to finish.
			spinWaitUntil1(writerFlags_[dataSlot]);
			continue; // try again
		} else {
			if (readerCounts_[dataSlot].load(std::memory_order_acquire) != 0) {
				// a reader sneaked in while we were waiting for the write lock
				writerFlags_[dataSlot].clear(std::memory_order_relaxed);
				continue;
			}
			// we got the exclusive write lock for this slot, so we can safely write to it.
			return dataSlot;
		}
	}
}

bool ClientBuffer::writeLock_SingleBuffer() const {
	// acquire exclusive write lock
	if (writerFlags_[0].test_and_set(std::memory_order_acquire)) {
		return false; // Busy writing
	}
	// check for any active readers.
	if (readerCounts_[0].load(std::memory_order_acquire) != 0) {
		writerFlags_[0].clear(std::memory_order_relaxed);
		return false; // Busy reading
	}
	return true;
}

void ClientBuffer::writeUnlock(int dataSlot, bool hasDataChanged) const {
	if (hasDataChanged) {
		// increment the data stamp, and remember the last slot that was written to.
		// consecutive reads will be done from this slot, next write will be done to the other slot.
		// If the write operation did not change the data, the stamp is not incremented,
		// and the last slot is not updated.
		dataStamp_.fetch_add(1, std::memory_order_relaxed);
		lastDataSlot_.store(dataSlot, std::memory_order_release);
		// TODO: remove after adding VBO stuff into staging!
		if (hasServerData_) {
			requiresReUpload_ = true;
		}
	}
	// clear the exclusive write lock for this slot, allowing any waiting writer to proceed.
	// NOTE: reader will only proceed once all writing is done.
	writerFlags_[dataSlot].clear(std::memory_order_relaxed);
}

void ClientBuffer::writeLockAll() const {
	for (auto & writerFlag : writerFlags_) {
		// get exclusive write access to the data slot:
		// block any attempt to write concurrently to this slot.
		while (writerFlag.test_and_set(std::memory_order_acquire)) {
			CPU_PAUSE(); // spin-wait for writers
		}
	}
	for (auto & readerCount : readerCounts_) {
		// wait for any active readers to finish.
		spinWaitUntil2(readerCount);
	}
}

void ClientBuffer::writeUnlockAll(bool hasDataChanged) const {
	writeUnlock(1, false);
	writeUnlock(0, hasDataChanged);
}

void ClientBuffer::allocateSecondSlot() const {
	auto data_w = new byte[inputSize_];
	std::memcpy(data_w, dataSlots_[0], inputSize_);
	dataSlots_[1] = data_w;
}

bool ClientBuffer::writeClientData(const byte *data) {
	if (data) {
		// NOTE: writeLockAll locks slot 0 last, so we know it will be the active slot when we unlock.
		std::memcpy(dataSlots_[0], data, inputSize_);
		return true;
	} else {
		return false;
	}
}

MappedData ClientBuffer::mapClientData(int mapMode) const {
	// ShaderInput initially has only one slot, the second is allocated on demand in case
	// multiple threads are concurrently reading/writing the data.
	// here we keep writing to the active slot as long as no one has to wait,
	// but as soon as there is waiting time we allocate the second slot and copy the data
	// to avoid waiting in the future.

	if ((mapMode & ShaderData::WRITE) != 0) {
		if (!hasTwoSlots()) {
			// partial writing is ok here, as we update the most recent data slot.
			// NOTE: r_index -1 indicates that there is no read lock, i.e. no need to call readUnlock in unmap.
			if (writeLock_SingleBuffer()) {
				// got the write lock, return the data.
				// this means there are currently no readers, nor writers, so we can safely write to the active slot.
				return { dataSlots_[0], -1, dataSlots_[0], 0 };
			} else {
				// write lock failed, which means there is another operation in progress.
				// in this case we allocate the second slot, and copy the data from the first slot to it,
				// i.e. we switch to double-buffered mode.
				writeLockAll();
				if (dataSlots_[1] == nullptr) {
					allocateSecondSlot();
					writeUnlock(0, false);
					return { dataSlots_[1], -1, dataSlots_[1], 1 };
				} else {
					// someone else has already allocated the second slot
					writeUnlockAll(false);
					int w_index = writeLock();
					return { dataSlots_[w_index], -1, dataSlots_[w_index], w_index };
				}
			}
		} else {
			// we are in double-buffered mode, i.e. we have two slots.
			// partial write can be expensive here!
			// NOTE: no index mapping needed if there is only one vertex/array element
			bool isFullWrite = (mapMode & ShaderData::INDEX) == 0 || (inputSize_ <= itemSize_);
			int w_index = writeLock();
			byte *data_w = dataSlots_[w_index];

			// TODO: I do not think read lock is needed when having write lock,
			//       because as long as there is a write lock on one slot it is certain the other slot can be read safely.
			if (isFullWrite) {
				if ((mapMode & ShaderData::READ) != 0) {
					int r_index = readLock();
					return { dataSlots_[r_index], r_index, data_w, w_index };
				} else {
					return { data_w, -1, data_w, w_index };
				}
			} else {
				// copy FULL data into write slot for partial write.
				// NOTE: this will be inefficient if the data is large!
				// TODO: in some cases a better strategy could be to write into the current slot instead of copying
				//       the data to the other slot. Maybe a sensible heuristic would be the data size:
				//       for small data, especially non-array, non-vertex data, always prefer copy.
				//       for larger array and vertex data prefer write into current slot.
				int r_index = readLock();
				std::memcpy(data_w, dataSlots_[r_index], inputSize_);
				readUnlock(r_index);
				return { data_w, -1, data_w, w_index };
			}
		}
	} else {
		// read only. the case of reading at index is not handled differently here.
		if (!hasTwoSlots()) {
			// we are still in single-buffered mode.
			// first we try to get a read lock on the single slot.
			if (readLock_SingleBuffer()) {
				// got the read lock, return the data.
				return { dataSlots_[0], 0 };
			} else {
				// read lock failed, which means there is a write operation in progress.
				// in this case we allocate the second slot, and copy the data from the first slot to it,
				// i.e. we switch to double-buffered mode.
				writeLockAll();
				if (dataSlots_[1] == nullptr) {
					allocateSecondSlot();
					writeUnlock(0, false);
					writeUnlock(1, true);
				} else {
					writeUnlockAll(false);
				}
			}
		}
		// read lock in double-buffered mode.
		int r_index = readLock();
		return { dataSlots_[r_index], r_index };
	}
}

void ClientBuffer::unmapClientData(int mapMode, int slotIndex) const {
	if ((mapMode & ShaderData::WRITE) != 0) {
		writeUnlock(slotIndex, true);
	} else {
		readUnlock(slotIndex);
	}
}

void ClientBuffer::deallocateClientData() {
	for (int i = 0; i < 2; ++i) {
		if (dataSlots_[i]) {
			delete[] dataSlots_[i];
			dataSlots_[i] = nullptr;
		}
	}
}

void ClientBuffer::resizeClientBuffer(
				size_t bufferSize,
				size_t itemSize,
				const byte *initialData) {
	{
		if (dataSlots_[0]) {
			delete[] dataSlots_[0];
		}
		dataSlots_[0] = new byte[bufferSize];
		if (initialData) {
			std::memcpy(dataSlots_[0], initialData, bufferSize);
		}
	}
	{
		if (dataSlots_[1]) {
			delete[] dataSlots_[1];
			dataSlots_[1] = new byte[bufferSize];
		}
		if (initialData) {
			std::memcpy(dataSlots_[1], initialData, bufferSize);
		}
	}
	inputSize_ = static_cast<uint32_t>(bufferSize);
	itemSize_ = static_cast<uint32_t>(itemSize);
}
