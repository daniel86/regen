#include "staged-buffer.h"
#include "ubo.h"
#include "ssbo.h"
#include "regen/scene/state.h"
#include "../shader/input-processor.h"

using namespace regen;

//#define REGEN_DISABLE_GLOBAL_STAGING
//#define REGEN_DISABLE_EXPLICIT_FLUSHING
//#define REGEN_FORCE_IMPLICIT_STAGING
#define REGEN_FORCE_CLIENT_DOUBLE_BUFFER

uint32_t StagedBuffer::MIN_SEGMENTS_PARTIAL_TEMPORARY = 6;
float StagedBuffer::MAX_UPDATE_RATIO_PARTIAL_TEMPORARY = 0.33f;
// Default to 60 frames for update rate computation.
uint32_t StagedBuffer::UPDATE_RATE_RANGE = 60;

StagedBuffer::StagedBuffer(
		std::string_view name,
		BufferTarget target,
		const BufferUpdateFlags &hints,
		BufferMemoryLayout memoryLayout)
		: BufferObject(target, hints),
		  ShaderInput(name, GL_INVALID_ENUM, 0, 0, 0, false),
		  stagingFlags_(target, hints) {
	memoryLayout_ = memoryLayout;
	isStagedBuffer_ = true;
	clientBuffer_->setMemoryLayout(memoryLayout_);

	shared_ = ref_ptr<Shared>::alloc();
	shared_->updateRange_ = UPDATE_RATE_RANGE;
	shared_->f_updateRangeInv_ = 1.0f / static_cast<float>(UPDATE_RATE_RANGE);
	shared_->updatedFrames_ = new bool[UPDATE_RATE_RANGE];
	std::fill(
			shared_->updatedFrames_,
			shared_->updatedFrames_ + UPDATE_RATE_RANGE,
			false);
	drawBufferRange_ = ref_ptr<BufferRange>::alloc();
	// initially assume it is a GPU-only buffer.
	// the flag will be switched to something else based on the inputs added.
	setClientAccessMode(BUFFER_GPU_ONLY);
	setBufferMapMode(BUFFER_MAP_DISABLED);
	// if the buffer will never be updated, we can use implicit staging.
	if (hints.frequency == BUFFER_UPDATE_NEVER) {
		setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
	}
	clientBuffer_->setFrameLocked(hints.frequency < BUFFER_UPDATE_PER_DRAW);
#ifdef REGEN_FORCE_IMPLICIT_STAGING
	setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
#endif
	adoptBufferRange_ = [this](uint32_t requiredSize) {
		return adoptBufferRange(requiredSize);
	};
}

static std::string_view getName(const StagedBuffer &other, std::string_view name) {
	return name.empty() ? other.name() : name;
}

StagedBuffer::StagedBuffer(const StagedBuffer &other, std::string_view name)
		: BufferObject(other),
		  ShaderInput(getName(other,name), GL_INVALID_ENUM, 0, 0, 0, false),
		  stagingFlags_(other.bufferTarget(), other.bufferUpdateHints()) {
	memoryLayout_ = other.memoryLayout_;
	hasClientData_ = other.hasClientData_;
	isBufferValid_ = other.isBufferValid_;
	inputs_ = other.inputs_;
	drawBufferRef_ = other.drawBufferRef_;
	drawBufferRange_ = other.drawBufferRange_;
	requiredSize_ = other.requiredSize_;
	estimatedSize_ = other.estimatedSize_;
	stamp_ = other.stamp_;
	stagedInputs_ = other.stagedInputs_;
	stagingFlags_ = other.stagingFlags_;
	userDefinedBufferingMode_ = other.userDefinedBufferingMode_;
	shared_ = other.shared_;
	shared_->copyCount_.fetch_add(1, std::memory_order_relaxed);
	clientBuffer_ = other.clientBuffer_;
	adoptBufferRange_ = other.adoptBufferRange_;
	isStagedBuffer_ = true;
	isVertexAttribute_ = false;
	isVertexAttribute_ = false;
}

StagedBuffer::~StagedBuffer() {
	if (shared_->copyCount_.fetch_sub(1) == 1) {
		if (shared_->isGloballyStaged_) {
			StagingSystem::instance().removeBufferBlock(this);
		}
	}
}

void StagedBuffer::setBufferingMode(BufferingMode mode) {
	userDefinedBufferingMode_ = mode;
	stagingFlags_.bufferingMode = mode;
}

void StagedBuffer::setStagingMapMode(BufferMapMode mode) {
	if (!flags_.useExplicitStaging()) {
		// if we are not using separate staging buffers, we need to set the same map mode
		// for the main buffer as well.
		setBufferMapMode(mode);
		stagingFlags_.mapMode = flags_.mapMode;
	} else {
		stagingFlags_.mapMode = mode;
	}
}

