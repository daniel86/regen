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
	deallocateClientData();
	bufferSegments_.clear();
}

void ClientBuffer::setFrameLocked(bool frameLocked) {
	isFrameLocked_ = frameLocked;
	for (auto &segment : bufferSegments_) {
		segment->setFrameLocked(frameLocked);
	}
}

void ClientBuffer::setSegments(const std::vector<ref_ptr<ClientBuffer>> &segments) {
	writeLockAll();
	// clear the current segments.
	for (auto &segment : bufferSegments_) {
		segment->parentBuffer_ = nullptr;
	}
	bufferSegments_ = segments;
	for (auto &segment : bufferSegments_) {
		if (segment->parentBuffer_ != nullptr) {
			REGEN_WARN("Segment already has a parent buffer!");
		}
		segment->parentBuffer_ = this;
		segment->setFrameLocked(isFrameLocked_);
	}
	dataSize_ = 0u;
	allocatedSize_ = 0u;
	// do the re-allocation of data slots.
	dataOwner_->ownerResize();
	nextStamp();
	writeUnlockAll(0u, 0);
}

void ClientBuffer::addSegment(const ref_ptr<ClientBuffer> &segment) {
	if (segment->parentBuffer_ != nullptr) {
		REGEN_WARN("Segment already has a parent buffer, cannot add it again.");
		return;
	}
	writeLockAll();
	// add the segment to the list of segments.
	bufferSegments_.push_back(segment);
	// set the parent buffer for the segment.
	segment->parentBuffer_ = this;
	segment->setFrameLocked(isFrameLocked_);

	// do the re-allocation of data slots.
	dataOwner_->ownerResize();
	nextStamp();
	writeUnlockAll(0u, 0u);
}

void ClientBuffer::removeSegment(const ref_ptr<ClientBuffer> &segment) {
	auto it = std::find(bufferSegments_.begin(), bufferSegments_.end(), segment);
	if (it != bufferSegments_.end()) {
		writeLockAll();
		// remove the segment from the list of segments.
		bufferSegments_.erase(it);
		// clear the parent buffer for the segment.
		segment->parentBuffer_ = nullptr;
		// do the re-allocation of data slots.
		dataOwner_->ownerResize();
		nextStamp();
		writeUnlockAll(0u, 0u);
	} else {
		REGEN_WARN("Segment not found in the list of segments.");
	}
}

