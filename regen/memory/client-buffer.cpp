#include "client-buffer.h"
#include "regen/compute/threading.h"
#include "regen/utility/logging.h"
#include "buffer-enums.h"
#include <cstring>

using namespace regen;

namespace regen {
	static constexpr bool USE_CLIENT_BUFFER_POOL = false;
}

ClientBuffer::ClientBuffer() {
	// initially any data that will be allocated will be owned by this instance.
	// this is until it is added as a segment to another ClientBuffer.
	dataOwner_ = this;
}

ClientBuffer::~ClientBuffer() {
	deallocateClientData();
	bufferSegments_.clear();
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
		memoryPool->set_alignment(32);
		memoryPool->set_minSize(4u * 1024u * 1024u); // 4 MB blocks
	}
	return memoryPool;
}

ClientBufferPool *ClientBuffer::getMemoryPool() {
	return getMemoryPool(false);
}

ClientBufferPool *ClientBuffer::resetMemoryPool() {
	return getMemoryPool(true);
}

ClientBufferPool::Node* ClientBuffer::getMemoryAllocator(uint32_t dataSize) {
	ClientBufferPool::Node *n = getMemoryPool()->chooseAllocator(dataSize);
	if (n == nullptr) {
		n = getMemoryPool()->createAllocator(dataSize);
	}
	if (n == nullptr) {
		REGEN_ERROR("no allocator found for " << dataSize/1024.0 << " KB for client buffer.");
	}
	return n;
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
	if (const auto it = std::ranges::find(bufferSegments_, segment);
			it != bufferSegments_.end()) {
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

static inline uint32_t loadAtomicStamp(const CachePadded<std::atomic<uint32_t>> &stamp) {
	return stamp.value.load(std::memory_order_relaxed);
}

uint32_t ClientBuffer::swapData() {
	// NOTE: This function should be very fast as potentially both animation and rendering threads
	//       are waiting for it to finish.
	// flushing is only needed if the buffer is frame-locked.
	if (!isFrameLocked_ || dataSize_ == 0u) return 0u;

	const int32_t lastReadSlot = swapDataSlot_;
	auto &dirtyLastFrame = dirtyLists_[lastReadSlot];
	//assert(lastReadSlot == lastDataSlot_.load(std::memory_order_relaxed));

	if (!dataSlots_[1]) {
		// Single-buffered mode, no need to copy data.
		// Just clear the dirty list for the last read slot.
		dirtyLastFrame.clear();
		return 0u;
	} else if (dirtyLastFrame.empty()) {
		const int32_t lastWriteSlot = 1 - lastReadSlot;
		const auto &dirtyThisFrame = dirtyLists_[lastWriteSlot];

		if(!dirtyThisFrame.empty()) {
			// there was a write this frame -> need to do a swap
			writeLockAll();
			lastDataSlot_.store(lastWriteSlot, std::memory_order_release);
			writeUnlockAll(0u, 0u);
			swapDataSlot_ = lastWriteSlot;
		}

		return 0u;
	} else {
		// There were some dirty ranges in the last frame, we may need to copy the updated data
		// over to have the full data in place for reading in the next frame.
		const int32_t lastWriteSlot = 1 - lastReadSlot;
		const auto &dirtyThisFrame = dirtyLists_[lastWriteSlot];

		// pull the slot data pointers
		const byte* __restrict lastReadData = dataSlots_[lastReadSlot];
		byte* __restrict lastWriteData = dataSlots_[lastWriteSlot];

		// We need to avoid race conditions of another thread getting a read lock
		// while we do the switching, then the thread may attempt (while holding read)
		// to acquire a write lock which will fail because the readers are blocking.
		// Also we need to avoid any writer adding stuff to the dirty list while we read it.
		writeLockAll();

		// Delete all dirty ranges from the last read slot that have been written to this frame.
		// It is certain that both dirty lists are coalesced, so calling subtract is safe.
		dirtyLastFrame.subtract(dirtyThisFrame);
		// Coalesce the remaining dirty ranges reducing the number of copy operations needed.
		dirtyLastFrame.coalesce();

		// Remaining are the ranges where data in the write slot is not up-to-date with the read slot,
		// hence we copy it over.
		const uint32_t numCopiesNeeded = dirtyLastFrame.count();
		for (uint32_t rangeIdx = 0; rangeIdx < numCopiesNeeded; ++rangeIdx) {
			const auto &range = dirtyLastFrame.ranges()[rangeIdx];
			// Copy the data from the read slot to the write slot.
			std::memcpy(
					lastWriteData + range.offset,
					lastReadData + range.offset,
					range.size);
		}

		// For each write segment with stamp < read segment stamp: set the stamp to read segment stamp,
		// as we have synced the data above.
		const uint32_t readStamp  = loadAtomicStamp(dataStamps_[lastReadSlot]);
		const uint32_t writeStamp = loadAtomicStamp(dataStamps_[lastWriteSlot]);
		if (writeStamp < readStamp) {
			dataStamps_[lastWriteSlot].value.store(readStamp, std::memory_order_relaxed);
		}
		for (auto &segment: bufferSegments_) {
			const auto s_writeStamp = loadAtomicStamp(segment->dataStamps_[lastWriteSlot]);
			const auto s_readStamp  = loadAtomicStamp(segment->dataStamps_[lastReadSlot]);
			if (s_writeStamp < s_readStamp) {
				segment->dataStamps_[lastWriteSlot].value.store(s_readStamp, std::memory_order_relaxed);
			}
		}

		// Finally swap read and write idx, new read idx should have new data for reading next frame.
		lastDataSlot_.store(lastWriteSlot, std::memory_order_release);

		// Unlock the write locks on both slots.
		writeUnlockAll(0u, 0u);

		// Keep a cached copy of the last read slot for the next swap.
		swapDataSlot_ = lastWriteSlot;

		// clear dirty lists for the last read slot, such that it can be reused
		// next frame for writing.
		dirtyLastFrame.clear();

		return numCopiesNeeded;
	}
}

void ClientBuffer::nextStamp(int32_t writeSlot) const {
	// Increase the stamp for the given write slot to one higher than the current read slot.
	const int32_t readSlot = (dataSlots_[1] ? (1 - writeSlot) : 0);
	uint32_t stamp = loadAtomicStamp(dataStamps_[readSlot]);
	dataStamps_[writeSlot].value.store(stamp + 1, std::memory_order_relaxed);

	// Propagate to parent buffers.
	auto *parent = parentBuffer_;
	while (parent) {
		stamp = loadAtomicStamp(parent->dataStamps_[readSlot]);
		parent->dataStamps_[writeSlot].value.store(stamp + 1, std::memory_order_relaxed);
		parent = parent->parentBuffer_;
	}
}

void ClientBuffer::nextSegmentStamp(int32_t dataSlot, uint32_t writeBegin, uint32_t writeSize) const {
	const uint32_t writeEnd = writeBegin + writeSize;

	// Update the stamp for all segments that overlap with the updated range.
	for (auto &segment: bufferSegments_) {
		if (writeBegin < segment->dataOffset_ + segment->dataSize_ && writeEnd > segment->dataOffset_) {
			// check if the segment overlaps with the updated range.
			const int32_t readSlot = (segment->dataSlots_[1] ? (1 - dataSlot) : 0);
			const uint32_t readStamp = loadAtomicStamp(segment->dataStamps_[readSlot]);
			segment->dataStamps_[dataSlot].value.store(readStamp + 1, std::memory_order_relaxed);
		} else if (segment->dataOffset_ >= writeEnd) {
			// drop out if segment is located after the updated range
			break;
		}
	}
}

void ClientBuffer::nextStamp() const {
	// Increase the stamp for both slots.
	// This is only rarely used, e.g. when segments are changed or buffer resized.
	uint32_t stamp = 1u + std::max(
		loadAtomicStamp(dataStamps_[0]),
		loadAtomicStamp(dataStamps_[1]));
	dataStamps_[0].value.store(stamp, std::memory_order_relaxed);
	dataStamps_[1].value.store(stamp, std::memory_order_relaxed);

	// Propagate to parent buffers.
	auto *parent = parentBuffer_;
	while (parent != nullptr) {
		stamp = 1u + std::max(
			loadAtomicStamp(parent->dataStamps_[0]),
			loadAtomicStamp(parent->dataStamps_[1]));
		parent->dataStamps_[0].value.store(stamp, std::memory_order_relaxed);
		parent->dataStamps_[1].value.store(stamp, std::memory_order_relaxed);
		parent = parent->parentBuffer_;
	}
}

MappedClientData ClientBuffer::mapRange(int mapMode, uint32_t offset, uint32_t size) const {
	const bool singleBufferMode = (clientBufferMode_ == SingleBuffer ||
		(clientBufferMode_ == AdaptiveBuffer && !hasTwoSlots()));

	if ((mapMode & BUFFER_GPU_WRITE) != 0) {
		if (singleBufferMode) {
			// Single-buffered mode, we can only write to the first slot.
			return writeRange_SingleBuffer(offset, size);
		} else { // DoubleBuffer || (AdaptiveBuffer && isDoubleBuffered)
			// Double-buffered mode, we can write to either slot.
			return writeRange_DoubleBuffer(offset, size);
		}
	} else if (singleBufferMode) {
		// Single-buffered mode, we can only read from the first slot.
		return readRange_SingleBuffer(offset, size);
	} else { // clientBufferMode_ == DoubleBuffer
		// Double-buffered mode, we can read from either slot.
		return readRange_DoubleBuffer(offset, size);
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
			const int r_index = readLock();
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
		dataOwner_->readerCounts_[0].value.fetch_add(1, std::memory_order_relaxed);
		// Verify that we are still the last owner of the write lock on slot 0.
		if (!isOwnerOfWriteLock(0)) {
			dataOwner_->readerCounts_[0].value.fetch_sub(1, std::memory_order_relaxed);
			return mapRange(BUFFER_GPU_READ, offset, size); // retry the read operation
		}
		// Got the read lock, return the data.
		return {dataSlots_[0] + offset, 0};
	} else {
		// (2) Another thread holds the lock. Hence, it is not safe to copy data from
		//     slot 0 into slot 1 -> We need to wait until the other thread is done, then retry.
		while (dataOwner_->writerFlags_[0].value.test(std::memory_order_acquire) != 0) {
			// busy wait, we expect very short duration of wait here.
			CPU_PAUSE();
		}
		// The concurrent write has finished, we can give it another try.
		if (readLock_SingleBuffer()) {
			// Attempt to switch to double-buffered mode.
			// We do this here to avoid waiting like above in the future.
			if (dataOwner_->writerFlags_[1].value.test_and_set(std::memory_order_acquire) == 0) {
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
	else if (writerFlags_[0].value.test(std::memory_order_acquire) != 0) {
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
		} while (writerFlags_[0].value.test(std::memory_order_acquire) != 0);
		// the concurrent write has finished, we can give it another try.
		// note that in the meantime maybe we switched to double-buffered mode.
		return mapRange(BUFFER_GPU_WRITE, offset, size);
	} else {
		// A read operation is in progress on the first slot.
		// Try also to get a read lock on the first slot such that we can
		// safely switch to double-buffered mode.
		// Note: in case all reader are in this thread, we could skip switching to double-buffered mode,
		//       but currently the thread ids of readers are not tracked.
		const int r_index = readLock();
		if (r_index > 0) {
			// seems someone else allocated the second slot already.
			// release the read lock and do double-buffered write.
			readUnlock(r_index);
			return writeRange_DoubleBuffer(offset, size);
		} else if (dataOwner_->writerFlags_[1].value.test_and_set(std::memory_order_acquire) == 0) {
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
				if constexpr(USE_CLIENT_BUFFER_POOL) {
					getMemoryPool()->free(dataRefs_[i]);
				} else {
					delete[] dataSlots_[i];
				}
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

void ClientBuffer::ownerResize() {
	// keep a reference to the old data slots, for copying data over.
	byte *oldData0 = dataSlots_[0];
	byte *oldData1 = dataSlots_[1];
	// compute the new data size, and update the offsets of the segments.
	// note: that some segments may need padding to align to their base alignment.
	//       so the data size of a composed client buffer might be larger than the sum of the segment sizes.
	updateBufferSize();

	// allocate new data slots.
	if constexpr(USE_CLIENT_BUFFER_POOL) {
		auto oldDataRefs0 = dataRefs_[0];
		auto oldDataRefs1 = dataRefs_[1];
		auto memoryPool = getMemoryPool();
		auto *allocator = getMemoryAllocator(dataSize_);
		dataRefs_[0] = memoryPool->alloc(allocator, dataSize_);
		dataSlots_[0] = dataRefs_[0].allocatorNode->allocatorRef;

		if (clientBufferMode_ == SingleBuffer) {
			dataSlots_[1] = nullptr;
		} else if (clientBufferMode_ == DoubleBuffer) {
			allocator = getMemoryAllocator(dataSize_);
			dataRefs_[1] = memoryPool->alloc(allocator, dataSize_);
			dataSlots_[1] = dataRefs_[1].allocatorNode->allocatorRef;
		} else if (clientBufferMode_ == AdaptiveBuffer) {
			if (dataSlots_[1]) {
				allocator = getMemoryAllocator(dataSize_);
				dataRefs_[1] = memoryPool->alloc(allocator, dataSize_);
				dataSlots_[1] = dataRefs_[1].allocatorNode->allocatorRef;
			}
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
		if (oldData0) memoryPool->free(oldDataRefs0);
		if (oldData1) memoryPool->free(oldDataRefs1);
	} else { // not using memory pool
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

		delete[] oldData0;
		delete[] oldData1;
	}
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
		readerCounts_[0].value.store(0, std::memory_order_release);
		readerCounts_[1].value.store(0, std::memory_order_release);
		writerFlags_[0].value.clear(std::memory_order_release);
		writerFlags_[1].value.clear(std::memory_order_release);
		if constexpr(USE_CLIENT_BUFFER_POOL) {
			getMemoryPool()->free(dataRefs_[0]);
		} else {
			delete[] localOldDataPtr;
		}
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
		readerCounts_[0].value.store(0, std::memory_order_release);
		readerCounts_[1].value.store(0, std::memory_order_release);
		writerFlags_[0].value.clear(std::memory_order_release);
		writerFlags_[1].value.clear(std::memory_order_release);
		if constexpr(USE_CLIENT_BUFFER_POOL) {
			getMemoryPool()->free(dataRefs_[0]);
			if (localOldDataPtr1 != nullptr && localOldDataPtr1 != localOldDataPtr0) {
				getMemoryPool()->free(dataRefs_[1]);
			}
		} else {
			delete[] localOldDataPtr0;
			if (localOldDataPtr1 != nullptr && localOldDataPtr1 != localOldDataPtr0) {
				delete[] localOldDataPtr1;
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
	for (auto &segment: bufferSegments_) {
		segment->setDataPointer(owner, dataPtr + segment->dataOffset_, slotIdx);
	}
}

void ClientBuffer::writeLockAll() const {
	auto *owner0 = dataOwner_;

	// Indicate intent to acquire all write locks.
	// This is used to prevent new readers from acquiring read locks meanwhile.
	// However, this has one exception: if the thread that wants to acquire read
	// lock is also the owner of the write lock on the slot, then it is allowed
	// to acquire the read lock even with writeAllPending=true to avoid deadlocks.
	owner0->writeAllPending_.store(true, std::memory_order_release);

	while (true) {
		auto *currentOwner = dataOwner_;

		// If owner changed, move intent declaration to new owner
		if (currentOwner != owner0) {
			owner0->writeAllPending_.store(false, std::memory_order_release);
			owner0 = currentOwner;
			owner0->writeAllPending_.store(true, std::memory_order_release);
		}

		const int readSlot = currentOwner->lastDataSlot_.load(std::memory_order_acquire);
		const int writeSlot = 1 - readSlot;
		// pull atomics
		auto &writeFlag = currentOwner->writerFlags_[writeSlot].value;
		auto &readFlag  = currentOwner->writerFlags_[readSlot].value;
		auto &readerCount = currentOwner->readerCounts_[readSlot].value;

		// First try to acquire write lock on current write slot.
		// This will prevent any *new* attempts to write to this slot.
		if (writeFlag.test_and_set(std::memory_order_acquire)) {
			// Failed, meaning there is another active writer on this slot.
			CPU_PAUSE();
			continue; // try again
		}

		// Check if there are any active readers on the read slot.
		if (readerCount.load(std::memory_order_acquire) != 0) {
			// With active readers we must lift the lock again as it could be that the thread
			// holding the read lock will also attempt to acquire the write lock.
			writeFlag.clear(std::memory_order_release);
			CPU_PAUSE();
			continue; // try again
		}

		// Also acquire write lock on the read slot.
		// This will prevent any *new* attempts to read or write.
		if (readFlag.test_and_set(std::memory_order_acquire)) {
			// failed to acquire write lock on read slot, release write lock on write slot.
			writeFlag.clear(std::memory_order_release);
			CPU_PAUSE();
			continue; // try again
		}

		// To be safe, make sure that no readers sneaked in meanwhile on the read slot.
		if (readerCount.load(std::memory_order_acquire) != 0) {
			// Readers sneaked in, release both write locks as it is not safe to read during swapping.
			writeFlag.clear(std::memory_order_release);
			readFlag.clear(std::memory_order_release);
			CPU_PAUSE();
			continue; // try again
		}

		break; // got both write locks
	}

	// Writer now owns all slots
	owner0->writeAllPending_.store(false, std::memory_order_release);
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
		// note that the slot index can be flipped through data swapping, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try, e.g.
		// because there are active writers on the slot which in turn will flip the slot index once done.
		// Even worse, the flip can happen *during* this call, so we need to verify again after acquiring
		// the read lock that we in fact acquired the lock on the correct slot.
		const int dataSlot = currentOwner->lastDataSlot_.load(std::memory_order_acquire);
		// pull atomics
		auto &readerCount = currentOwner->readerCounts_[dataSlot].value;

		// Check if a writer is pending, if so give them priority.
		// However, if this thread is the owner of the write lock on this slot, then
		// we must ignore the intent as otherwise we definitely would deadlock in this situation!
		if (currentOwner->writeAllPending_.load(std::memory_order_acquire) &&
				currentOwner->writerThreads_[dataSlot] != std::this_thread::get_id()) {
			// there is a pending writer, we need to wait for them to finish.
			waitOnAtomic<bool,false>(currentOwner->writeAllPending_);
			continue; // try again
		}

		// First step: increment the reader count for this slot.
		readerCount.fetch_add(1, std::memory_order_relaxed);

		// However, maybe there is an active writer on this slot already, we need to check that.
		if (dataOwner_->writerFlags_[dataSlot].value.test(std::memory_order_acquire) != 0) {
			// Seems there is an active writer on this slot, we need to wait for them to finish.
			// first decrement the reader count, so that we do not block writer in the meanwhile.
			readerCount.fetch_sub(1, std::memory_order_relaxed);
			// then wait until there are no active writers on `dataSlot`.
			waitOnFlag<false>(dataOwner_->writerFlags_[dataSlot].value);
			continue;
		}

		if (dataOwner_ != currentOwner ||
				currentOwner->lastDataSlot_.load(std::memory_order_acquire) != dataSlot) {
			// data owner has changed, or the read/write slot swapped meanwhile.
			// better to retry in this case.
			readerCount.fetch_sub(1, std::memory_order_relaxed);
			CPU_PAUSE();
			continue;
		}

		// no writer has locked the slot, other ones are prevented from doing so,
		// hence we can safely read from this slot.
		return dataSlot;
	}
}

int ClientBuffer::writeLock_DoubleBuffer() const {
	while (true) {
		// Note: ownership may change while waiting for the lock.
		auto *currentOwner = dataOwner_;

		// get the current slot index for writing.
		// note that every writer will flip the slot index, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try.
		const int currentReadSlot = currentOwner->lastDataSlot_.load(std::memory_order_acquire);
		const int currentWriteSlot = 1 - currentReadSlot;
		// pull atomics
		auto &readerCount = currentOwner->readerCounts_[currentWriteSlot].value;
		auto &writeFlag = currentOwner->writerFlags_[currentWriteSlot].value;

		// check if there are any active readers on the write slot.
		if (readerCount.load(std::memory_order_acquire) != 0) {
			// seems there are some remaining readers on the write slot, we need to wait for them to finish.
			waitOnAtomic<uint32_t,0u>(readerCount);
			continue; // try again
		}

		if (writeFlag.test_and_set(std::memory_order_acquire)) {
			// seems someone else is writing to this slot, we need to wait for them to finish.
			waitOnFlag<false>(writeFlag);
			continue; // try again
		}

		if (readerCount.load(std::memory_order_acquire) != 0 ||
				dataOwner_ != currentOwner ||
				currentOwner->lastDataSlot_.load(std::memory_order_acquire) != currentReadSlot) {
			// a reader sneaked in while we were waiting for the write lock,
			// data owner has changed, or read/write slot swapped meanwhile.
			writeFlag.clear(std::memory_order_relaxed);
			CPU_PAUSE();
			continue;
		}

		// we got the exclusive write lock for this slot, so we can safely write to it.
		setOwnerOfWriteLock(currentWriteSlot);
		return currentWriteSlot;
	}
}

bool ClientBuffer::readLock_SingleBuffer() const {
	// We are here in single buffer mode, and only quickly try to get a lock in the one
	// slot (with index 0), or else return false.
	// and the only thing preventing us from doing so would be a writer that is currently writing to the slot
	// which would be indicated by the writerFlags_[0] being set.
	auto *owner = dataOwner_;

	if (owner->writeAllPending_.load(std::memory_order_acquire) &&
		owner->writerThreads_[0] != std::this_thread::get_id()) return false;

	owner->readerCounts_[0].value.fetch_add(1, std::memory_order_relaxed);
	if (owner->writerFlags_[0].value.test(std::memory_order_acquire) != 0) {
		owner->readerCounts_[0].value.fetch_sub(1, std::memory_order_relaxed);
		return false; // Busy writing
	} else {
		return true;
	}
}

bool ClientBuffer::writeLock_SingleBuffer() const {
	auto *currentOwner = dataOwner_;
	// acquire exclusive write lock
	if (currentOwner->writerFlags_[0].value.test_and_set(std::memory_order_acquire)) {
		return false; // Busy writing
	}
	// check for any active readers.
	if (currentOwner->readerCounts_[0].value.load(std::memory_order_acquire) != 0) {
		currentOwner->writerFlags_[0].value.clear(std::memory_order_relaxed);
		return false; // Busy reading
	}
	setOwnerOfWriteLock(0);
	return true;
}

void ClientBuffer::readUnlock(int dataSlot) const {
	dataOwner_->readerCounts_[dataSlot].value.fetch_sub(1, std::memory_order_relaxed);
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
	dataOwner_->writerFlags_[dataSlot].value.clear(std::memory_order_relaxed);
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
	if constexpr(USE_CLIENT_BUFFER_POOL) {
		auto *allocator = ClientBuffer::getMemoryAllocator(dataSize_);
		dataRefs_[1] = ClientBuffer::getMemoryPool()->alloc(allocator, dataSize_);
		dataSlots_[1] = dataRefs_[1].allocatorNode->allocatorRef;
	} else {
		dataSlots_[1] = new byte[dataSize_];
	}
	std::memcpy(dataSlots_[1], dataSlots_[0], dataSize_);

	// Initialize second slot stamp to the same value as the first slot.
	dataStamps_[1].value.store(loadAtomicStamp(dataStamps_[0]), std::memory_order_relaxed);
	REGEN_INFO("Switch to double-buffered mode"
					   << " with " << dataSize_ / 1024.0f << " KiB "
					   << " in " << bufferSegments_.size() << " segments.");

	// Assign second slot ptr's and offsets to all segments
	for (auto &segment: bufferSegments_) {
		segment->setDataPointer(this, dataSlots_[1] + segment->dataOffset_, 1);
		segment->dataStamps_[1].value.store(
			loadAtomicStamp(segment->dataStamps_[0]), std::memory_order_relaxed);
	}

	markWrittenTo(currentWriteSlot(), 0, dataSize_);
}