void StagedBuffer::setStagingAccessMode(ClientAccessMode mode) {
	if (!flags_.useExplicitStaging()) {
		// if we are not using separate staging buffers, we need to set the same access mode
		// for the main buffer as well.
		setClientAccessMode(mode);
		stagingFlags_.accessMode = flags_.accessMode;
	} else {
		stagingFlags_.accessMode = mode;
	}
}

void StagedBuffer::enableWriteAccess() {
	if (stagingFlags_.accessMode == BUFFER_GPU_ONLY) {
		// if the access mode is GPU-only, we need to switch it to CPU_WRITE
		setStagingAccessMode(BUFFER_CPU_WRITE);
	} else if (stagingFlags_.accessMode == BUFFER_CPU_READ) {
		REGEN_WARN("Attempting to enable write access on a buffer that is CPU_READ only.");
	}
}

void StagedBuffer::setStagingBuffering(BufferingMode mode) {
	if (!userDefinedBufferingMode_.has_value()) {
		stagingFlags_.bufferingMode = mode;
	}
}

void StagedBuffer::updateStorageFlags() {
	if (!hasClientData_) return;
	if (flags_.updateHints.frequency == BUFFER_UPDATE_NEVER) return;

	// update the storage flags based on added inputs
	auto sizeClass = StagingBuffer::getBufferSizeClass(estimatedSize_);
	// input has client data, so we need to set the access mode such that the CPU can write to it.
	enableWriteAccess();
	setStagingBuffering(SINGLE_BUFFER);
#ifdef REGEN_FORCE_CLIENT_DOUBLE_BUFFER
	clientBuffer_->setClientBufferMode(ClientBuffer::DoubleBuffer);
#endif

	// NOTE: in local staging we avoid persistent mapping the buffer to CPU memory to avoid performance issues
	//       with fencing, as currently local staging uses per-BO and per-segment fences which is overkill
	//       for most cases.
	if (sizeClass == BUFFER_SIZE_SMALL) {
		// If the buffer is small (e.g. < 512 Byte), then ..
		setStagingMapMode(BUFFER_MAP_TEMPORARY);
		if (stagingFlags_.updateHints.frequency >= BUFFER_UPDATE_PER_FRAME) {
			setStagingBuffering(DOUBLE_BUFFER);
		}
	} else if (sizeClass == BUFFER_SIZE_MEDIUM) {
		// If the buffer is medium-sized (e.g. < 64KB), then ...
		if (stagingFlags_.areUpdatesFrequent()) {
			setStagingMapMode(BUFFER_MAP_TEMPORARY);
			setStagingBuffering(DOUBLE_BUFFER);
		} else if (stagingFlags_.areUpdatesVeryFrequent()) {
			// The current local fencing would not work well with very frequent updates!
			// So better use temporary mapping in this case.
			setStagingMapMode(BUFFER_MAP_TEMPORARY);
			setStagingBuffering(TRIPLE_BUFFER);
		} else {
			// Use single-buffering in staging with unmapped copy or temporary mapping for infrequent updates.
			if (stagingFlags_.areUpdatesPartial()) {
				setStagingMapMode(BUFFER_MAP_DISABLED);
			} else {
				setStagingMapMode(BUFFER_MAP_TEMPORARY);
			}
		}
	} else if (sizeClass == BUFFER_SIZE_LARGE) {
		// If the buffer is large (e.g. < 1MB)
		if (stagingFlags_.areUpdatesFrequent()) {
			// If updates are frequent, then use range invalidation.
			setStagingBuffering(DOUBLE_BUFFER);
			setStagingMapMode(BUFFER_MAP_TEMPORARY);
		} else if (stagingFlags_.areUpdatesVeryFrequent()) {
			setStagingMapMode(BUFFER_MAP_TEMPORARY);
		} else {
			// If updates are infrequent, then use single-buffering in implicit staging and avoid mapping
			// the buffer to CPU memory.
			setStagingMapMode(BUFFER_MAP_DISABLED);
			setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
		}
	} else { // sizeClass == BUFFER_SIZE_VERY_LARGE
		// If the buffer is very large (e.g. > 1MB), avoid mapping it to CPU memory.
		setStagingMapMode(BUFFER_MAP_DISABLED);
		if (stagingFlags_.areUpdatesRare()) {
			setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
		}
	}
}

