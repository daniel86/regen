#include "client-buffer.h"
#include "regen/utility/threading.h"
#include "regen/utility/logging.h"
#include "buffer-enums.h"
#include <cstring>

using namespace regen;

ClientBuffer::ClientBuffer() {
	// initially any data that will be allocated will be owned by this instance.
	// this is until it is added as a segment to another ClientBuffer.
	dataOwner_ = this;
}

ClientBuffer::~ClientBuffer() {
	if (isDataOwner()) {
		deallocateClientData();
	}
	for (auto &segment : bufferSegments_) {
		segment->parentBuffer_ = nullptr;
	}
}

/**
void ClientBuffer::addSegment(const ref_ptr<ClientBuffer> &segment) {
	if (segment->parentBuffer_ != nullptr) {
		REGEN_WARN("Segment already has a parent buffer, cannot add it again.");
		return;
	}
	// add the segment to the list of segments.
	bufferSegments_.push_back(segment);
	// set the parent buffer for the segment.
	segment->parentBuffer_ = this;
	// TODO: do a delayed resize here??
}

void ClientBuffer::removeSegment(const ref_ptr<ClientBuffer> &segment) {
	auto it = std::find(bufferSegments_.begin(), bufferSegments_.end(), segment);
	if (it != bufferSegments_.end()) {
		// remove the segment from the list of segments.
		bufferSegments_.erase(it);
		// clear the parent buffer for the segment.
		segment->parentBuffer_ = nullptr;
		// TODO: do a delayed resize here??
	} else {
		REGEN_WARN("Segment not found in the list of segments.");
	}
}
**/

void ClientBuffer::flush() {
	// flushing is only needed if the buffer is frame-locked.
	if (!isFrameLocked_) return;

	int32_t lastReadSlot = lastDataSlot_.load(std::memory_order_relaxed);
	int32_t lastWriteSlot = (dataSlots_[1] ? 1 - lastReadSlot : lastReadSlot);
	auto &dirtyLastFrame = dirtyLists_[lastReadSlot];
	auto &dirtyThisFrame = dirtyLists_[lastWriteSlot];

	// Merge overlapping segments, and sort along offsets.
	dirtyThisFrame.coalesce();
	// Delete all dirty ranges from the last read slot that have been written to this frame.
	// It is certain that both dirty lists are coalesced, so calling subtract is safe.
	dirtyLastFrame.subtract(dirtyThisFrame);

	// Remaining are the ranges where data in the write slot is not up-to-date with the read slot,
	// hence we copy it over.
	for (uint32_t rangeIdx=0; rangeIdx < dirtyLastFrame.count(); ++rangeIdx) {
		const auto &range = dirtyLastFrame.ranges()[rangeIdx];
		// Copy the data from the read slot to the write slot.
		std::memcpy(
			dataSlots_[lastWriteSlot] + range.offset,
			dataSlots_[lastReadSlot] + range.offset,
			range.size);
	}

	// For each write segment with stamp < read segment stamp: set the stamp to read segment stamp,
	// as we have synced the data above.
	for (auto &segment : bufferSegments_) {
		if (segment->dataStamps_[lastWriteSlot] < dataStamps_[lastReadSlot]) {
			segment->dataStamps_[lastWriteSlot] = dataStamps_[lastReadSlot];
		}
	}

	// clear dirty lists for the last read slot, such that it can be reused
	// next frame for writing.
	dirtyLists_[lastReadSlot].clear();
	// Finally swap read and write idx, new read idx should have new data for reading next frame.
	lastDataSlot_.store(lastWriteSlot, std::memory_order_relaxed);
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

MappedClientData ClientBuffer::mapRange(int mapMode, uint32_t offset, uint32_t size) const {
	if ((mapMode & BUFFER_GPU_WRITE) != 0) {
		if (!hasTwoSlots()) {
			// ClientBuffer in single-buffered mode.
			return mapRange_SingleBuffer(offset, size);
		} else {
			// ClientBuffer in double-buffered mode.
			return mapRange_DoubleBuffer(mapMode, offset, size);
		}
	} else {
		return mapRange_ReadOnly(offset, size);
	}
}

MappedClientData ClientBuffer::mapRange_SingleBuffer(uint32_t offset, uint32_t /*size*/) const {
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
		return { dataSlots_[0]+offset, -1, dataSlots_[0]+offset, 0 };
	} else {
		// write lock failed, which means there is another operation in progress.
		// in this case we allocate the second slot, and copy the data from the first slot to it,
		// i.e. we switch to double-buffered mode.
		writeLockAll();
		if (dataSlots_[1] == nullptr) {
			// second slot must be allocated by top-level data owner.
			dataOwner_->createSecondSlot();
			writeUnlock(0, 0, 0);
			return { dataSlots_[1]+offset, -1, dataSlots_[1]+offset, 1 };
		} else {
			// someone else has already allocated the second slot
			writeUnlockAll(0, 0);
			int w_index = dataOwner_->writeLock_DoubleBuffer();
			return { dataSlots_[w_index]+offset, -1, dataSlots_[w_index]+offset, w_index };
		}
	}
}

