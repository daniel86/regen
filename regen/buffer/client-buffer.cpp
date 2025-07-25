#include "client-buffer.h"
#include "regen/utility/threading.h"
#include "regen/utility/logging.h"
#include <cstring>
// TODO move enum somewhere else
#include "regen/gl-types/shader-data.h"

// TODO: could do locking of ranges such that writing into distinct ranges
//    is possible concurrently.

using namespace regen;

ClientBuffer::ClientBuffer() {
	// initially any data that will be allocated will be owned by this instance.
	// this is until it is added as a segment to another ClientBuffer.
	dataOwner_ = this;
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

ClientBuffer::~ClientBuffer() {
	if (isDataOwner()) {
		deallocateClientData();
	}
	for (auto &segment : bufferSegments_) {
		segment->parentBuffer_ = nullptr;
	}
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

void ClientBuffer::nextStamp() const {
	dataStamps_[0] += 1;
	dataStamps_[1] += 1;
	auto *parent = parentBuffer_;
	while (parent != nullptr) {
		parent->dataStamps_[0] += 1;
		parent->dataStamps_[1] += 1;
		parent = parent->parentBuffer_;
	}
}

MappedData ClientBuffer::map(int mapMode) const {
	if ((mapMode & ShaderData::WRITE) != 0) {
		if (!hasTwoSlots()) {
			// ClientBuffer in single-buffered mode.
			return mapClientData_SingleBuffer();
		} else {
			// ClientBuffer in double-buffered mode.
			return mapClientData_DoubleBuffer(mapMode);
		}
	} else {
		return mapClientData_ReadOnly();
	}
}

MappedData ClientBuffer::mapClientData_SingleBuffer() const {
	// ClientBuffer initially has only one slot, the second is allocated on demand in case
	// multiple threads are concurrently reading/writing the data.
	// here we keep writing to the active slot as long as no one has to wait,
	// but as soon as there is waiting time we allocate the second slot and copy the data
	// to avoid waiting in the future.
	// partial writing is ok here, as we update the most recent data slot.
	// NOTE: r_index -1 indicates that there is no read lock, i.e. no need to call readUnlock in unmap.
	if (dataOwner_->writeLock_SingleBuffer()) {
		// got the write lock, return the data.
		// this means there are currently no readers, nor writers, so we can safely write to the active slot.
		return { dataSlots_[0], -1, dataSlots_[0], 0 };
	} else {
		// write lock failed, which means there is another operation in progress.
		// in this case we allocate the second slot, and copy the data from the first slot to it,
		// i.e. we switch to double-buffered mode.
		writeLockAll();
		if (dataSlots_[1] == nullptr) {
			// second slot must be allocated by top-level data owner.
			dataOwner_->createSecondSlot();
			writeUnlock(0, false);
			return { dataSlots_[1], -1, dataSlots_[1], 1 };
		} else {
			// someone else has already allocated the second slot
			writeUnlockAll(false);
			int w_index = dataOwner_->writeLock();
			return { dataSlots_[w_index], -1, dataSlots_[w_index], w_index };
		}
	}
}

MappedData ClientBuffer::mapClientData_DoubleBuffer(int mapMode) const {
	// we are in double-buffered mode, i.e. we have two slots.
	// partial write can be expensive here!
	// NOTE: no index mapping needed if there is only one vertex/array element
	bool isFullWrite = ((mapMode & ShaderData::INDEX) == 0 || (dataSize_ <= itemSize_));
	int w_index = dataOwner_->writeLock();
	byte *data_w = dataSlots_[w_index];

	if (isFullWrite) {
		// Note: if we are frame-locked, we skip the copy of read data,
		//       as this is done only once per frame for the whole client buffer.
		if ((mapMode & ShaderData::READ) != 0) {
			// TODO: I do not think read lock is needed when having write lock,
			//       because as long as there is a write lock on one slot it is certain the other slot can be read safely.
			int r_index = dataOwner_->readLock();
			return { dataSlots_[r_index], r_index, data_w, w_index };
		} else {
			return { data_w, -1, data_w, w_index };
		}
	} else {
		// we swap after each write operation, and a partial write is required.
		// make sure to copy the data from the read slot to the write slot before we do the swap.
		// FIXME: In frame-locked mode this will overwrite data!!
		//    --> use stamps to ensure that we do not overwrite data? will need to use > then
		int r_index = dataOwner_->readLock();
		std::memcpy(data_w, dataSlots_[r_index], dataSize_);
		dataOwner_->readUnlock(r_index);
		return { data_w, -1, data_w, w_index };
	}
}

MappedData ClientBuffer::mapClientData_ReadOnly() const {
	// read only. the case of reading at index is not handled differently here.
	if (!hasTwoSlots()) {
		// we are still in single-buffered mode.
		// first we try to get a read lock on the single slot.
		if (dataOwner_->readLock_SingleBuffer()) {
			// got the read lock, return the data.
			return { dataSlots_[0], 0 };
		} else {
			// read lock failed, which means there is a write operation in progress.
			// in this case we allocate the second slot, and copy the data from the first slot to it,
			// i.e. we switch to double-buffered mode.
			writeLockAll();
			if (dataSlots_[1] == nullptr) {
				dataOwner_->createSecondSlot();
				writeUnlock(0, false);
				writeUnlock(1, true);
			} else {
				writeUnlockAll(false);
			}
		}
	}
	// read lock in double-buffered mode.
	int r_index = dataOwner_->readLock();
	return { dataSlots_[r_index], r_index };
}

void ClientBuffer::unmap(int mapMode, int slotIndex) const {
	if ((mapMode & ShaderData::WRITE) != 0) {
		writeUnlock(slotIndex, true);
	} else {
		dataOwner_->readUnlock(slotIndex);
	}
}

void ClientBuffer::deallocateClientData() {
	if (isDataOwner()) {
		for (int i = 0; i < 2; ++i) {
			if (dataSlots_[i]) {
				delete[] dataSlots_[i];
				dataSlots_[i] = nullptr;
			}
		}
	} else {
		for (int i = 0; i < 2; ++i) {
			if (dataSlots_[i]) {
				dataSlots_[i] = nullptr;
			}
		}
	}
	for (auto &segment : bufferSegments_) {
		segment->deallocateClientData();
	}
	dataOffset_ = 0u;
	dataSize_ = 0u;
	itemSize_ = 0u;
	allocatedSize_ = 0u;
}

void ClientBuffer::resize(size_t dataSize, size_t itemSize, const byte *initialData) {
	// NOTE: resize should only be called with both slots being write-locked!
	int32_t resizeAmount = static_cast<int32_t>(dataSize) - static_cast<int32_t>(dataSize_);

	// adjust the data size
	itemSize_ = static_cast<uint32_t>(itemSize);
	dataSize_ = static_cast<uint32_t>(dataSize);
	auto *parent = parentBuffer_;
	while (parent) {
		parent->dataSize_ = parent->dataSize_ + resizeAmount;
	}

	// do the re-allocation of data slots.
	dataOwner_->ownerResize();
	nextStamp();

	// copy over initial data if any
	if (initialData) {
		std::memcpy(dataSlots_[0], initialData, dataSize);
		if (dataSlots_[1]) {
			std::memcpy(dataSlots_[1], initialData, dataSize);
		}
	}
}

void ClientBuffer::ownerResize() {
	// keep a reference to the old data slots, for copying data over.
	byte *oldData0 = dataSlots_[0];
	byte *oldData1 = dataSlots_[1];

	// allocate new data slots.
	if (dataSlots_[0]) {
		// TODO: Better avoid reallocation, and mae it faster if possible
		// 		- using larger buffers
		//      - using a pool allocator
		//      - maybe fast re-allocation is possible?
		REGEN_WARN("Re-allocating ClientBuffer data slots from "
			<< allocatedSize_/1024.0f << " to " << dataSize_/1024.0f << " KiB.");
	}
	dataSlots_[0] = new byte[dataSize_];
	if (dataSlots_[1]) {
		dataSlots_[1] = new byte[dataSize_];
	}
	allocatedSize_ = dataSize_;

	uint32_t segmentOffset = 0;
	if (dataSlots_[1]) {
		for (auto &segment : bufferSegments_) {
			segment->resize_(
				oldData0 + segmentOffset,
				oldData1 + segmentOffset,
				dataSlots_[0] + segmentOffset,
				dataSlots_[1] + segmentOffset);
			segment->dataOffset_ = segmentOffset;
			segmentOffset += segment->dataSize_;
		}
	} else {
		for (auto &segment : bufferSegments_) {
			segment->resize_(
				oldData0 + segmentOffset,
				dataSlots_[0] + segmentOffset);
			segment->dataOffset_ = segmentOffset;
			segmentOffset += segment->dataSize_;
		}
	}

	// delete the old data slots.
	delete[] oldData0;
	delete[] oldData1;
}

void ClientBuffer::resize_(const byte *oldDataPtr, byte *newDataPtr) {
	if (dataSize_ == allocatedSize_) {
		// no resize, just copy over the data from old to new slot.
		std::memcpy(newDataPtr, oldDataPtr, dataSize_);
		setDataPointer(newDataPtr, 0);
	} else {
		if (bufferSegments_.empty()) {
			dataSlots_[0] = newDataPtr;
		} else {
			uint32_t offset = 0;
			for (auto &segment : bufferSegments_) {
				// resize each segment, copying over the data from old to new slot if size did not change.
				segment->resize_(
					oldDataPtr + segment->dataOffset_,
					newDataPtr + offset);
				segment->dataOffset_ = offset;
				offset += segment->dataSize_;
			}
		}
		allocatedSize_ = dataSize_;
	}
}

void ClientBuffer::resize_(
		const byte *oldDataPtr0,
		const byte *oldDataPtr1,
		byte *newDataPtr0,
		byte *newDataPtr1) {
	if (dataSize_ == allocatedSize_) {
		// no resize, just copy over the data from old to new slot.
		std::memcpy(newDataPtr0, oldDataPtr0, dataSize_);
		std::memcpy(newDataPtr1, oldDataPtr1, dataSize_);
		setDataPointer(newDataPtr0, 0);
		setDataPointer(newDataPtr1, 1);
	} else {
		if (bufferSegments_.empty()) {
			dataSlots_[0] = newDataPtr0;
			dataSlots_[1] = newDataPtr1;
			allocatedSize_ = dataSize_;
		} else {
			uint32_t offset = 0;
			for (auto &segment : bufferSegments_) {
				// resize each segment, copying over the data from old to new slot if size did not change.
				// TODO: need to mark dirty each segment that moved in the buffer?
				segment->resize_(
					oldDataPtr0 + segment->dataOffset_,
					oldDataPtr1 + segment->dataOffset_,
					newDataPtr0 + offset,
					newDataPtr1 + offset);
				segment->dataOffset_ = offset;
				offset += segment->dataSize_;
			}
		}
	}
}

void ClientBuffer::setDataPointer(byte *dataPtr, uint32_t slotIdx) const {
	// blindly assign a new data pointer to the slot at slotIdx,
	// assuming this ClientBuffer and its segments are not the data owner.
	dataSlots_[slotIdx] = dataPtr;
	// Also set pointer on any sub-segments.
	// The sub-segment offsets are relative to the parent buffer range.
	for (auto &segment : bufferSegments_) {
		segment->setDataPointer(dataPtr + segment->dataOffset_, slotIdx);
	}
}

void ClientBuffer::writeLockAll() const {
	for (auto & writerFlag : dataOwner_->writerFlags_) {
		// get exclusive write access to the data slot:
		// block any attempt to write concurrently to this slot.
		while (writerFlag.test_and_set(std::memory_order_acquire)) {
			CPU_PAUSE(); // spin-wait for writers
		}
	}
	for (auto & readerCount : dataOwner_->readerCounts_) {
		// wait for any active readers to finish.
		spinWaitUntil2(readerCount);
	}
}

void ClientBuffer::writeUnlockAll(bool hasDataChanged) const {
	writeUnlock(1, false);
	writeUnlock(0, hasDataChanged);
}

void ClientBuffer::writeUnlock(int dataSlot, bool hasDataChanged) const {
	if (hasDataChanged) {
		// increment the data stamp, and remember the last slot that was written to.
		// consecutive reads will be done from this slot, next write will be done to the other slot.
		// If the write operation did not change the data, the stamp is not incremented,
		// and the last slot is not updated.
		if (dataSlots_[1]) {
			dataStamps_[dataSlot] = dataStamps_[1-dataSlot] + 1;
			// Increase the stamp for all parent buffer ranges as well.
			auto *parent = parentBuffer_;
			while (parent) {
				parent->dataStamps_[dataSlot] = parent->dataStamps_[1-dataSlot] + 1;
				parent = parent->parentBuffer_;
			}
		} else {
			// single-buffered mode, we just increment the stamp for the single slot.
			dataStamps_[dataSlot] += 1;
			// Increase the stamp for all parent buffer ranges as well.
			auto *parent = parentBuffer_;
			while (parent) {
				parent->dataStamps_[dataSlot] += 1;
				parent = parent->parentBuffer_;
			}
		}

		if (isFrameLocked_) {
			// If frame-locked, the swap to the other slot is done centrally, not on write unlock.
			// But we still need to remember which data range was written to this frame.
			// This is done to avoid unnecessary copies.
			markWrittenTo(0, dataSize_);
		} else {
			// swap to the other slot.
			lastDataSlot_.store(dataSlot, std::memory_order_release);
		}

		// TODO: remove after adding VBO stuff into staging!
		if (hasServerData_) {
			requiresReUpload_ = true;
		}
	}
	// clear the exclusive write lock for this slot, allowing any waiting writer to proceed.
	// NOTE: reader will only proceed once all writing is done.
	dataOwner_->writerFlags_[dataSlot].clear(std::memory_order_relaxed);
}

void ClientBuffer::markWrittenTo(uint32_t offset, uint32_t size) const {
	auto *parent = parentBuffer_;
	// compute global offset
	while (parent) {
		offset += parent->dataOffset_;
		parent = parent->parentBuffer_;
	}
	// TODO: Mark global range as dirty.
	REGEN_WARN("todo: mark dirty range: " << offset << " size: " << size);
	//dirtyList.add({offset, size});
}

////////////////////
////////////// below functions are only called on the data owner.
////////////////////

int ClientBuffer::lastDataSlot() const {
	return lastDataSlot_.load(std::memory_order_acquire);
}

int ClientBuffer::readLock() {
	while (true) {
		// Get the current slot index for reading.
		// note that every writer will flip the slot index, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try, e.g.
		// because there are active writers on the slot which in turn will flip the slot index once done.
		int dataSlot = lastDataSlot_.load(std::memory_order_acquire);

		// First step: increment the reader count for this slot.
		// this will prevent writers from setting the flag on this slot.
		readerCounts_[dataSlot].fetch_add(1, std::memory_order_relaxed);

		// However, maybe there is an active writer on this slot already, we need to check that.
		if (writerFlags_[dataSlot].test(std::memory_order_acquire) == 0) {
			// no writer has locked the slot, other ones are prevented from doing so,
			// hence we can safely read from this slot.
			return dataSlot;
		}
		else {
			// Seems there is an active writer on this slot, we need to wait for them to finish.
			// but first decrement the reader count, so that we do not block writer in the meanwhile.
			readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
			// wait until there are no active writers on `dataSlot`.
			spinWaitUntil1(writerFlags_[dataSlot]);
		}
	}
}

bool ClientBuffer::readLock_SingleBuffer() {
	// We are here in single buffer mode, and only quickly try to get a lock in the one
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

void ClientBuffer::readUnlock(int dataSlot) {
	readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
}

int ClientBuffer::writeLock() {
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

bool ClientBuffer::writeLock_SingleBuffer() {
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

void ClientBuffer::createSecondSlot() {
	auto data_w = new byte[dataSize_];
	std::memcpy(data_w, dataSlots_[0], dataSize_);
	dataSlots_[1] = data_w;
	// Initialize second slot stamp to the same value as the first slot.
	dataStamps_[1] = dataStamps_[0];

	// Assign second slot ptr's and offsets to all segments
	for (auto &segment : bufferSegments_) {
		segment->setDataPointer(data_w + segment->dataOffset_, 1);
	}
}