void StagedBuffer::addStagedInput(const ref_ptr<ShaderInput> &input, std::string_view name) {
	auto &bufferInput = stagedInputs_.emplace_back();
	bufferInput.input = input.get();
	inputs_.emplace_back(input, std::string(name));
	estimatedSize_ += input->elementSize();
	hasClientData_ = input->hasClientData() && hasClientData_;
	input->setMemoryLayout(memoryLayout_);
	if (clientBuffer_->hasSegments()) {
		clientBuffer_->addSegment(input->clientBuffer());
	}

	updateStorageFlags();
}

void StagedBuffer::removeStagedInput(std::string_view name) {
	for (auto it = stagedInputs_.begin(); it != stagedInputs_.end(); ++it) {
		auto &blockInput = *it;
		if (blockInput.input->name() == name) {
			// remove the input from the inputs_ vector
			for (auto inputIt = inputs_.begin(); inputIt != inputs_.end(); ++inputIt) {
				if (inputIt->name_ == name) {
					inputs_.erase(inputIt);
					break;
				}
			}
			// remove the block input
			stagedInputs_.erase(it);
			if (clientBuffer_->hasSegments()) {
				// remove the segment from the client buffer
				clientBuffer_->removeSegment(blockInput.input->clientBuffer());
			}
			return;
		}
	}
	REGEN_WARN("Unable to remove input '" << name << "'. Input not found.");
}

void StagedBuffer::update(bool forceUpdate) {
	if (!isBufferValid_) return;
	updateStagedInputs();
	updateDrawBuffer();
	if (!shared_->isGloballyStaged_) {
		// note: don't mess with the staging buffer if it is managed by the staging system.
		// i.e. in case someone explicitly called update() on the buffer block.
		copyStagingData(forceUpdate);
	}
}

uint32_t StagedBuffer::getLastInputStamp(const StagedInput &blockInput) const {
	const auto &buffer = shared_->stagingBuffer_;
	const uint32_t stampIdx = (buffer.get() != nullptr) ? buffer->nextWriteIndex() : 0u;
	return blockInput.lastStamp[stampIdx];
}

void StagedBuffer::setLastInputStamp(StagedInput &blockInput) const {
	const auto &buffer = shared_->stagingBuffer_;
	const uint32_t stampIdx = (buffer.get() != nullptr) ? buffer->nextWriteIndex() : 0u;
	blockInput.lastStamp[stampIdx] = blockInput.input->stampOfReadData();
}

uint32_t StagedBuffer::updateStagedInputs() {
	const uint32_t numStagedInputs = static_cast<uint32_t>(stagedInputs_.size());
	const StagedInput* stagedInputs = stagedInputs_.data();

	// whether the size of the block has changed
	bool hasNewSize = (requiredSize_ == 0);
	// whether all inputs have client data, and we have at least one input.
	bool hasClientData = (numStagedInputs != 0);
	for (uint32_t i = 0; i < numStagedInputs; ++i) {
		const ShaderInput &in = *stagedInputs[i].input;
		hasNewSize |= (stagedInputs[i].inputSize != in.alignedInputSize());
		hasClientData &= in.hasClientData();
	}
	hasClientData_ = hasClientData;

	//  Initialize the client buffer here lazily.
	if (!clientBuffer_->hasSegments() && hasClientData) [[unlikely]] {
		std::vector<ref_ptr<ClientBuffer>> segments(numStagedInputs);
		for (uint32_t i = 0; i < numStagedInputs; ++i) {
			segments[i] = stagedInputs[i].input->clientBuffer();
		}
		clientBuffer_->setSegments(segments);
		clientBuffer_->swapData();
	}
	// Update required size if needed.
	if (hasNewSize) [[unlikely]] {
		updateRequiredSize();
	}

	// Update dirty segments.
	const bool isDirty = updateDirtySegments();
	// Remember if we had a dirty segment in this frame for computing the update rate.
	// this is useful for detecting stalls in the staging system, for adaptive ring buffering.
	setUpdatedFrame(isDirty);

	return requiredSize_;
}

void StagedBuffer::updateRequiredSize() {
	const uint32_t numStagedInputs = static_cast<uint32_t>(stagedInputs_.size());
	StagedInput* stagedInputs = stagedInputs_.data();

	requiredSize_ = 0;
	for (size_t i = 0; i < numStagedInputs; ++i) {
		StagedInput &blockInput = stagedInputs[i];
		const ShaderInput &in = *blockInput.input;
		// Align the offset to the required alignment
		// baseAlignment is always a power of two, so we can use bitwise AND
		requiredSize_ = (requiredSize_ + in.baseAlignment() - 1) & ~(in.baseAlignment() - 1);
		blockInput.offset = requiredSize_;
		blockInput.inputSize =  in.alignedInputSize();
		requiredSize_ += blockInput.inputSize;
	}
	// Round total size up to next multiple of 16 (vec4 alignment for std140)
	if (memoryLayout_ == BUFFER_MEMORY_STD140) {
		static constexpr size_t std140AlignmentMinOne = 15; // 16 - 1
		requiredSize_ = (requiredSize_ + std140AlignmentMinOne) & ~(std140AlignmentMinOne);
	}
}

