#include "client-buffer.h"
#include "regen/utility/threading.h"
#include "regen/utility/logging.h"
#include "buffer-enums.h"
#include <cstring>

using namespace regen;

//#define USE_CLIENT_BUFFER_POOL

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
	for (auto &segment: bufferSegments_) {
		segment->setFrameLocked(frameLocked);
	}
}

void ClientBuffer::setSegments(const std::vector<ref_ptr<ClientBuffer>> &segments) {
	writeLockAll();
	// clear the current segments.
	for (auto &segment: bufferSegments_) {
		segment->parentBuffer_ = nullptr;
	}
	bufferSegments_ = segments;
	for (auto &segment: bufferSegments_) {
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
	if (!isFrameLocked_ || dataSize_ == 0u) return 0u;

	int32_t lastReadSlot = lastDataSlot_.load(std::memory_order_relaxed);
	auto &dirtyLastFrame = dirtyLists_[lastReadSlot];

	if (!dataSlots_[1]) {
		// Single-buffered mode, no need to copy data.
		// Just clear the dirty list for the last read slot.
		dirtyLastFrame.clear();
		return 0u;
	} else if (dirtyLastFrame.empty()) {
		int32_t lastWriteSlot = 1 - lastReadSlot;
		auto &dirtyThisFrame = dirtyLists_[lastWriteSlot];

		if(!dirtyThisFrame.empty()) {
			// there was a write this frame -> need to do a swap
			writeLockAll();
			lastDataSlot_.store(lastWriteSlot, std::memory_order_relaxed);
			writeUnlockAll(0u, 0u);
		}

		return 0u;
	} else {
		// There were some dirty ranges in the last frame, we may need to copy the updated data
		// over to have the full data in place for reading in the next frame.
		int32_t lastWriteSlot = 1 - lastReadSlot;
		auto &dirtyThisFrame = dirtyLists_[lastWriteSlot];

		// We need to avoid race conditions of another thread getting a read lock
		// while we do the switching, then the thread may attempt (while holding read)
		// to acquire a write lock which will fail because the readers are blocking.
		// Also we need to avoid any writer adding stuff to the dirty list while we read it.
		writeLockAll();

		// Merge overlapping segments, and sort along offsets.
		// TODO: Merging of dirty frames could safely be done across padded regions,
		//       i.e. the regions that are not used to store actual data.
		//       that would reduce the number of copies needed in some cases.
		dirtyThisFrame.coalesce();
		// Delete all dirty ranges from the last read slot that have been written to this frame.
		// It is certain that both dirty lists are coalesced, so calling subtract is safe.
		dirtyLastFrame.subtract(dirtyThisFrame);

		// Remaining are the ranges where data in the write slot is not up-to-date with the read slot,
		// hence we copy it over.
		const uint32_t numCopiesNeeded = dirtyLastFrame.count();
		for (uint32_t rangeIdx = 0; rangeIdx < numCopiesNeeded; ++rangeIdx) {
			const auto &range = dirtyLastFrame.ranges()[rangeIdx];
			// Copy the data from the read slot to the write slot.
			std::memcpy(
					dataSlots_[lastWriteSlot] + range.offset,
					dataSlots_[lastReadSlot] + range.offset,
					range.size);
		}

		// For each write segment with stamp < read segment stamp: set the stamp to read segment stamp,
		// as we have synced the data above.
		dataStamps_[lastWriteSlot].store(
			dataStamps_[lastReadSlot].load(std::memory_order_relaxed),
			std::memory_order_relaxed);
		for (auto &segment: bufferSegments_) {
			auto writeStamp = segment->dataStamps_[lastWriteSlot].load(std::memory_order_relaxed);
			auto readStamp = segment->dataStamps_[lastReadSlot].load(std::memory_order_relaxed);
			if (writeStamp < readStamp) {
				segment->dataStamps_[lastWriteSlot].store(readStamp, std::memory_order_relaxed);
			}
		}

		// Finally swap read and write idx, new read idx should have new data for reading next frame.
		lastDataSlot_.store(lastWriteSlot, std::memory_order_relaxed);

		// Unlock the write locks on both slots.
		writeUnlockAll(0u, 0u);

		// clear dirty lists for the last read slot, such that it can be reused
		// next frame for writing.
		dirtyLastFrame.clear();

		return numCopiesNeeded;
	}
}

void ClientBuffer::nextStamp() const {
	uint32_t stamp = 1u + std::max(
		dataStamps_[0].load(std::memory_order_relaxed),
		dataStamps_[1].load(std::memory_order_relaxed));
	dataStamps_[0].store(stamp, std::memory_order_relaxed);
	dataStamps_[1].store(stamp, std::memory_order_relaxed);
	auto *parent = parentBuffer_;
	while (parent != nullptr) {
		stamp = 1u + std::max(
			parent->dataStamps_[0].load(std::memory_order_relaxed),
			parent->dataStamps_[1].load(std::memory_order_relaxed));
		parent->dataStamps_[0].store(stamp, std::memory_order_relaxed);
		parent->dataStamps_[1].store(stamp, std::memory_order_relaxed);
		parent = parent->parentBuffer_;
	}
}

void ClientBuffer::nextStamp(uint32_t dataSlot) const {
	auto readSlot = (dataSlots_[1] ? (1 - dataSlot) : 0);
	auto stamp = dataStamps_[readSlot].load(std::memory_order_relaxed);
	dataStamps_[dataSlot].store(stamp + 1, std::memory_order_relaxed);
	// Increase the stamp for all parent buffer ranges as well.
	auto *parent = parentBuffer_;
	while (parent) {
		stamp = parent->dataStamps_[readSlot].load(std::memory_order_relaxed);
		parent->dataStamps_[dataSlot].store(stamp + 1, std::memory_order_relaxed);
		parent = parent->parentBuffer_;
	}
}

void ClientBuffer::nextSegmentStamp(uint32_t dataSlot, uint32_t writeBegin, uint32_t writeSize) const {
	const uint32_t writeEnd = writeBegin + writeSize;

	// Update the stamp for all segments that overlap with the updated range.
	for (auto &segment: bufferSegments_) {
		if (writeBegin < segment->dataOffset_ + segment->dataSize_ && writeEnd > segment->dataOffset_) {
			// check if the segment overlaps with the updated range.
			auto readSlot = (segment->dataSlots_[1] ? (1 - dataSlot) : 0);
			segment->dataStamps_[dataSlot].store(
				segment->dataStamps_[readSlot].load(std::memory_order_relaxed) + 1,
				std::memory_order_relaxed);
		} else if (segment->dataOffset_ >= writeEnd) {
			// drop out if segment is located after the updated range
			break;
		}
	}
}

MappedClientData ClientBuffer::mapRange(int mapMode, uint32_t offset, uint32_t size) const {
	if ((mapMode & BUFFER_GPU_WRITE) != 0) {
		if (clientBufferMode_ == AdaptiveBuffer) {
			if (!hasTwoSlots()) {
				return writeRange_SingleBuffer(offset, size);
			} else {
				return writeRange_DoubleBuffer(offset, size);
			}
		} else if (clientBufferMode_ == SingleBuffer) {
			// Single-buffered mode, we can only write to the first slot.
			return writeRange_SingleBuffer(offset, size);
		} else { // clientBufferMode_ == DoubleBuffer
			// Double-buffered mode, we can write to either slot.
			return writeRange_DoubleBuffer(offset, size);
		}
	} else {
		if (clientBufferMode_ == AdaptiveBuffer) {
			if (!hasTwoSlots()) {
				return readRange_SingleBuffer(offset, size);
			} else {
				return readRange_DoubleBuffer(offset, size);
			}
		} else if (clientBufferMode_ == SingleBuffer) {
			// Single-buffered mode, we can only read from the first slot.
			return readRange_SingleBuffer(offset, size);
		} else { // clientBufferMode_ == DoubleBuffer
			// Double-buffered mode, we can read from either slot.
			return readRange_DoubleBuffer(offset, size);
		}
	}
}

MappedClientData ClientBuffer::readRange_DoubleBuffer(uint32_t offset, uint32_t /*size*/) const {
	// read lock in double-buffered mode.
	int r_index = readLock();
	return {dataSlots_[r_index] + offset, r_index};
}

MappedClientData ClientBuffer::writeRange_DoubleBuffer(uint32_t offset, uint32_t size) const {
	// we are in double-buffered mode, i.e. we have two slots.
	// partial write can be expensive here!
	// NOTE: no index mapping needed if there is only one vertex/array element
	int w_index = writeLock_DoubleBuffer();
	byte *data_w = dataSlots_[w_index];

	if (dataSize_ == size) {
		// FULL write
		return {
				dataSlots_[1 - w_index] + offset, -1,
				data_w + offset, w_index};
	} else { // PARTIAL WRITE
		if (!isFrameLocked_) {
			// we swap after each write operation, and a partial write is required.
			// make sure to copy the data from the read slot to the write slot before we do the swap.
			int r_index = readLock();
			std::memcpy(data_w, dataSlots_[r_index], dataSize_);
			readUnlock(r_index);
		}
		return {
				dataSlots_[1 - w_index] + offset, -1,
				data_w + offset, w_index};
	}
}

MappedClientData ClientBuffer::readRange_SingleBuffer(uint32_t offset, uint32_t size) const {
	// we are still in single-buffered mode.
	// first we try to get a read lock on the single slot.
	if (readLock_SingleBuffer()) {
		// got the read lock, return the data.
		return {dataSlots_[0] + offset, 0};
	} else if (isOwnerOfWriteLock(0)) {
		// Read lock failed, which means there is a write operation in progress. There are two cases:
		// (1) This thread holds the write lock on slot 0. If we wait here, then
		//     we would deadlock. But it is actually fine in this case to also
		//     read-lock the very same slot! then we can stay single-buffered.
		dataOwner_->readerCounts_[0].fetch_add(1, std::memory_order_relaxed);
		// Verify that we are still the last owner of the write lock on slot 0.
		if (!isOwnerOfWriteLock(0)) {
			dataOwner_->readerCounts_[0].fetch_sub(1, std::memory_order_relaxed);
			return mapRange(BUFFER_GPU_READ, offset, size); // retry the read operation
		}
		// Got the read lock, return the data.
		return {dataSlots_[0] + offset, 0};
	} else {
		// (2) Another thread holds the lock. Hence, it is not safe to copy data from
		//     slot 0 into slot 1 -> We need to wait until the other thread is done, then retry.
		while (dataOwner_->writerFlags_[0].test(std::memory_order_acquire) != 0) {
			// busy wait, we expect very short duration of wait here.
			CPU_PAUSE();
		}
		// The concurrent write has finished, we can give it another try.
		if (readLock_SingleBuffer()) {
			// Attempt to switch to double-buffered mode.
			// We do this here to avoid waiting like above in the future.
			if (dataOwner_->writerFlags_[1].test_and_set(std::memory_order_acquire) == 0) {
				setOwnerOfWriteLock(1);
				if (dataSlots_[1] == nullptr) {
					dataOwner_->createSecondSlot();
				}
				writeUnlock(1, 0, 0);
			}
			// Got the read lock, return the data.
			return {dataSlots_[0] + offset, 0};
		} else {
			// Failed again, retry.
			CPU_PAUSE();
			return mapRange(BUFFER_GPU_READ, offset, size);
		}
	}
}

MappedClientData ClientBuffer::writeRange_SingleBuffer(uint32_t offset, uint32_t size) const {
	if (writeLock_SingleBuffer()) {
		return {dataSlots_[0] + offset, -1, dataSlots_[0] + offset, 0};
	}
	else if (writerFlags_[0].test(std::memory_order_acquire) != 0) {
		// Another write operation is in progress on the first slot.
		// Note: If the first slot is write-locked by this thread, then we would deadlock
		// waiting here.
		if (isOwnerOfWriteLock(0)) {
			// For now, we return the data pointer without doing any locking, hoping that the original
			// write lock will be lifted after the write operation.
			return {
				dataSlots_[0] + offset, -1,
				dataSlots_[0] + offset, -1};
		}
		// If the first slot is write-locked by another thread,
		// then we need to wait for it to be unlocked.
		do {
			// busy wait, we expect very short duration of wait here.
			CPU_PAUSE();
		} while (writerFlags_[0].test(std::memory_order_acquire) != 0);
		// the concurrent write has finished, we can give it another try.
		// note that in the meantime maybe we switched to double-buffered mode.
		return mapRange(BUFFER_GPU_WRITE, offset, size);
	} else {
		// A read operation is in progress on the first slot.
		// Try also to get a read lock on the first slot such that we can
		// safely switch to double-buffered mode.
		// Note: in case all reader are in this thread, we could skip switching to double-buffered mode,
		//       but currently the thread ids of readers are not tracked.
		int r_index = readLock();
		if (r_index > 0) {
			// seems someone else allocated the second slot already.
			// release the read lock and do double-buffered write.
			readUnlock(r_index);
			return writeRange_DoubleBuffer(offset, size);
		} else if (dataOwner_->writerFlags_[1].test_and_set(std::memory_order_acquire) == 0) {
			// got a write lock on the second slot.
			setOwnerOfWriteLock(1);
			if (dataSlots_[1] == nullptr) {
				// Still single-buffered, we can finally create the second slot.
				// Note: In case of frame-locked mode, readers will continue using slot 0 for this frame.
				//       else switch to slot 1 after this write has finished.
				dataOwner_->createSecondSlot();
				return {dataSlots_[0] + offset, 0, dataSlots_[1] + offset, 1};
			} else {
				// someone else allocated the second slot already, retry.
				writeUnlock(1, 0, 0);
				readUnlock(r_index);
				return writeRange_DoubleBuffer(offset, size);
			}
		} else {
			// someone else holds the write lock on the second slot.
			readUnlock(r_index);
			if (dataSlots_[1] == nullptr) {
				// still single-buffered. retry...
				CPU_PAUSE();
				return mapRange(BUFFER_GPU_WRITE, offset, size);
			} else {
				// there is a second slot, switch to double-buffered mode.
				return writeRange_DoubleBuffer(offset, size);
			}
		}
	}
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
#ifdef USE_CLIENT_BUFFER_POOL
				getMemoryPool()->free(dataRefs_[i]);
#else
				delete[] dataSlots_[i];
#endif
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
	for (auto &segment: bufferSegments_) {
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

	// copy over initial data if any
	if (initialData) {
		std::memcpy(dataSlots_[0], initialData, dataSize);
		if (dataSlots_[1]) {
			std::memcpy(dataSlots_[1], initialData, dataSize);
		}
	}

	nextStamp();
}

void ClientBuffer::updateBufferSize() {
	// TODO: Support interleaved layouts here?
	if (!bufferSegments_.empty()) {
		uint32_t offset = 0u;
		for (auto &segment: bufferSegments_) {
			// compute the offset for the segment, aligned to its base alignment.
			offset = (offset + segment->baseAlignment_ - 1) & ~(segment->baseAlignment_ - 1);
			// set the data size for the segment.
			segment->lastOffset_ = segment->dataOffset_;
			segment->dataOffset_ = dataOffset_ + offset;

			segment->updateBufferSize();
			offset += segment->dataSize_;
		}
		dataSize_ = offset + bufferSegments_.back()->dataSize_;
		// Round total size up to next multiple of 16 (vec4 alignment for std140)
		if (memoryLayout_ == BUFFER_MEMORY_STD140) {
			static constexpr size_t std140Alignment = 16;
			dataSize_ = (dataSize_ + std140Alignment - 1) & ~(std140Alignment - 1);
		}
	}
}

ClientBufferPool *ClientBuffer::getMemoryPool(bool reset) {
	static ClientBufferPool *memoryPool = nullptr;
	if (reset && memoryPool != nullptr) {
		delete memoryPool;
		memoryPool = nullptr;
	}
	if (memoryPool == nullptr) {
		memoryPool = new ClientBufferPool();
		memoryPool->set_index(0);
		memoryPool->set_alignment(1);
		memoryPool->set_minSize(4u * 1024u * 1024u); // 4 MB blocks
	}
	return memoryPool;
}

ClientBufferPool::Node* ClientBuffer::getMemoryAllocator(uint32_t dataSize) {
	ClientBufferPool::Node *n = getMemoryPool()->chooseAllocator(dataSize);
	if (n == nullptr) {
		n = getMemoryPool()->createAllocator(dataSize);
	}
	return n;
}

ClientBufferPool *ClientBuffer::getMemoryPool() {
	return getMemoryPool(false);
}

ClientBufferPool *ClientBuffer::resetMemoryPool() {
	return getMemoryPool(true);
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
#ifdef USE_CLIENT_BUFFER_POOL
	auto oldDataRefs0 = dataRefs_[0];
	auto oldDataRefs1 = dataRefs_[1];
	auto memoryPool = ClientBuffer::getMemoryPool();
	auto *allocator = ClientBuffer::getMemoryAllocator(dataSize_);
	dataRefs_[0] = memoryPool->alloc(allocator, dataSize_);
	dataSlots_[0] = dataRefs_[0].allocatorNode->allocatorRef;
	if (clientBufferMode_ == SingleBuffer) {
		dataSlots_[1] = nullptr;
	} else if (clientBufferMode_ == DoubleBuffer) {
		dataRefs_[1] = memoryPool->alloc(allocator, dataSize_);
		dataSlots_[1] = dataRefs_[1].allocatorNode->allocatorRef;
	} else if (clientBufferMode_ == AdaptiveBuffer) {
		if (dataSlots_[1]) {
			dataRefs_[1] = memoryPool->alloc(allocator, dataSize_);
			dataSlots_[1] = dataRefs_[1].allocatorNode->allocatorRef;
		}
	}
#else
	dataSlots_[0] = new byte[dataSize_];
	if (clientBufferMode_ == SingleBuffer) {
		dataSlots_[1] = nullptr;
	} else if (clientBufferMode_ == DoubleBuffer) {
		dataSlots_[1] = new byte[dataSize_];
	} else if (clientBufferMode_ == AdaptiveBuffer) {
		if (dataSlots_[1]) {
			dataSlots_[1] = new byte[dataSize_];
		}
	}
#endif

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
#ifdef USE_CLIENT_BUFFER_POOL
	if (oldData0) memoryPool->free(oldDataRefs0);
	if (oldData1) memoryPool->free(oldDataRefs1);
#else
	delete[] oldData0;
	delete[] oldData1;
#endif
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
			for (auto &segment: bufferSegments_) {
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
#ifdef USE_CLIENT_BUFFER_POOL
		getMemoryPool()->free(dataRefs_[0]);
#else
		delete[] localOldDataPtr;
#endif
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
		// Make sure we only use the last data slot, i.e. newest data.
		oldDataPtr0 = dataSlots_[lastDataSlot_.load(std::memory_order_acquire)];
		oldDataPtr1 = oldDataPtr0;
	}
	if (!oldDataPtr1) {
		oldDataPtr1 = oldDataPtr0;
	}

	if (dataSize_ == allocatedSize_) {
		// no resize, just copy over the data from old to new slot.
		if (oldDataPtr0) {
			std::memcpy(newDataPtr0, oldDataPtr0, dataSize_);
			std::memcpy(newDataPtr1, oldDataPtr1, dataSize_);
		}
		setDataPointer(owner, newDataPtr0, 0);
		setDataPointer(owner, newDataPtr1, 1);
		markWrittenTo(currentWriteSlot(), 0, dataSize_);
	} else {
		if (bufferSegments_.empty()) {
			dataSlots_[0] = newDataPtr0;
			dataSlots_[1] = newDataPtr1;
			markWrittenTo(currentWriteSlot(), 0, dataSize_);
		} else {
			for (auto &segment: bufferSegments_) {
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
#ifdef USE_CLIENT_BUFFER_POOL
		getMemoryPool()->free(dataRefs_[0]);
		if (localOldDataPtr1 != nullptr && localOldDataPtr1 != localOldDataPtr0) {
			getMemoryPool()->free(dataRefs_[1]);
		}
#else
		delete[] localOldDataPtr0;
		if (localOldDataPtr1 != nullptr && localOldDataPtr1 != localOldDataPtr0) {
			delete[] localOldDataPtr1;
		}
#endif
	}
}

void ClientBuffer::setDataPointer(ClientBuffer *owner, byte *dataPtr, uint32_t slotIdx) const {
	// blindly assign a new data pointer to the slot at slotIdx,
	// assuming this ClientBuffer and its segments are not the data owner.
	dataSlots_[slotIdx] = dataPtr;
	dataOwner_ = owner;
	// Also set pointer on any sub-segments.
	// The sub-segment offsets are relative to the parent buffer range.
	for (auto &segment: bufferSegments_) {
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

	for (auto &writerFlag: currentOwner->writerFlags_) {
		// get exclusive write access to the data slot:
		// block any attempt to write concurrently to this slot.
		while (writerFlag.test_and_set(std::memory_order_acquire)) {
			CPU_PAUSE(); // spin-wait for writers
		}
	}
	for (auto &readerCount: currentOwner->readerCounts_) {
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
		} else {
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
			setOwnerOfWriteLock(dataSlot);
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
	setOwnerOfWriteLock(0);
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
		if (!bufferSegments_.empty()) {
			// Advance the stamp for all overlapping segments.
			nextSegmentStamp(dataSlot, writeOffset, writeSize);
		}

		if (isFrameLocked_) {
			// If frame-locked, the swap to the other slot is done centrally, not on write unlock.
			// But we still need to remember which data range was written to this frame.
			// This is done to avoid unnecessary copies.
			markWrittenTo(dataSlot, writeOffset, writeSize);
		} else {
			// swap to the other slot.
			dataOwner_->lastDataSlot_.store(dataSlot, std::memory_order_release);
		}
	}
	// clear the exclusive write lock for this slot, allowing any waiting writer to proceed.
	// NOTE: reader will only proceed once all writing is done.
	dataOwner_->writerFlags_[dataSlot].clear(std::memory_order_relaxed);
}

void ClientBuffer::markWrittenTo(uint32_t slotIdx, uint32_t offset, uint32_t size) const {
	dataOwner_->dirtyLists_[slotIdx].insert(dataOffset_+offset, size);
}

bool ClientBuffer::isOwnerOfWriteLock(int dataSlot) const {
	return dataOwner_->writerThreads_[dataSlot] == std::this_thread::get_id();
}

void ClientBuffer::setOwnerOfWriteLock(int dataSlot) const {
	dataOwner_->writerThreads_[dataSlot] = std::this_thread::get_id();
}

void ClientBuffer::createSecondSlot() {
#ifdef USE_CLIENT_BUFFER_POOL
	auto *allocator = ClientBuffer::getMemoryAllocator(dataSize_);
	dataRefs_[1] = ClientBuffer::getMemoryPool()->alloc(allocator, dataSize_);
	dataSlots_[1] = dataRefs_[1].allocatorNode->allocatorRef;
#else
	dataSlots_[1] = new byte[dataSize_];
#endif
	std::memcpy(dataSlots_[1], dataSlots_[0], dataSize_);

	// Initialize second slot stamp to the same value as the first slot.
	dataStamps_[1].store(dataStamps_[0].load(std::memory_order_relaxed), std::memory_order_relaxed);
	REGEN_INFO("Switch to double-buffered mode"
					   << " with " << dataSize_ / 1024.0f << " KiB "
					   << " in " << bufferSegments_.size() << " segments.");

	// Assign second slot ptr's and offsets to all segments
	for (auto &segment: bufferSegments_) {
		segment->setDataPointer(this, dataSlots_[1] + segment->dataOffset_, 1);
		segment->dataStamps_[1].store(
			segment->dataStamps_[0].load(std::memory_order_relaxed), std::memory_order_relaxed);
	}

	markWrittenTo(currentWriteSlot(), 0, dataSize_);
}