uint32_t ClientBuffer::swapData() {
	// NOTE: This function should be very fast as potentially both animation and rendering threads
	//       are waiting for it to finish.
	// flushing is only needed if the buffer is frame-locked.
	if (!isFrameLocked_ || dataSize_==0u) return 0u;

	int32_t lastReadSlot = lastDataSlot_.load(std::memory_order_relaxed);
	auto &dirtyLastFrame = dirtyLists_[lastReadSlot];

	if (dataSlots_[1]) {
		int32_t lastWriteSlot = 1 - lastReadSlot;
		auto &dirtyThisFrame = dirtyLists_[lastWriteSlot];

		// Merge overlapping segments, and sort along offsets.
		dirtyThisFrame.coalesce();
		// Delete all dirty ranges from the last read slot that have been written to this frame.
		// It is certain that both dirty lists are coalesced, so calling subtract is safe.
		dirtyLastFrame.subtract(dirtyThisFrame);

		// Remaining are the ranges where data in the write slot is not up-to-date with the read slot,
		// hence we copy it over.
		const uint32_t numCopiesNeeded = dirtyLastFrame.count();
		for (uint32_t rangeIdx=0; rangeIdx < numCopiesNeeded; ++rangeIdx) {
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

		// Finally swap read and write idx, new read idx should have new data for reading next frame.
		lastDataSlot_.store(lastWriteSlot, std::memory_order_relaxed);

		// clear dirty lists for the last read slot, such that it can be reused
		// next frame for writing.
		dirtyLastFrame.clear();

		return numCopiesNeeded;
	} else {
		// Single-buffered mode, no need to copy data.
		// Just clear the dirty list for the last read slot.
		dirtyLastFrame.clear();
		return 0u;
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

void ClientBuffer::nextStamp(uint32_t dataSlot) const {
	auto readSlot = (dataSlots_[1] ? (1-dataSlot) : 0);
	dataStamps_[dataSlot] = dataStamps_[readSlot] + 1;
	// Increase the stamp for all parent buffer ranges as well.
	auto *parent = parentBuffer_;
	while (parent) {
		parent->dataStamps_[dataSlot] = parent->dataStamps_[readSlot] + 1;
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
			return mapRange_DoubleBuffer(offset, size);
		}
	} else {
		return mapRange_ReadOnly(offset, size);
	}
}

MappedClientData ClientBuffer::mapRange_SingleBuffer(uint32_t offset, uint32_t size) const {
	// ClientBuffer initially has only one slot, the second is allocated on demand in case
	// multiple threads are concurrently reading/writing the data.
	// here we keep writing to the active slot as long as no one has to wait,
	// but as soon as there is waiting time we allocate the second slot and copy the data
	// to avoid waiting in the future.
	// partial writing is ok here, as we update the most recent data slot.
	// NOTE: r_index -1 indicates that there is no read lock, i.e. no need to call readUnlock in unmap.
	if (writeLock_SingleBuffer()) {
		// got the write lock, return the data.
		// this means there are currently no readers, nor writers, so we can safely write to the active slot.
		return { dataSlots_[0]+offset, -1, dataSlots_[0]+offset, 0 };
	} else {
		// write lock failed, which means there is another operation in progress.
		// in this case we allocate the second slot, and copy the data from the first slot to it,
		// i.e. we switch to double-buffered mode.
		// get a read lock on the first slot.
		int r_index = readLock();
		if (r_index > 0) {
			// seems someone else allocated the second slot already.
			// release the read lock and do double-buffered write.
			readUnlock(r_index);
			return mapRange_DoubleBuffer(offset, size);
		} else {
			// get a write lock on the second slot.
			if (dataOwner_->writerFlags_[1].test_and_set(std::memory_order_acquire) == 0) {
				if (dataSlots_[1] == nullptr) {
					dataOwner_->createSecondSlot();
					return { dataSlots_[0]+offset, 0, dataSlots_[1]+offset, 1 };
				} else {
					writeUnlock(1, 0, 0);
					readUnlock(r_index);
					return mapRange_DoubleBuffer(offset, size);
				}
			} else {
				// someone else holds the write lock on the second slot.
				readUnlock(r_index);
				if (dataSlots_[1] == nullptr) {
					// still single-buffered, probably someone does a lock-all on second slot. retry...
					CPU_PAUSE();
					REGEN_WARN("write lock on null second slot failed, retrying...");
					return mapRange(BUFFER_GPU_WRITE, offset, size);
				} else {
					// we have the second slot, so we can write to it, once we have the write lock.
					return mapRange_DoubleBuffer(offset, size);
				}
			}
		}
	}
}

MappedClientData ClientBuffer::mapRange_DoubleBuffer(uint32_t offset, uint32_t size) const {
	// we are in double-buffered mode, i.e. we have two slots.
	// partial write can be expensive here!
	// NOTE: no index mapping needed if there is only one vertex/array element
	int w_index = writeLock_DoubleBuffer();
	byte *data_w = dataSlots_[w_index];

	if (dataSize_ == size) { // FULL write
		return {
			dataSlots_[1-w_index] + offset, -1,
			data_w + offset, w_index };
	} else {
		// we swap after each write operation, and a partial write is required.
		// make sure to copy the data from the read slot to the write slot before we do the swap.
		if (!isFrameLocked_) {
			// copy the data from the read slot to the write slot.
			int r_index = readLock();
			std::memcpy(data_w, dataSlots_[r_index], dataSize_);
			readUnlock(r_index);
		}
		return {
			dataSlots_[1-w_index] + offset, -1,
			data_w + offset, w_index };
	}
}

MappedClientData ClientBuffer::mapRange_ReadOnly(uint32_t offset, uint32_t size) const {
	// read only. the case of reading at index is not handled differently here.
	if (!hasTwoSlots()) {
		// we are still in single-buffered mode.
		// first we try to get a read lock on the single slot.
		if (readLock_SingleBuffer()) {
			// got the read lock, return the data.
			return { dataSlots_[0] + offset, 0 };
		} else {
			// read lock failed, which means there is a write operation in progress.
			// in this case we allocate the second slot, and copy the data from the first slot to it,
			// i.e. we switch to double-buffered mode.
			// get a write lock on the second slot.
			if (dataOwner_->writerFlags_[1].test_and_set(std::memory_order_acquire) == 0) {
				// we got the write lock on the second slot, so we can allocate it.
				if (dataSlots_[1] == nullptr) {
					dataOwner_->createSecondSlot();
				}
				writeUnlock(1, 0, 0);
			} else {
				// someone else holds the write lock on the second slot.
				// we need to wait for it to finish.
				CPU_PAUSE();
				REGEN_WARN("read lock on null second slot failed, retrying...");
			}
			return mapRange_ReadOnly(offset, size); // retry
		}
	}
	// read lock in double-buffered mode.
	int r_index = readLock();
	return { dataSlots_[r_index] + offset, r_index };
}

void ClientBuffer::unmapRange(int32_t mapMode, uint32_t writeOffset, uint32_t writeSize, int32_t slotIndex) const {
	if ((mapMode & BUFFER_GPU_WRITE) != 0) {
		writeUnlock(slotIndex, writeOffset, writeSize);
	} else {
		readUnlock(slotIndex);
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
	lastOffset_ = 0u;
	dataOwner_ = this;
	parentBuffer_ = nullptr;
	allocatedSize_ = 0u;
}

void ClientBuffer::resize(size_t dataSize, const byte *initialData) {
	// adjust the data size
	dataSize_ = static_cast<uint32_t>(dataSize);
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

void ClientBuffer::updateBufferSize() {
	if (!bufferSegments_.empty()) {
		uint32_t offset = dataOffset_;
		for (auto &segment : bufferSegments_) {
			// compute the offset for the segment, aligned to its base alignment.
			offset = (offset + segment->baseAlignment_ - 1) & ~(segment->baseAlignment_ - 1);
			// set the data size for the segment.
			segment->lastOffset_ = segment->dataOffset_;
			segment->dataOffset_ = offset;

			segment->updateBufferSize();
			offset += segment->dataSize_;
		}
		dataSize_ = offset + bufferSegments_.back()->dataSize_;
	}
}

void ClientBuffer::ownerResize() {
	// keep a reference to the old data slots, for copying data over.
	byte *oldData0 = dataSlots_[0];
	byte *oldData1 = dataSlots_[1];
	// compute the new data size, and update the offsets of the segments.
	// note: that some segments may need padding to align to their base alignment.
	//       so the data size of a composed client buffer might be larger than the sum of the segment sizes.
	updateBufferSize();

	// allocate new data slots.
	// TODO: Better avoid reallocation, and make it faster if possible
	// 		- using larger buffers
	//      - using a pool allocator
	//      - maybe fast re-allocation is possible?
	dataSlots_[0] = new byte[dataSize_];
	if (dataSlots_[1]) {
		dataSlots_[1] = new byte[dataSize_];
	}

	if (dataSlots_[1]) {
		resize_DoubleBuffer(
				this,
				oldData0,
				oldData1,
				dataSlots_[0],
				dataSlots_[1]);
	} else {
		resize_SingleBuffer(
				this,
				oldData0,
				dataSlots_[0]);
	}

	// delete the old data slots.
	delete[] oldData0;
	delete[] oldData1;
}

void ClientBuffer::resize_SingleBuffer(ClientBuffer *owner, const byte *oldDataPtr, byte *newDataPtr) {
	const byte *localOldDataPtr = nullptr;
	if (owner != this && dataOwner_ == this) {
		// write-lock the slots, avoiding any concurrent reads/writes.
		writeLockAll();
		localOldDataPtr = dataSlots_[0];
		oldDataPtr = localOldDataPtr;
		markWrittenTo(0, 0, dataSize_);
	}

	if (dataSize_ == allocatedSize_) {
		// no resize, just copy over the data from old to new slot.
		if (oldDataPtr) {
			std::memcpy(newDataPtr, oldDataPtr, dataSize_);
		}
		setDataPointer(owner, newDataPtr, 0);
		markWrittenTo(0, 0, dataSize_);
	} else {
		if (bufferSegments_.empty()) {
			dataSlots_[0] = newDataPtr;
		} else {
			for (auto &segment : bufferSegments_) {
				segment->resize_SingleBuffer(
					owner,
					oldDataPtr ? oldDataPtr + segment->lastOffset_ : oldDataPtr,
					newDataPtr + segment->dataOffset_);
			}
		}
		allocatedSize_ = dataSize_;
		dataOwner_ = owner;
	}

	if (localOldDataPtr) {
		// we had a local copy of the data, let's clean up the local locks,
		// and delete the local data pointer.
		readerCounts_[0].store(0, std::memory_order_release);
		readerCounts_[1].store(0, std::memory_order_release);
		writerFlags_[0].clear(std::memory_order_release);
		writerFlags_[1].clear(std::memory_order_release);
		delete[] localOldDataPtr;
	}
}

void ClientBuffer::resize_DoubleBuffer(
		ClientBuffer *owner,
		const byte *oldDataPtr0,
		const byte *oldDataPtr1,
		byte *newDataPtr0,
		byte *newDataPtr1) {
	const byte *localOldDataPtr0 = nullptr;
	const byte *localOldDataPtr1 = nullptr;
	if (owner != this && dataOwner_ == this && dataOwner_ != owner) {
		// write-lock the slots, avoiding any concurrent reads/writes.
		writeLockAll();
		localOldDataPtr0 = dataSlots_[0];
		localOldDataPtr1 = dataSlots_[1];
		oldDataPtr0 = localOldDataPtr0;
		oldDataPtr1 = localOldDataPtr1;
	}

	if (dataSize_ == allocatedSize_) {
		// no resize, just copy over the data from old to new slot.
		if (oldDataPtr0) {
			std::memcpy(newDataPtr0, oldDataPtr0, dataSize_);
			std::memcpy(newDataPtr1, oldDataPtr1, dataSize_);
		}
		setDataPointer(owner,newDataPtr0, 0);
		setDataPointer(owner,newDataPtr1, 1);
		markWrittenTo(0, 0, dataSize_);
		markWrittenTo(1, 0, dataSize_);
	} else {
		if (bufferSegments_.empty()) {
			dataSlots_[0] = newDataPtr0;
			dataSlots_[1] = newDataPtr1;
			markWrittenTo(0, 0, dataSize_);
			markWrittenTo(1, 0, dataSize_);
		} else {
			for (auto &segment : bufferSegments_) {
				segment->resize_DoubleBuffer(
					owner,
					oldDataPtr0 ? oldDataPtr0 + segment->lastOffset_ : oldDataPtr0,
					oldDataPtr1 ? oldDataPtr1 + segment->lastOffset_ : oldDataPtr1,
					newDataPtr0 + segment->dataOffset_,
					newDataPtr1 + segment->dataOffset_);
			}
		}
		allocatedSize_ = dataSize_;
		dataOwner_ = owner;
	}

	if (localOldDataPtr0) {
		// we had a local copy of the data, let's clean up the local locks,
		// and delete the local data pointer.
		readerCounts_[0].store(0, std::memory_order_release);
		readerCounts_[1].store(0, std::memory_order_release);
		writerFlags_[0].clear(std::memory_order_release);
		writerFlags_[1].clear(std::memory_order_release);
		delete[] localOldDataPtr0;
		delete[] localOldDataPtr1;
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
	auto *currentOwner = dataOwner_;

	for (auto & writerFlag : currentOwner->writerFlags_) {
		// get exclusive write access to the data slot:
		// block any attempt to write concurrently to this slot.
		while (writerFlag.test_and_set(std::memory_order_acquire)) {
			CPU_PAUSE(); // spin-wait for writers
		}
	}
	for (auto & readerCount : currentOwner->readerCounts_) {
		// wait for any active readers to finish.
		spinWaitUntil2(readerCount);
	}
}

void ClientBuffer::writeUnlockAll(uint32_t writeOffset, uint32_t writeSize) const {
	writeUnlock(1, writeOffset, 0);
	writeUnlock(0, writeOffset, writeSize);
}

int ClientBuffer::readLock() const {
	while (true) {
		// Note: ownership may change while waiting for the lock.
		auto *currentOwner = dataOwner_;
		// Get the current slot index for reading.
		// note that every writer will flip the slot index, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try, e.g.
		// because there are active writers on the slot which in turn will flip the slot index once done.
		int dataSlot = currentOwner->lastDataSlot_.load(std::memory_order_acquire);

		// First step: increment the reader count for this slot.
		// this will prevent writers from setting the flag on this slot.
		currentOwner->readerCounts_[dataSlot].fetch_add(1, std::memory_order_relaxed);

		// However, maybe there is an active writer on this slot already, we need to check that.
		if (dataOwner_->writerFlags_[dataSlot].test(std::memory_order_acquire) == 0) {
			// no writer has locked the slot, other ones are prevented from doing so,
			// hence we can safely read from this slot.
			if (dataOwner_ != currentOwner) {
				currentOwner->readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
				continue;
			}
			return dataSlot;
		}
		else {
			// Seems there is an active writer on this slot, we need to wait for them to finish.
			// but first decrement the reader count, so that we do not block writer in the meanwhile.
			currentOwner->readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
			// wait until there are no active writers on `dataSlot`.
			spinWaitUntil1(dataOwner_->writerFlags_[dataSlot]);
		}
	}
}

int ClientBuffer::writeLock_DoubleBuffer() const {
	while (true) {
		// Note: ownership may change while waiting for the lock.
		auto *currentOwner = dataOwner_;
		// get the current slot index for writing.
		// note that every writer will flip the slot index, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try.
		int dataSlot = 1 - currentOwner->lastDataSlot_.load(std::memory_order_acquire);

		// check if there are any active readers on the write slot.
		if (currentOwner->readerCounts_[dataSlot].load(std::memory_order_acquire) != 0) {
			// seems there are some remaining readers on the write slot, we need to wait for them to finish.
			spinWaitUntil2(currentOwner->readerCounts_[dataSlot]);
			continue; // try again
		}

		if (currentOwner->writerFlags_[dataSlot].test_and_set(std::memory_order_acquire)) {
			// seems someone else is writing to this slot, we need to wait for them to finish.
			spinWaitUntil1(currentOwner->writerFlags_[dataSlot]);
			continue; // try again
		} else {
			if (currentOwner->readerCounts_[dataSlot].load(std::memory_order_acquire) != 0) {
				// a reader sneaked in while we were waiting for the write lock
				currentOwner->writerFlags_[dataSlot].clear(std::memory_order_relaxed);
				continue;
			}
			if (dataOwner_ != currentOwner) {
				// data owner has changed, we need to retry.
				currentOwner->writerFlags_[dataSlot].clear(std::memory_order_relaxed);
				continue;
			}
			// we got the exclusive write lock for this slot, so we can safely write to it.
			return dataSlot;
		}
	}
}

bool ClientBuffer::readLock_SingleBuffer() const {
	// We are here in single buffer mode, and only quickly try to get a lock in the one
	// slot (with index 0), or else return false.
	// and the only thing preventing us from doing so would be a writer that is currently writing to the slot
	// which would be indicated by the writerFlags_[0] being set.
	dataOwner_->readerCounts_[0].fetch_add(1, std::memory_order_relaxed);
	if (dataOwner_->writerFlags_[0].test(std::memory_order_acquire) != 0) {
		dataOwner_->readerCounts_[0].fetch_sub(1, std::memory_order_relaxed);
		return false; // Busy writing
	} else {
		return true;
	}
}

bool ClientBuffer::writeLock_SingleBuffer() const {
	auto *currentOwner = dataOwner_;
	// acquire exclusive write lock
	if (currentOwner->writerFlags_[0].test_and_set(std::memory_order_acquire)) {
		return false; // Busy writing
	}
	// check for any active readers.
	if (currentOwner->readerCounts_[0].load(std::memory_order_acquire) != 0) {
		currentOwner->writerFlags_[0].clear(std::memory_order_relaxed);
		return false; // Busy reading
	}
	return true;
}

void ClientBuffer::readUnlock(int dataSlot) const {
	dataOwner_->readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
}

void ClientBuffer::writeUnlock(int32_t dataSlot, uint32_t writeOffset, uint32_t writeSize) const {
	if (writeSize > 0u) {
		// increment the data stamp, and remember the last slot that was written to.
		// consecutive reads will be done from this slot, next write will be done to the other slot.
		// If the write operation did not change the data, the stamp is not incremented,
		// and the last slot is not updated.
		nextStamp(dataSlot);

		if (isFrameLocked_) {
			// If frame-locked, the swap to the other slot is done centrally, not on write unlock.
			// But we still need to remember which data range was written to this frame.
			// This is done to avoid unnecessary copies.
			markWrittenTo(dataSlot, writeOffset, writeSize);
		} else {
			// swap to the other slot.
			lastDataSlot_.store(dataSlot, std::memory_order_release);
		}

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

void ClientBuffer::createSecondSlot() {
	auto data_w = new byte[dataSize_];
	std::memcpy(data_w, dataSlots_[0], dataSize_);
	dataSlots_[1] = data_w;
	// Initialize second slot stamp to the same value as the first slot.
	dataStamps_[1] = dataStamps_[0];
	REGEN_INFO("Switch to double-buffered mode"
		<< " with " << dataSize_/1024.0f << " KiB "
		<< " in " << bufferSegments_.size() << " segments.");

	// Assign second slot ptr's and offsets to all segments
	for (auto &segment : bufferSegments_) {
		segment->setDataPointer(this, data_w + segment->dataOffset_, 1);
	}

	markWrittenTo(1, 0, dataSize_);
}