bool StagedBuffer::updateDirtySegments() {
	const uint32_t numStagedInputs = static_cast<uint32_t>(stagedInputs_.size());
	auto* stagedInputs = stagedInputs_.data();

	if (dirtySegmentRanges_.size() < numStagedInputs) [[unlikely]] {
		// resize the dirty arrays if needed
		dirtySegmentRanges_.resize(numStagedInputs + 4);
		dirtyBufferRanges_.resize(numStagedInputs + 4);
	}

	auto* __restrict bufferRanges = dirtyBufferRanges_.data();
	auto* __restrict segmentRanges = dirtySegmentRanges_.data();

	// number of dirty segments found
	uint32_t numDirtySegments = 0u;
	uint32_t dirtyBytes = 0u;
	// whether the last input changed or not
	bool lastChanged = false;

	for (uint32_t inputIdx = 0u; inputIdx < numStagedInputs; ++inputIdx) {
		StagedInput &blockInput = stagedInputs[inputIdx];
		const bool thisChanged = blockInput.input->stampOfReadData() != getLastInputStamp(blockInput);
		if (thisChanged && lastChanged) {
			// this input adds to the current segment
			const uint32_t dirtyIdx = numDirtySegments - 1;
			auto &dirty_s = segmentRanges[dirtyIdx];
			auto &dirty_b = bufferRanges[dirtyIdx];
			dirty_b.size = blockInput.offset - dirty_b.offset + blockInput.inputSize;
			dirty_s.endIdx = inputIdx+1;
			dirtyBytes += blockInput.inputSize;
		}
		else if (thisChanged) {
			// this input starts a new segment
			const uint32_t dirtyIdx = numDirtySegments++;
			auto &dirty_s = segmentRanges[dirtyIdx];
			auto &dirty_b = bufferRanges[dirtyIdx];
			dirty_b.offset = blockInput.offset;
			dirty_b.size = blockInput.inputSize;
			dirty_s.startIdx = inputIdx;
			dirty_s.endIdx = inputIdx+1;
			dirtyBytes += blockInput.inputSize;
		}
		lastChanged = thisChanged;
	}

	numDirtySegments_ = numDirtySegments;
	updatedSize_ = dirtyBytes;
	return numDirtySegments > 0;
}

void StagedBuffer::copyDirtyData(byte* __restrict dstData, uint32_t localMapOffset) {
	const uint32_t fullSize = clientBuffer_->dataSize();
	const uint32_t numDirtySegments = numDirtySegments_;

	auto* __restrict stagedInputs = stagedInputs_.data();
	const auto* __restrict bufferRanges = dirtyBufferRanges_.data();
	const auto* __restrict segmentRanges = dirtySegmentRanges_.data();

	auto mapped = clientBuffer_->mapRange(BUFFER_GPU_READ, 0u, fullSize);
	const byte* __restrict srcData = mapped.r;

	for (uint32_t segmentIdx = 0; segmentIdx < numDirtySegments; ++segmentIdx) {
		// Copy the whole segment range at once.
		const auto &dirtyRange = bufferRanges[segmentIdx];
		// NOTE: The main buffer maybe is not mapped from the start if the adopted buffer range, e.g.
		//       in case starts at first dirt segment. However, the block input offsets are always
		//       relative to the start of the buffer, so we need to adjust the offset accordingly...
		const uint32_t dstOffset = dirtyRange.offset - localMapOffset;
		memcpy(
			dstData + dstOffset,
			srcData + dirtyRange.offset,
			dirtyRange.size);

		const auto &segmentRange = segmentRanges[segmentIdx];
		for (uint32_t inputIdx = segmentRange.startIdx; inputIdx < segmentRange.endIdx; ++inputIdx) {
			setLastInputStamp(stagedInputs[inputIdx]);
		}
	}

	clientBuffer_->unmapRange(BUFFER_GPU_READ, 0u, fullSize, mapped.r_index);
}