MappedClientData ClientBuffer::mapRange_DoubleBuffer(int mapMode, uint32_t offset, uint32_t size) const {
	// we are in double-buffered mode, i.e. we have two slots.
	// partial write can be expensive here!
	// NOTE: no index mapping needed if there is only one vertex/array element
	int w_index = dataOwner_->writeLock_DoubleBuffer();
	byte *data_w = dataSlots_[w_index];

	if (dataSize_ == size) { // FULL write
		// Note: if we are frame-locked, we skip the copy of read data,
		//       as this is done only once per frame for the whole client buffer.
		data_w += offset;
		if ((mapMode & BUFFER_GPU_READ) != 0) {
			// TODO: I do not think read lock is needed when having write lock,
			//       because as long as there is a write lock on one slot it is certain the other slot can be read safely.
			int r_index = dataOwner_->readLock();
			return {
				dataSlots_[r_index] + offset,
				r_index, data_w, w_index };
		} else {
			return { data_w, -1, data_w, w_index };
		}
	} else {
		// we swap after each write operation, and a partial write is required.
		// make sure to copy the data from the read slot to the write slot before we do the swap.
		if (!isFrameLocked_) {
			// copy the data from the read slot to the write slot.
			int r_index = dataOwner_->readLock();
			std::memcpy(data_w, dataSlots_[r_index], dataSize_);
			dataOwner_->readUnlock(r_index);
		}
		data_w += offset;
		return { data_w, -1, data_w, w_index };
	}
}

MappedClientData ClientBuffer::mapRange_ReadOnly(uint32_t offset, uint32_t /*size*/) const {
	// read only. the case of reading at index is not handled differently here.
	if (!hasTwoSlots()) {
		// we are still in single-buffered mode.
		// first we try to get a read lock on the single slot.
		if (dataOwner_->readLock_SingleBuffer()) {
			// got the read lock, return the data.
			return { dataSlots_[0] + offset, 0 };
		} else {
			// read lock failed, which means there is a write operation in progress.
			// in this case we allocate the second slot, and copy the data from the first slot to it,
			// i.e. we switch to double-buffered mode.
			writeLockAll();
			if (dataSlots_[1] == nullptr) {
				dataOwner_->createSecondSlot();
			}
			writeUnlockAll(0, 0);
		}
	}
	// read lock in double-buffered mode.
	int r_index = dataOwner_->readLock();
	return { dataSlots_[r_index] + offset, r_index };
}