void StagedBuffer::copyFullData(byte *mappedBufferData, uint32_t localMapOffset) {
	// full write of mapped range
	// get start and end indices from first and last segment
	uint32_t startIdx = dirtySegmentRanges_[0].startIdx;
	uint32_t endIdx   = dirtySegmentRanges_[numDirtySegments_ - 1].endIdx;
	auto &firstSegment = stagedInputs_[startIdx];
	auto &lastSegment = stagedInputs_[endIdx - 1];

	// copy the whole range of block inputs.
	const auto fullSize = clientBuffer_->dataSize();
	auto mapped = clientBuffer_->mapRange(BUFFER_GPU_READ, 0u, fullSize);

	const uint32_t offset = firstSegment.offset - localMapOffset;
	memcpy(mappedBufferData + offset,
		   mapped.r + firstSegment.offset,
		   lastSegment.offset + lastSegment.inputSize - firstSegment.offset);

	// update the last stamps for all inputs in the dirty range
	for (uint32_t inputIdx = startIdx; inputIdx < endIdx; ++inputIdx) {
		setLastInputStamp(stagedInputs_[inputIdx]);
	}
	clientBuffer_->unmapRange(BUFFER_GPU_READ, 0u, fullSize, mapped.r_index);
}

void StagedBuffer::markBufferDirty() {
	numDirtySegments_ = 0;
	createNextDirtySegment();
	dirtyBufferRanges_[0].offset = 0;
	dirtyBufferRanges_[0].size = requiredSize_;
	dirtySegmentRanges_[0].startIdx = 0;
	dirtySegmentRanges_[0].endIdx = static_cast<uint32_t>(stagedInputs_.size());
}

void StagedBuffer::resetDataStamps() {
	// reset the last stamps for all inputs and segments.
	for (auto &input: stagedInputs_) {
		std::memset(input.lastStamp.data(), 0, input.lastStamp.size() * sizeof(uint32_t));
	}
}

void StagedBuffer::queueStagingUpdate() {
	if (!shared_->isGloballyStaged_) {
		// reset the local staging buffer, causing it to be reinitialized
		shared_->stagingBuffer_ = {};
	}
	// on resize, create one dirty segment that covers the whole buffer.
	// also reset the last stamps for all inputs and segments.
	markBufferDirty();
	resetDataStamps();
}

void StagedBuffer::updateDrawBuffer() {
	if (adoptedSize_ == requiredSize_) {
		// nothing to do, the draw buffer is already up-to-date
		return;
	}
	if (drawBufferRef_.get()) {
		orphanBufferRange(drawBufferRef_.get());
	}

	// if neither read nor write access is requested, we can use implicit staging.
	if (!stagingFlags_.isWritable() && !stagingFlags_.isReadable()) {
		setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
	}

	if (flags_.useExplicitStaging()) {
		drawBufferRef_ = adoptBufferRange_(requiredSize_);
	} else {
		// note: in case of implicit staging with multi-buffering, we need to allocate space for each segment.
		if (flags_.bufferingMode == RING_BUFFER) {
			REGEN_WARN("Ring buffer is not supported with implicit staging. "
					   "Switching to TRIPLE_BUFFER.");
			flags_.bufferingMode = TRIPLE_BUFFER;
		}
		drawBufferRef_ = adoptBufferRange_(requiredSize_ * (int) flags_.bufferingMode);
	}

	// validate the allocation
	if (!drawBufferRef_.get()) {
		REGEN_ERROR("failed to allocate buffer for buffer flags " << flags_);
		isBufferValid_ = false;
		return;
	}
	if (isMapModePersistent(flags_.mapMode) && !drawBufferRef_->mappedData()) {
		REGEN_WARN("something went wrong with persistent mapping for buffer flags " << flags_);
		isBufferValid_ = false;
		return;
	}
	isBufferValid_ = true;

	adoptedSize_ = requiredSize_;
	inputSize_ = requiredSize_;
	// set draw buffer range to first segment in the ring buffer
	drawBufferRange_->buffer_ = drawBufferRef_->bufferID();
	drawBufferRange_->size_ = requiredSize_;
	drawBufferRange_->offset_ = drawBufferRef_->address();
	queueStagingUpdate();

	if (hasClientData()) {
		// Copy over client data initially into the main buffer.
		// This is done to ensure that the draw buffer has some initial data
		// that can be drawn before the staging buffer is filled.
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		setBufferData(mapped.r);
	}

	// set up the inputs to point to the correct offsets in the draw buffer
	const uint32_t drawBufferOffset = drawBufferRef_->address();
	uint32_t bufferByteOffset = 0u;
	for (auto &staged: stagedInputs_) {
		ShaderInput &att = *staged.input;
		// Fulfill requirement for alignment of starting offset of attribute data,
		// i.e. the start byte of the data must be a multiple of the base alignment.
		const uint32_t nextAlignment = att.baseAlignment() - 1;
		bufferByteOffset = (bufferByteOffset + nextAlignment) & ~nextAlignment;
		// set the main buffer reference for the input
		att.setMainBuffer(drawBufferRef_, drawBufferOffset + bufferByteOffset);
		bufferByteOffset += att.alignedInputSize();
	}

	REGEN_DEBUG("Created "
					   << StagingBuffer::getBufferSizeClass(requiredSize_)
					   << " " << stagingFlags_.target
					   << " \"" << name() << "\" with"
					   << " " << requiredSize_ / 1024.0 << " Kib"
					   << " BO: " << drawBufferRef_->bufferID()
					   << " at: " << drawBufferRef_->address());
}

void StagedBuffer::setStagingOffset(uint32_t offset) {
	shared_->stagingOffset_ = offset;
	queueStagingUpdate();
}

void StagedBuffer::resetStagingBuffer(bool removeFromStagingSystem) {
	if (removeFromStagingSystem && shared_->isGloballyStaged_) {
		StagingSystem::instance().removeBufferBlock(this);
	}
	shared_->stagingBuffer_ = {};
	shared_->isGloballyStaged_ = false;
}

void StagedBuffer::createStagingBuffer() {
#ifdef REGEN_DISABLE_GLOBAL_STAGING
	// disable global staging, falling back to local staging buffer.
	ref_ptr<StagingBuffer> buf;
#else
	auto buf = StagingSystem::instance().addBufferBlock(this);
#endif
	shared_->stagingOffset_ = 0;
	if (buf.get() != nullptr) {
		// the block was added to the staging system.
		// the system will globally manage updates and resizes of the staging buffer.
		shared_->stagingBuffer_ = buf;
		shared_->isGloballyStaged_ = true;
		stagingFlags_ = shared_->stagingBuffer_->stagingFlags();
	} else {
		// create a local staging buffer exclusively for this block.
		shared_->stagingBuffer_ = ref_ptr<StagingBuffer>::alloc(stagingFlags_);
		if (StagingBuffer::getBufferSizeClass(requiredSize_) < BUFFER_SIZE_LARGE) {
			shared_->stagingBuffer_->setMaxRingSegments(16);
		} else {
			shared_->stagingBuffer_->setMaxRingSegments(4);
		}
		shared_->stagingBuffer_->resizeBuffer(requiredSize_, 2);
		shared_->isGloballyStaged_ = false;
		REGEN_INFO("Using local staging for block \""
			<< name() << "\" with size " << requiredSize_ / 1024.0 << " Kib"
			<< " and " << shared_->stagingBuffer_->numBufferSegments()
			<< " segments.");
		REGEN_INFO("Local staging flags: " << stagingFlags_);
	}
}

void StagedBuffer::copyStagingData(bool forceUpdate) {
	if (forceUpdate) { markBufferDirty(); }
	bool needsUpdate = (numDirtySegments_ > 0);
	// advance update history
	setUpdatedFrame(needsUpdate);
	if (!needsUpdate) { return; }

	// lazy initialization of the staging buffer
	if (shared_->stagingBuffer_.get() == nullptr) {
		createStagingBuffer();
	} else if (!shared_->isGloballyStaged_ &&
			   isMapModePersistent(stagingFlags_.mapMode) &&
			   stagingFlags_.bufferingMode == RING_BUFFER) {
		// increase number of ring buffer segments if fence is stalled too much,
		// and we are not globally staged. in case of global staging, the system
		// will handle the resizing of the staging buffer.
		if (shared_->stagingBuffer_->getStallRate() > StagingBuffer::MAX_ACCEPTABLE_STALL_RATE) {
			const uint32_t currentNumSegments = shared_->stagingBuffer_->numBufferSegments();
			const uint32_t desiredNumSegments = currentNumSegments + 1;
			REGEN_INFO("Resizing local staging buffer for block \""
							   << name() << "\" to " << desiredNumSegments
							   << " segments due to high stall rate.");
			if (desiredNumSegments < shared_->stagingBuffer_->maxRingSegments()) {
				shared_->stagingBuffer_->resizeBuffer(
						requiredSize_,
						desiredNumSegments);
				shared_->stagingBuffer_->resetStallRate();
			}
		}
	}
	if (!shared_->stagingBuffer_->hasAdoptedRange() && stagingFlags_.useExplicitStaging()) {
		// the staging buffer is not initialized yet, probably globally managed by the staging system,
		// but the system is not ready yet. Or implicit staging is used.
		// So we need to wait until the system is ready.
		return;
	}

	const uint32_t numStagingSegments = shared_->stagingBuffer_->numBufferSegments();
	if (shared_->numBufferSegments_ != numStagingSegments) {
		// the number of segments in the staging buffer has changed, so we need to resize the
		// last stamp vector for each input.
		// we reset the stamps causing a re-load of all segments.
		for (auto &input: stagedInputs_) {
			input.lastStamp.resize(numStagingSegments);
		}
		resetDataStamps();
		shared_->numBufferSegments_ = numStagingSegments;
	}

	if (shared_->stagingBuffer_->stagingFlags().isReadable()) {
		// Copy from draw buffer to the staging buffer, then read from the staging buffer into CPU memory.
		if (!updateReadBuffer()) {
			REGEN_WARN("Failed to update read buffer for block \""
							   << name() << "\". This is likely a bug, buffer object will be disabled."
							   << " Staging flags: " << stagingFlags_ << ".");
			isBufferValid_ = false;
		}
	} else if (hasClientData_) {
		// Write from CPU memory to the staging buffer, and then copy to the draw buffer.
		// Note: this is even done in case the staging storage is not writable, in which
		//       case a temporary writable buffer range is adopted.
		if (isMapModePersistent(stagingFlags_.mapMode)) {
			updatePersistentMapped();
		} else if (stagingFlags_.mapMode == BUFFER_MAP_TEMPORARY) {
			updateTemporaryMapped();
		} else {
			updateNonMapped();
		}
	} else if (stagingFlags_.useExplicitStaging()) {
		REGEN_WARN("No client data to update BO \""
						   << name() << "\". This is likely a bug, buffer object will be disabled."
						   << " Staging flags: " << stagingFlags_ << ".");
		isBufferValid_ = false;
	}
}