void ClientBuffer::unmapRange(int32_t mapMode, uint32_t writeOffset, uint32_t writeSize, int32_t slotIndex) const {
	if ((mapMode & BUFFER_GPU_WRITE) != 0) {
		writeUnlock(slotIndex, writeOffset, writeSize);
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
	dataOwner_ = this;
	allocatedSize_ = 0u;
}

void ClientBuffer::resize(size_t dataSize, const byte *initialData) {
	// NOTE: resize should only be called with both slots being write-locked!
	int32_t resizeAmount = static_cast<int32_t>(dataSize) - static_cast<int32_t>(dataSize_);

	// adjust the data size
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
	// TODO: Better avoid reallocation, and mae it faster if possible
	// 		- using larger buffers
	//      - using a pool allocator
	//      - maybe fast re-allocation is possible?
	dataSlots_[0] = new byte[dataSize_];
	if (dataSlots_[1]) {
		dataSlots_[1] = new byte[dataSize_];
	}
	allocatedSize_ = dataSize_;
	dataOwner_ = this;

	uint32_t segmentOffset = 0;
	if (dataSlots_[1]) {
		for (auto &segment : bufferSegments_) {
			segment->resize_(
				this,
				oldData0 + segmentOffset,
				oldData1 + segmentOffset,
				dataSlots_[0] + segmentOffset,
				dataSlots_[1] + segmentOffset);
			segment->dataOffset_ = segmentOffset;
			segment->dataOwner_ = this;
			segmentOffset += segment->dataSize_;
		}
	} else {
		for (auto &segment : bufferSegments_) {
			segment->resize_(
				this,
				oldData0 + segmentOffset,
				dataSlots_[0] + segmentOffset);
			segment->dataOffset_ = segmentOffset;
			segment->dataOwner_ = this;
			segmentOffset += segment->dataSize_;
		}
	}

	// delete the old data slots.
	delete[] oldData0;
	delete[] oldData1;
}

void ClientBuffer::resize_(ClientBuffer *owner, const byte *oldDataPtr, byte *newDataPtr) {
	if (dataSize_ == allocatedSize_) {
		// no resize, just copy over the data from old to new slot.
		std::memcpy(newDataPtr, oldDataPtr, dataSize_);
		setDataPointer(owner, newDataPtr, 0);
	} else {
		if (bufferSegments_.empty()) {
			dataSlots_[0] = newDataPtr;
		} else {
			uint32_t offset = 0;
			for (auto &segment : bufferSegments_) {
				// resize each segment, copying over the data from old to new slot if size did not change.
				segment->resize_(
					owner,
					oldDataPtr + segment->dataOffset_,
					newDataPtr + offset);
				segment->dataOffset_ = offset;
				segment->dataOwner_ = owner;
				offset += segment->dataSize_;
			}
		}
		allocatedSize_ = dataSize_;
		dataOwner_ = owner;
	}
}

void ClientBuffer::resize_(
		ClientBuffer *owner,
		const byte *oldDataPtr0,
		const byte *oldDataPtr1,
		byte *newDataPtr0,
		byte *newDataPtr1) {
	if (dataSize_ == allocatedSize_) {
		// no resize, just copy over the data from old to new slot.
		std::memcpy(newDataPtr0, oldDataPtr0, dataSize_);
		std::memcpy(newDataPtr1, oldDataPtr1, dataSize_);
		setDataPointer(owner,newDataPtr0, 0);
		setDataPointer(owner,newDataPtr1, 1);
	} else {
		dataOwner_ = owner;
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
					owner,
					oldDataPtr0 + segment->dataOffset_,
					oldDataPtr1 + segment->dataOffset_,
					newDataPtr0 + offset,
					newDataPtr1 + offset);
				segment->dataOffset_ = offset;
				segment->dataOwner_ = owner;
				offset += segment->dataSize_;
			}
		}
	}
}

void ClientBuffer::setDataPointer(ClientBuffer *owner, byte *dataPtr, uint32_t slotIdx) const {
	// blindly assign a new data pointer to the slot at slotIdx,
	// assuming this ClientBuffer and its segments are not the data owner.
	dataSlots_[slotIdx] = dataPtr;
	dataOwner_ = owner;
	// Also set pointer on any sub-segments.
	// The sub-segment offsets are relative to the parent buffer range.
	for (auto &segment : bufferSegments_) {
		segment->setDataPointer(owner, dataPtr + segment->dataOffset_, slotIdx);
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

void ClientBuffer::writeUnlockAll(uint32_t writeOffset, uint32_t writeSize) const {
	writeUnlock(1, writeOffset, 0);
	writeUnlock(0, writeOffset, writeSize);
}

void ClientBuffer::writeUnlock(int32_t dataSlot, uint32_t writeOffset, uint32_t writeSize) const {
	if (writeSize > 0u) {
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
			markWrittenTo(dataSlot, writeOffset, writeSize);
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

void ClientBuffer::markWrittenTo(uint32_t slotIdx, uint32_t offset, uint32_t size) const {
	auto *parent = parentBuffer_;
	// compute global offset
	while (parent) {
		offset += parent->dataOffset_;
		parent = parent->parentBuffer_;
	}
	dataOwner_->dirtyLists_[slotIdx].insert(offset, size);
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

int ClientBuffer::writeLock_DoubleBuffer() {
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
		segment->setDataPointer(this, data_w + segment->dataOffset_, 1);
	}
}