void StagedBuffer::updateNonMapped() {
	// iterate over the changed segments and copy only those into the staging buffer.
	shared_->stagingBuffer_->beginNonMappedWrite();

	const auto dataSize = clientBuffer_->dataSize();
	auto mapped = clientBuffer_->mapRange(BUFFER_GPU_READ, 0u, dataSize);

	for (uint32_t dirtyIdx = 0; dirtyIdx < numDirtySegments_; ++dirtyIdx) {
		auto &dirtyRange_s = dirtySegmentRanges_[dirtyIdx];
		auto &dirtyRange_b = dirtyBufferRanges_[dirtyIdx];
		const uint32_t localOffset = shared_->stagingOffset_ + dirtyRange_b.offset;

		shared_->stagingBuffer_->setSubData(
				drawBufferRef_,
				localOffset,
				dirtyRange_b.size,
				mapped.r + dirtyRange_b.offset);

		// update the last stamps for all inputs in the dirty range
		for (uint32_t inputIdx = dirtyRange_s.startIdx; inputIdx < dirtyRange_s.endIdx; ++inputIdx) {
			setLastInputStamp(stagedInputs_[inputIdx]);
		}
	}

	clientBuffer_->unmapRange(BUFFER_GPU_READ, 0u, dataSize, mapped.r_index);
	shared_->stagingBuffer_->endNonMappedWrite(
			drawBufferRef_,
			*drawBufferRange_.get(),
			shared_->stagingOffset_);
	stamp_ += 1;
}

void StagedBuffer::updateTemporaryMapped() {
	// Selectively enable partial updates.
	// However, note that we need to do multiple mappings in case of partial updates,
	// as we need to always should use range invalidation for the mapped range.
	// In case of full updates, we can map the whole buffer range at once with invalidation.
	const bool doPartialUpdate = (numDirtySegments_ > 1) &&
								 // disable partial updates for small buffers, as they are fast to update anyway.
								 (StagingBuffer::getBufferSizeClass(estimatedSize_) > BUFFER_SIZE_SMALL) &&
								 // disable partial updates in case the update covers a large part of the buffer.
								 (updatedSize_ / static_cast<float>(requiredSize_) >
								  BufferBlock::MAX_UPDATE_RATIO_PARTIAL_TEMPORARY) &&
								 // disable partial updates for larger number of dirty segments.
								 (numDirtySegments_ <= BufferBlock::MIN_SEGMENTS_PARTIAL_TEMPORARY);

	if (doPartialUpdate) {
		for (uint32_t dirtyIdx = 0; dirtyIdx < numDirtySegments_; ++dirtyIdx) {
			auto &dirtyRange_s = dirtySegmentRanges_[dirtyIdx];
			auto &dirtyRange_b = dirtyBufferRanges_[dirtyIdx];
			const uint32_t localOffset = shared_->stagingOffset_ + dirtyRange_b.offset;
			byte *bufferData = shared_->stagingBuffer_->beginMappedWrite(
					drawBufferRef_,
					false,
					localOffset,
					dirtyRange_b.size);
			if (bufferData) {
				const auto dataSize = clientBuffer_->dataSize();
				auto mapped = clientBuffer_->mapRange(BUFFER_GPU_READ, 0u, dataSize);

				const uint32_t offset = dirtyRange_b.offset - localOffset;
				memcpy(bufferData + offset,
					   mapped.r + dirtyRange_b.offset,
					   dirtyRange_b.size);

				for (uint32_t inputIdx = dirtyRange_s.startIdx; inputIdx < dirtyRange_s.endIdx; ++inputIdx) {
					setLastInputStamp(stagedInputs_[inputIdx]);
				}

				clientBuffer_->unmapRange(BUFFER_GPU_READ,
						0u, dataSize, mapped.r_index);
				shared_->stagingBuffer_->endMappedWrite(
						drawBufferRef_,
						*drawBufferRange_.get(),
						shared_->stagingOffset_);
			} // else: frame was dropped
		}
		stamp_ += 1;
	} else if (numDirtySegments_ > 0) {
		// full update: map the whole range between the first and last dirty segment.
		auto &firstSegment = dirtyBufferRanges_[0];
		auto &lastSegment = dirtyBufferRanges_[numDirtySegments_ - 1];
		const uint32_t mapRangeSize = lastSegment.offset - firstSegment.offset + lastSegment.size;
		const uint32_t localOffset = shared_->stagingOffset_ + firstSegment.offset;

		byte *bufferData = shared_->stagingBuffer_->beginMappedWrite(
				drawBufferRef_,
				// partial writing not ok, as we invalidate the whole range
				false,
				localOffset,
				mapRangeSize);
		if (bufferData) {
			copyFullData(bufferData, firstSegment.offset);
			shared_->stagingBuffer_->endMappedWrite(
					drawBufferRef_,
					*drawBufferRange_.get(),
					shared_->stagingOffset_);
			stamp_ += 1;
		} // else: frame was dropped
	}
}

void StagedBuffer::updatePersistentMapped() {
	// note: partial writing is ok with persistent mapping.
	static constexpr bool usePersistentPartialUpdate = true;

	if (numDirtySegments_ == 0) { return; }
	// copy first to last dirty segments to the mapped buffer.
	auto &firstSegment = dirtyBufferRanges_[0];
	auto &lastSegment = dirtyBufferRanges_[numDirtySegments_ - 1];
	auto &sb = shared_->stagingBuffer_;
	const uint32_t localOffset = shared_->stagingOffset_ + firstSegment.offset;

	byte *bufferData = sb->beginMappedWrite(
			drawBufferRef_,
			usePersistentPartialUpdate,
			localOffset,
			lastSegment.offset - firstSegment.offset + lastSegment.size);
	if (bufferData) {
		// only copy the dirty segments to the mapped buffer.
		copyDirtyData(
				bufferData,
				firstSegment.offset);
		// push the dirty segments to the flush queue for just-in-time flushing.
		if (stagingFlags_.useExplicitFlushing()) {
			auto dirtySegments = (BufferRange2ui *) (&dirtyBufferRanges_.data()[0].offset);
			sb->pushToFlushQueue(
					dirtySegments + shared_->stagingOffset_,
					numDirtySegments_);
		}
		sb->endMappedWrite(
				drawBufferRef_,
				*drawBufferRange_.get(),
				shared_->stagingOffset_);
		stamp_ += 1;
	} // else: frame was dropped
}

bool StagedBuffer::updateReadBuffer() {
	stamp_ += 1;
	return shared_->stagingBuffer_->readBuffer(
			drawBufferRef_,
			*drawBufferRange_.get(),
			shared_->stagingOffset_);
}

void StagedBuffer::createNextDirtySegment() {
	if (numDirtySegments_ >= dirtySegmentRanges_.size()) {
		// allocate a new segment if we have no more space
		dirtySegmentRanges_.resize(dirtySegmentRanges_.size() + 4);
		dirtyBufferRanges_.resize(dirtyBufferRanges_.size() + 4);
	}
	numDirtySegments_ += 1;
}

void StagedBuffer::Shared::setUpdatedFrame(bool isUpdated) {
	bool &wasUpdated = updatedFrames_[updateIdx_++];
	// count the number of frames that had an update over the last n frames.
	updateCount_ += (wasUpdated != isUpdated) * (isUpdated*2 - 1);
	wasUpdated = isUpdated;
	// wrap around the index
	updateIdx_ *= (updateIdx_ < updateRange_);
	hasUpdateRotated_ = hasUpdateRotated_ || (updateIdx_ >= updateRange_);
}

void StagedBuffer::resetUpdateHistory() {
	shared_->updateIdx_ = 0;
	shared_->hasUpdateRotated_ = false;
}
