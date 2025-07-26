#include "buffer-block.h"
#include "ubo.h"
#include "ssbo.h"
#include "regen/states/state.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

//#define BUFFER_BLOCK_DISABLE_GLOBAL_STAGING
//#define BUFFER_BLOCK_DISABLE_EXPLICIT_FLUSHING
//#define BUFFER_BLOCK_FORCE_IMPLICIT_STAGING
// If defined, do buffer-to-buffer copy within ring buffer in case we can fetch needed
// data from another segment without stalling.
//#define BLOCK_INPUT_USE_BUFFERED_DATA

uint32_t BufferBlock::MIN_SEGMENTS_PARTIAL_TEMPORARY = 6;
float BufferBlock::MAX_UPDATE_RATIO_PARTIAL_TEMPORARY = 0.33f;
// Default to 60 frames for update rate computation.
uint32_t BufferBlock::UPDATE_RATE_RANGE = 60;

BufferBlock::BufferBlock(
		const std::string &name,
		BufferTarget target,
		const BufferUpdateFlags &hints,
		Qualifier blockQualifier,
		BufferMemoryLayout memoryLayout)
		: BufferObject(target, hints),
		  ShaderInput(name, GL_INVALID_ENUM, 0, 0, 0, false),
		  blockQualifier_(blockQualifier),
		  memoryLayout_(memoryLayout),
		  stagingFlags_(target, hints) {
	enableInput_ = [this](GLint loc) { enableBufferBlock(loc); };
	isBufferBlock_ = true;
	isVertexAttribute_ = false;
	isVertexAttribute_ = false;
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
	setBufferAccessMode(BUFFER_GPU_ONLY);
	setBufferMapMode(BUFFER_MAP_DISABLED);
	// if the buffer will never be updated, we can use implicit staging.
	if (hints.frequency == BUFFER_UPDATE_NEVER) {
		setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
	}
#ifdef BUFFER_BLOCK_FORCE_IMPLICIT_STAGING
	setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
#endif
}

BufferBlock::BufferBlock(const BufferBlock &other)
		: BufferObject(other),
		  ShaderInput(other.name(), GL_INVALID_ENUM, 0, 0, 0, false),
		  blockQualifier_(other.blockQualifier_),
		  memoryLayout_(other.memoryLayout_),
		  bindingIndex_(other.bindingIndex_),
		  hasClientData_(other.hasClientData_),
		  isBlockValid_(other.isBlockValid_),
		  inputs_(other.inputs_),
		  drawBufferRef_(other.drawBufferRef_),
		  drawBufferRange_(other.drawBufferRange_),
		  requiredSize_(other.requiredSize_),
		  estimatedSize_(other.estimatedSize_),
		  updatedSize_(other.updatedSize_),
		  stamp_(other.stamp_),
		  blockInputs_(other.blockInputs_),
		  stagingFlags_(other.stagingFlags_),
		  userDefinedBufferingMode_(other.userDefinedBufferingMode_),
		  shared_(other.shared_) {
	enableInput_ = [this](GLint loc) { enableBufferBlock(loc); };
	isBufferBlock_ = true;
	isVertexAttribute_ = false;
	shared_->copyCount_.fetch_add(1, std::memory_order_relaxed);
}

static std::string getName(const BufferObject &other, const std::string &name) {
	if (name.empty()) {
		auto *block = dynamic_cast<const BufferBlock *>(&other);
		if (block != nullptr) {
			return block->name();
		}
		auto *tbo = dynamic_cast<const TBO *>(&other);
		if (tbo != nullptr && tbo->input().get()) {
			return REGEN_STRING("Buffer_" << tbo->input()->name());
		}
	}
	return name;
}

BufferBlock::BufferBlock(const BufferObject &other, const std::string &name)
		: BufferObject(other),
		  ShaderInput(getName(other,name), GL_INVALID_ENUM, 0, 0, 0, GL_FALSE),
		  blockQualifier_(BufferBlock::BUFFER),
		  memoryLayout_(BUFFER_MEMORY_STD430),
		  stagingFlags_(other.bufferTarget(), other.bufferUpdateHints()) {
	auto block = dynamic_cast<const BufferBlock *>(&other);
	if (block != nullptr) {
		blockQualifier_ = block->blockQualifier_;
		memoryLayout_ = block->memoryLayout_;
		bindingIndex_ = block->bindingIndex_;
		hasClientData_ = block->hasClientData_;
		isBlockValid_ = block->isBlockValid_;
		inputs_ = block->inputs_;
		drawBufferRef_ = block->drawBufferRef_;
		drawBufferRange_ = block->drawBufferRange_;
		requiredSize_ = block->requiredSize_;
		estimatedSize_ = block->estimatedSize_;
		updatedSize_ = block->updatedSize_;
		stamp_ = block->stamp_;
		blockInputs_ = block->blockInputs_;
		stagingFlags_ = block->stagingFlags_;
		userDefinedBufferingMode_ = block->userDefinedBufferingMode_;
		shared_ = block->shared_;
		shared_->copyCount_.fetch_add(1, std::memory_order_relaxed);
	} else {
		shared_ = ref_ptr<Shared>::alloc();
		shared_->updatedFrames_ = new bool[UPDATE_RATE_RANGE];
		shared_->updateRange_ = UPDATE_RATE_RANGE;
		shared_->f_updateRangeInv_ = 1.0f / static_cast<float>(UPDATE_RATE_RANGE);
		std::fill(
				shared_->updatedFrames_,
				shared_->updatedFrames_ + UPDATE_RATE_RANGE,
				false);
		drawBufferRange_ = ref_ptr<BufferRange>::alloc();

		auto tbo = dynamic_cast<const TBO *>(&other);
		if (tbo != nullptr) {
			inputs_.emplace_back(tbo->input(), tbo->input()->name());

			auto bufferInput = ref_ptr<BlockInput>::alloc();
			bufferInput->input = tbo->input();
			bufferInput->offset = 0;
			bufferInput->lastStamp[0] = tbo->input()->stamp();
			blockInputs_.emplace_back(bufferInput);

			if (!tbo->allocations().empty()) {
				drawBufferRef_ = tbo->allocations()[0];
			}
		} else {
			REGEN_WARN("BufferBlock: Unable to copy buffer object of unknown type.");
		}
	}
	enableInput_ = [this](GLint loc) { enableBufferBlock(loc); };
	isBufferBlock_ = true;
	isVertexAttribute_ = false;
	isVertexAttribute_ = false;
}

BufferBlock::~BufferBlock() {
	if (shared_->copyCount_.fetch_sub(1) == 1) {
		if (shared_->isGloballyStaged_) {
			StagingSystem::instance().removeBufferBlock(this);
		}
	}
}

void BufferBlock::enableBufferBlock(GLint loc) {
	if (!isBlockValid_) return;
	auto *rs = RenderState::get();

	prepareRebind(loc);
	if (!shared_->isGloballyStaged_) {
		update();
	}
	rs->bufferRange(glTarget_).apply(loc, *drawBufferRange_.get());
	// mark the point of accessing a mapped buffer segment for draw operation,
	// which is needed to avoid writing to the buffer while it is being read.
	// only used in implicit staging mode, as the fence point is set after the
	// staging-to-main copy in explicit staging mode.
	// note: this is used in global and local staging modes.
	shared_->stagingBuffer_->markDrawAccessed(*drawBufferRange_.get());
	bindingIndex_ = loc;
}

void BufferBlock::bind(GLint loc) {
	auto *rs = RenderState::get();
	prepareRebind(loc);
	rs->bufferRange(glTarget_).apply(loc, *drawBufferRange_.get());
	bindingIndex_ = loc;
}

void BufferBlock::prepareRebind(GLint loc) {
	if (bindingIndex_ != loc && bindingIndex_ != -1) {
		// seems the buffer switched to another index!
		// this is something the buffer manager should try to avoid, but there are some situations
		// where it might be difficult.
		// In case of doing the switch, we need to unbind the old binding index.
		auto *rs = RenderState::get();
		auto &actual = rs->bufferRange(glTarget_).value(bindingIndex_);
		if (actual.buffer_ == drawBufferRef_->bufferID() &&
			actual.offset_ == drawBufferRef_->address() &&
			actual.size_ == drawBufferRef_->allocatedSize()) {
			//REGEN_INFO("Rebinding buffer block " << name()
			//	<< " from binding index " << bindingIndex_ << " to " << loc);
			rs->bufferRange(glTarget_).apply(bindingIndex_, BufferRange::nullReference());
			bindingIndex_ = -1;
		}
	}
}

void BufferBlock::setBufferingMode(BufferingMode mode) {
	userDefinedBufferingMode_ = mode;
	stagingFlags_.bufferingMode = mode;
}

void BufferBlock::setStagingMapMode(BufferMapMode mode) {
	if (!flags_.useExplicitStaging()) {
		// if we are not using separate staging buffers, we need to set the same map mode
		// for the main buffer as well.
		setBufferMapMode(mode);
		stagingFlags_.mapMode = flags_.mapMode;
	} else {
		stagingFlags_.mapMode = mode;
	}
}

void BufferBlock::setStagingAccessMode(BufferAccessMode mode) {
	if (!flags_.useExplicitStaging()) {
		// if we are not using separate staging buffers, we need to set the same access mode
		// for the main buffer as well.
		setBufferAccessMode(mode);
		stagingFlags_.accessMode = flags_.accessMode;
	} else {
		stagingFlags_.accessMode = mode;
	}
}

void BufferBlock::enableWriteAccess() {
	if (stagingFlags_.accessMode == BUFFER_GPU_ONLY) {
		// if the access mode is GPU-only, we need to switch it to CPU_WRITE
		setStagingAccessMode(BUFFER_CPU_WRITE);
	} else if (stagingFlags_.accessMode == BUFFER_CPU_READ) {
		REGEN_WARN("Attempting to enable write access on a buffer that is CPU_READ only.");
	}
}

void BufferBlock::setStagingBuffering(BufferingMode mode) {
	if (!userDefinedBufferingMode_.has_value()) {
		stagingFlags_.bufferingMode = mode;
	}
}

void BufferBlock::updateStorageFlags() {
	if (!hasClientData_) return;
	if (flags_.updateHints.frequency == BUFFER_UPDATE_NEVER) return;

	// update the storage flags based on added inputs
	auto sizeClass = StagingBuffer::getBufferSizeClass(estimatedSize_);
	// input has client data, so we need to set the access mode such that the CPU can write to it.
	enableWriteAccess();
	setStagingBuffering(SINGLE_BUFFER);

	// NOTE: in local staging we avoid persistent mapping the buffer to CPU memory to avoid performance issues
	//       with fencing, as currently local staging uses per-BO and per-segment fences which is overkill
	//       for most cases.
	// TODO: support adaptive ring buffering for local staging as well.
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

void BufferBlock::addBlockInput(const ref_ptr<ShaderInput> &input, const std::string &name) {
	auto bufferInput = ref_ptr<BlockInput>::alloc();
	bufferInput->input = input;
	blockInputs_.emplace_back(bufferInput);
	inputs_.emplace_back(input, name);
	estimatedSize_ += input->elementSize();
	hasClientData_ = input->hasClientData() && hasClientData_;
	input->setMemoryLayout(memoryLayout_);

	updateStorageFlags();
}

void BufferBlock::removeBlockInput(std::string_view name) {
	for (auto it = blockInputs_.begin(); it != blockInputs_.end(); ++it) {
		auto &blockInput = *it;
		if (blockInput->input->name() == name) {
			// remove the input from the inputs_ vector
			for (auto inputIt = inputs_.begin(); inputIt != inputs_.end(); ++inputIt) {
				if (inputIt->name_ == name) {
					inputs_.erase(inputIt);
					break;
				}
			}
			// remove the block input
			blockInputs_.erase(it);
			// TODO
			//xxx_rm_client_data(input, name);
			return;
		}
	}
	REGEN_WARN("Unable to remove input '" << name << "'. Input not found.");
}

void BufferBlock::update(bool forceUpdate) {
	if (!isBlockValid_) return;
	updateBlockInputs();
	updateDrawBuffer();
	if (!shared_->isGloballyStaged_) {
		// note: don't mess with the staging buffer if it is managed by the staging system.
		// i.e. in case someone explicitly called update() on the buffer block.
		copyStagingData(forceUpdate);
	}
}

void BufferBlock::resetDirtySegments() {
	// note: we never clear the segments_ vector, we just reset the counters
	numDirtySegments_ = 0;
}

void BufferBlock::createNextDirtySegment() {
	if (numDirtySegments_ >= dirtySegmentRanges_.size()) {
		// allocate a new segment if we have no more space
		dirtySegmentRanges_.emplace_back();
		dirtyBufferRanges_.emplace_back();
	}
	numDirtySegments_ += 1;
}

void BufferBlock::setDirtyRange(uint32_t dirtyIdx, BlockInput &input, uint32_t inputIdx) {
	auto &dirty_s = dirtySegmentRanges_[dirtyIdx];
	auto &dirty_b = dirtyBufferRanges_[dirtyIdx];
	dirty_b.offset = input.offset;
	dirty_b.size = input.inputSize;
	dirty_s.startIdx = inputIdx;
	dirty_s.endIdx = inputIdx;
}

void BufferBlock::appendToDirtyRange(uint32_t dirtyIdx, BlockInput &input, uint32_t inputIdx) {
	auto &dirty_s = dirtySegmentRanges_[dirtyIdx];
	auto &dirty_b = dirtyBufferRanges_[dirtyIdx];
	dirty_b.size = input.offset - dirty_b.offset + input.inputSize;
	dirty_s.endIdx = inputIdx;
}

void BufferBlock::resetUpdateHistory() {
	shared_->updateIdx_ = 0;
	shared_->hasUpdateRotated_ = false;
}

void BufferBlock::Shared::setUpdatedFrame(bool isUpdated) {
	bool &wasUpdated = updatedFrames_[updateIdx_++];
	// count the number of frames that had an update over the last n frames.
	updateCount_ += (wasUpdated != isUpdated) * (isUpdated*2 - 1);
	wasUpdated = isUpdated;
	// wrap around the index
	updateIdx_ *= (updateIdx_ < updateRange_);
	hasUpdateRotated_ = hasUpdateRotated_ || (updateIdx_ >= updateRange_);
}

uint32_t &BufferBlock::lastInputStamp(BlockInput &blockInput) {
	if (shared_->stagingBuffer_.get()) {
		return blockInput.lastStamp[shared_->stagingBuffer_->nextWriteIndex()];
	} else {
		return blockInput.lastStamp[0];
	}
}

uint32_t BufferBlock::updateBlockInputs() {
	bool lastChanged = false; // whether the last input changed or not
	bool hasNewSize = (requiredSize_ == 0); // whether the size of the block has changed
	hasClientData_ = true;
	updatedSize_ = 0u; // total size of the inputs that have changed
	resetDirtySegments();

	for (int32_t inputIdx = 0; inputIdx < static_cast<int32_t>(blockInputs_.size()); ++inputIdx) {
		auto &blockInput = *blockInputs_[inputIdx].get();
		hasNewSize = hasNewSize || (blockInput.inputSize != blockInput.input->inputSize());
		hasClientData_ = hasClientData_ && blockInput.input->hasClientData();

		// construct contiguous segments of inputs that have changed
		// FIXME: this should be done AFTER size update! BlockInput stuff is used, but updated later
		if (blockInput.input->stamp() != lastInputStamp(blockInput)) {
			updatedSize_ += blockInput.input->inputSize();
			if (lastChanged) {
				// this input adds to the current segment
				appendToDirtyRange(numDirtySegments_ - 1, blockInput, inputIdx);
			} else {
				// this input starts a new segment
				createNextDirtySegment();
				setDirtyRange(numDirtySegments_ - 1, blockInput, inputIdx);
			}
			lastChanged = true;
		} else {
			lastChanged = false;
		}
	}
	// remember if we had a dirty segment in this frame for computing the update rate.
	// this is useful for detecting stalls in the staging system, for adaptive ring buffering.
	setUpdatedFrame(hasDirtySegments());

	if (hasNewSize) {
		requiredSize_ = 0;
		for (auto &blockInput: blockInputs_) {
			auto &in = blockInput->input;
			// Compute the alignment based on the type
			// Align the offset to the required alignment
			auto remainder = requiredSize_ % in->baseAlignment();
			if (remainder != 0) {
				requiredSize_ += in->baseAlignment() - remainder;
			}
			blockInput->offset = requiredSize_;
			if (in->numElements() > 1) {
				blockInput->inputSize = in->baseAlignment() * in->alignmentCount() * in->numElements();
			} else {
				blockInput->inputSize = in->baseSize() * in->numElements();
			}
			requiredSize_ += blockInput->inputSize;
		}
		// Round total size up to next multiple of 16 (vec4 alignment for std140)
		if (memoryLayout_ == BUFFER_MEMORY_STD140) {
			static constexpr size_t std140Alignment = 16;
			size_t remainder = requiredSize_ % std140Alignment;
			if (remainder != 0) {
				requiredSize_ += std140Alignment - remainder;
				REGEN_DEBUG("RE-ALIGN for 16 bytes needed for block with size: " << requiredSize_ << " bytes"
																				 << " first input: "
																				 << blockInputs_[0]->input->name());
			}
		}

		// TODO: pdate the client buffer
		/**
		xxx_set_base_alignment;
		if (clientBuffer_.dataSize() != 0u) {
			uint32_t numAddedSegments = clientBuffer_->numSegments();
			for (uint32_t blockIdx=0; blockIdx < blockInputs_.size(); ++blockIdx) {
				auto &blockInput = *blockInputs_[blockIdx].get();
				if (blockIdx >= numAddedSegments) {
					// add the segment to the client buffer
					clientBuffer_.addSegment(
						blockInput.input->clientBuffer(),
						blockInput.offset,
						blockInput.inputSize);
				} else {
					// update the existing segment
					clientBuffer_.updateSegment(
						blockIdx,
						blockInput.offset,
						blockInput.inputSize);
				}

			}
		}
		**/
	}

	return requiredSize_;
}

/**
void BufferBlock::updateClientBuffer() {
	if(blockInputs_.empty()) { return; }
	if(clientBuffer_.dataSize()==requiredSize_) { return; }

	// BufferBlock uses client buffer for contiguous data storage.
	// Make sure the client buffer has requiredSize_ bytes allocated,
	// and map the ptrs to the shader inputs.
	if (clientBuffer_.dataSize()==0u) {
		// the first time updateClientBuffer has been called with block inputs added.
		// we need to allocate the client buffer with the required size.
		clientBuffer_.resize(requiredSize_, requiredSize_);
		for (auto &blockInput: blockInputs_) {
			clientBuffer_.addSegment(blockInput->input->clientBuffer());
		}
	} else {
		clientBuffer_.flush();
	}
}
**/

void BufferBlock::updateStridedData(BlockInput &bufferInput) {
	// Some attributes cannot be stored tightly packed in the buffer,
	// especially vec3 arrays or mat3 arrays cannot be stored tightly packed
	// in STD140 or STD430 layouts, so we need to align them to 16 bytes.
	// Which means there is a stride between array elements, which unfortunately
	// means that we need to copy element-by-element to the buffer instead of
	// copying the whole array at once using memcpy.
	auto &in = bufferInput.input;
	auto numElements = in->numArrayElements() * in->numInstances();
	if (numElements == 1) {
		return;
	}
	auto elementSizeUnaligned = in->valsPerElement() * in->dataTypeBytes();
	if (memoryLayout_ == BUFFER_MEMORY_STD140) {
		// the GL specification states that the stride between array elements must be
		// rounded up to 16 bytes for STD140.
		if (elementSizeUnaligned % 16 == 0) {
			return;
		}
	} else if (memoryLayout_ == BUFFER_MEMORY_STD430) {
		// only vec3 and mat3 types need to be aligned to 16 bytes with STD430.
		if (elementSizeUnaligned != 12 && elementSizeUnaligned != 48) {
			return;
		}
	} else {
		return;
	}
	REGEN_WARN("RE-ALIGN needed for input " << in->name() <<
											 " with " << numElements << " elements, unaligned size: "
											 << elementSizeUnaligned);
	auto elementSizeAligned = elementSizeUnaligned + (16 - elementSizeUnaligned % 16);
	auto dataSizeAligned = elementSizeAligned * numElements;
	if (dataSizeAligned != bufferInput.alignedSize) {
		delete[] bufferInput.alignedData;
		bufferInput.alignedSize = dataSizeAligned;
		bufferInput.alignedData = new byte[bufferInput.alignedSize];
	}
	auto clientData = in->mapClientDataRaw(ClientMappingMode::READ);
	auto *src = clientData.r;
	auto *dst = bufferInput.alignedData;
	for (unsigned int i = 0; i < numElements; ++i) {
		memcpy(dst, src, elementSizeUnaligned);
		src += elementSizeUnaligned;
		dst += elementSizeAligned;
	}
}

int32_t BufferBlock::getBufferedIndex(uint32_t stamp, const std::vector<uint32_t> &bufferedStamps) const {
	static constexpr int32_t NO_BUFFERED_INDEX = -1;
	for (int32_t i = 0; i < static_cast<int32_t>(bufferedStamps.size()); ++i) {
		if (bufferedStamps[i] == stamp && shared_->stagingBuffer_->isFenceSignaled(i)) {
			return i; // found the buffered index
		}
	}
	return NO_BUFFERED_INDEX; // not found
}

void BufferBlock::copyBlockInput(
		BlockInput &bufferInput,
		byte *mappedBufferData,
		uint32_t localMapOffset) {
	auto &currentStamp = lastInputStamp(bufferInput);
	currentStamp = bufferInput.input->stamp();

#ifdef BLOCK_INPUT_USE_BUFFERED_DATA
	auto &sb = shared_->stagingBuffer_;
	if (sb.get()) {
		auto ref = shared_->stagingBuffer_->stagingRef();
		if (!ref.get()) { ref = drawBufferRef_; }

		const int32_t bufferedIdx = getBufferedIndex(
				bufferInput.input->stamp(), bufferInput.lastStamp);
		if (bufferedIdx >= 0) {
			uint32_t readOffset  = sb->segmentOffset(bufferedIdx);
			uint32_t writeOffset = sb->segmentOffset(sb->nextWriteIndex());
			glCopyNamedBufferSubData(
				ref->bufferID(),
				ref->bufferID(),
				ref->address() + readOffset + bufferInput.offset,
				ref->address() + writeOffset + bufferInput.offset,
				bufferInput.input->inputSize());
			return;
		}
	}
#endif

	// NOTE: The buffer maybe is not mapped from the start if the adopted buffer range, e.g.
	//       in case starts at first dirt segment. However, the block input offsets are always
	//       relative to the start of the buffer, so we need to adjust the offset accordingly...
	const uint32_t offset = bufferInput.offset - localMapOffset;
	//updateStridedData(bufferInput);
	//if (bufferInput.alignedData) {
	//	memcpy(mappedBufferData + offset,
	//		   bufferInput.alignedData, bufferInput.alignedSize);
	//} else {
		auto mapped = bufferInput.input->mapClientDataRaw(ClientMappingMode::READ);
		memcpy(mappedBufferData + offset,
			   mapped.r,
			   bufferInput.input->inputSize());
	//}
}

void BufferBlock::copyDirtyData(byte *mappedBufferData, uint32_t localMapOffset) {
	// iterate over the changed segments and copy only those
	for (uint32_t segmentIdx = 0; segmentIdx < numDirtySegments_; ++segmentIdx) {
		auto &segment = dirtySegmentRanges_[segmentIdx];

		for (uint32_t inputIdx = segment.startIdx; inputIdx <= segment.endIdx; ++inputIdx) {
			auto &bufferInput = *blockInputs_[inputIdx].get();
			copyBlockInput(bufferInput, mappedBufferData, localMapOffset);
		}
	}
}

void BufferBlock::copyFullData(byte *mappedBufferData, uint32_t localMapOffset) {
	// full write of mapped range
	// get start and end indices from first and last segment
	uint32_t startIdx = dirtySegmentRanges_[0].startIdx;
	uint32_t endIdx = dirtySegmentRanges_[numDirtySegments_ - 1].endIdx;

	for (uint32_t inputIdx = startIdx; inputIdx <= endIdx; ++inputIdx) {
		auto &bufferInput = *blockInputs_[inputIdx].get();
		copyBlockInput(bufferInput, mappedBufferData, localMapOffset);
	}
}

void BufferBlock::markBufferDirty() {
	numDirtySegments_ = 0;
	createNextDirtySegment();
	dirtyBufferRanges_[0].offset = 0;
	dirtyBufferRanges_[0].size = requiredSize_;
	dirtySegmentRanges_[0].startIdx = 0;
	dirtySegmentRanges_[0].endIdx = static_cast<uint32_t>(blockInputs_.size() - 1);
}

void BufferBlock::resetDataStamps() {
	// reset the last stamps for all inputs and segments.
	for (auto &input: blockInputs_) {
		std::memset(input->lastStamp.data(), 0, input->lastStamp.size() * sizeof(uint32_t));
	}
}

void BufferBlock::queueStagingUpdate() {
	if (!shared_->isGloballyStaged_) {
		// reset the local staging buffer, causing it to be reinitialized
		shared_->stagingBuffer_ = {};
	}
	// on resize, create one dirty segment that covers the whole buffer.
	// also reset the last stamps for all inputs and segments.
	markBufferDirty();
	resetDataStamps();
}

void BufferBlock::updateDrawBuffer() {
	if (allocatedSize_ == requiredSize_) {
		// nothing to do, the draw buffer is already up-to-date
		return;
	}
	// enforce rebinding
	bindingIndex_ = -1;
	if (drawBufferRef_.get()) {
		orphanBufferRange(drawBufferRef_.get());
	}

	// if neither read nor write access is requested, we can use implicit staging.
	if (!stagingFlags_.isWritable() && !stagingFlags_.isReadable()) {
		setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
	}

	if (flags_.useExplicitStaging()) {
		drawBufferRef_ = adoptBufferRange(requiredSize_);
		// TODO: in case of explicit staging with multi buffering it might be best
		//    to copy initial data to the draw buffer right away to avoid some frames delay
		//    until the data is copied.
		//    - will be trivial once we have contiguous client buffer here!
	} else {
		// note: in case of implicit staging with multi-buffering, we need to allocate space for each segment.
		if (flags_.bufferingMode == RING_BUFFER) {
			REGEN_WARN("Ring buffer is not supported with implicit staging. "
					   "Switching to TRIPLE_BUFFER.");
			flags_.bufferingMode = TRIPLE_BUFFER;
		}
		drawBufferRef_ = adoptBufferRange(requiredSize_ * (int) flags_.bufferingMode);
	}

	// validate the allocation
	if (!drawBufferRef_.get()) {
		REGEN_ERROR("failed to allocate buffer for buffer flags " << flags_);
		isBlockValid_ = false;
		return;
	}
	if (isMapModePersistent(flags_.mapMode) && !drawBufferRef_->mappedData()) {
		REGEN_WARN("something went wrong with persistent mapping for buffer flags " << flags_);
		isBlockValid_ = false;
		return;
	}
	isBlockValid_ = true;

	allocatedSize_ = requiredSize_;
	// set draw buffer range to first segment in the ring buffer
	drawBufferRange_->buffer_ = drawBufferRef_->bufferID();
	drawBufferRange_->size_ = requiredSize_;
	drawBufferRange_->offset_ = drawBufferRef_->address();
	queueStagingUpdate();

	REGEN_INFO("Created "
					   << StagingBuffer::getBufferSizeClass(requiredSize_)
					   << " " << stagingFlags_.target
					   << " \"" << name() << "\" with"
					   << " " << requiredSize_ / 1024.0 << " Kib"
					   << " BO: " << drawBufferRef_->bufferID()
					   << " at: " << drawBufferRef_->address());
}

void BufferBlock::setStagingOffset(uint32_t offset) {
	shared_->stagingOffset_ = offset;
	queueStagingUpdate();
}

void BufferBlock::resetStagingBuffer(bool removeFromStagingSystem) {
	if (removeFromStagingSystem && shared_->isGloballyStaged_) {
		StagingSystem::instance().removeBufferBlock(this);
	}
	shared_->stagingBuffer_ = {};
	shared_->isGloballyStaged_ = false;
}

void BufferBlock::copyStagingData(bool forceUpdate) {
	if (forceUpdate) { markBufferDirty(); }
	bool needsUpdate = (numDirtySegments_ > 0);
	// advance update history
	setUpdatedFrame(needsUpdate);
	if (!needsUpdate) { return; }

	// lazy initialization of the staging buffer
	if (shared_->stagingBuffer_.get() == nullptr) {
#ifdef BUFFER_BLOCK_DISABLE_GLOBAL_STAGING
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
		for (auto &input: blockInputs_) {
			input->lastStamp.resize(numStagingSegments);
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
			isBlockValid_ = false;
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
		isBlockValid_ = false;
	}
}

void BufferBlock::updateNonMapped() {
	// iterate over the changed segments and copy only those into the staging buffer.
	shared_->stagingBuffer_->beginNonMappedWrite();

	for (uint32_t segmentIdx = 0; segmentIdx < numDirtySegments_; ++segmentIdx) {
		auto &dirtyRange_s = dirtySegmentRanges_[segmentIdx];

		for (uint32_t inputIdx = dirtyRange_s.startIdx; inputIdx <= dirtyRange_s.endIdx; ++inputIdx) {
			auto &bufferInput = *blockInputs_[inputIdx].get();
			const uint32_t localOffset = shared_->stagingOffset_ + bufferInput.offset;
			//updateStridedData(bufferInput);
			//if (bufferInput.alignedData) {
			//	shared_->stagingBuffer_->setSubData(
			//			drawBufferRef_,
			//			localOffset,
			//			bufferInput.alignedSize,
			//			bufferInput.alignedData);
			//} else {
				auto mapped = bufferInput.input->mapClientDataRaw(ClientMappingMode::READ);
				shared_->stagingBuffer_->setSubData(
						drawBufferRef_,
						localOffset,
						bufferInput.inputSize,
						mapped.r);
			//}
			lastInputStamp(bufferInput) = bufferInput.input->stamp();
		}
	}
	shared_->stagingBuffer_->endNonMappedWrite(
			drawBufferRef_,
			*drawBufferRange_.get(),
			shared_->stagingOffset_);
	stamp_ += 1;
}

void BufferBlock::updateTemporaryMapped() {
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
		for (uint32_t segmentIdx = 0; segmentIdx < numDirtySegments_; ++segmentIdx) {
			auto &dirtyRange_s = dirtySegmentRanges_[segmentIdx];
			auto &dirtyRange_b = dirtyBufferRanges_[segmentIdx];
			const uint32_t localOffset = shared_->stagingOffset_ + dirtyRange_b.offset;
			byte *bufferData = shared_->stagingBuffer_->beginMappedWrite(
					drawBufferRef_,
					false,
					localOffset,
					dirtyRange_b.size);
			if (bufferData) {
				for (uint32_t inputIdx = dirtyRange_s.startIdx; inputIdx <= dirtyRange_s.endIdx; ++inputIdx) {
					auto &bufferInput = *blockInputs_[inputIdx].get();
					copyBlockInput(bufferInput, bufferData, dirtyRange_b.offset);
				}
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

void BufferBlock::updatePersistentMapped() {
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

bool BufferBlock::updateReadBuffer() {
	stamp_ += 1;
	return shared_->stagingBuffer_->readBuffer(
			drawBufferRef_,
			*drawBufferRange_.get(),
			shared_->stagingOffset_);
}

ref_ptr<BufferBlock> BufferBlock::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto blockType = input.getValue<std::string>("type", "ubo");

	BufferUpdateFlags updateFlags;
	updateFlags.frequency = input.getValue<BufferUpdateFrequency>("update-frequency", BUFFER_UPDATE_NEVER);
	updateFlags.scope = input.getValue<BufferUpdateScope>("update-scope", BUFFER_UPDATE_FULLY);

	ref_ptr<BufferBlock> block;
	if (blockType == "ubo") {
		block = ref_ptr<UBO>::alloc(input.getName(), updateFlags);
	} else if (blockType == "ssbo") {
		block = ref_ptr<SSBO>::alloc(input.getName(), updateFlags);
	} else {
		REGEN_WARN("Unknown buffer block type '" << blockType << "'. Using UBO.");
		block = ref_ptr<UBO>::alloc(input.getName(), updateFlags);
	}
	if (input.hasAttribute("access-mode")) {
		block->setBufferAccessMode(
				input.getValue<BufferAccessMode>("access-mode", BUFFER_CPU_WRITE));
	}
	if (input.hasAttribute("map-mode")) {
		block->setBufferMapMode(
				input.getValue<BufferMapMode>("map-mode", BUFFER_MAP_DISABLED));
	}
	auto dummyState = ref_ptr<State>::alloc();

	for (auto &n: input.getChildren()) {
		if (n->getCategory() == "uniform" || n->getCategory() == "input") {
			auto uniform = scene::ShaderInputProcessor::createShaderInput(
					ctx.scene(), *n.get(), dummyState);
			if (uniform->isVertexAttribute()) {
				REGEN_WARN("UBO cannot contain vertex attributes. In node '" << n->getDescription() << "'.");
				continue;
			}
			auto name = n->getValue("name");
			block->addBlockInput(uniform, name);
		} else {
			REGEN_WARN("Unknown UBO child category '" << n->getCategory() << "'.");
		}
	}
	GL_ERROR_LOG();

	return block;
}

std::ostream &regen::operator<<(std::ostream &out, const BufferBlock::Qualifier &v) {
	switch (v) {
		case BufferBlock::Qualifier::UNIFORM:
			out << "uniform";
			break;
		case BufferBlock::Qualifier::BUFFER:
			out << "buffer";
			break;
		case BufferBlock::Qualifier::IN:
			out << "in";
			break;
		case BufferBlock::Qualifier::OUT:
			out << "out";
			break;
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BufferBlock::Qualifier &v) {
	std::string val;
	in >> val;
	boost::to_lower(val);
	if (val == "uniform") v = BufferBlock::Qualifier::UNIFORM;
	else if (val == "buffer") v = BufferBlock::Qualifier::BUFFER;
	else if (val == "in") v = BufferBlock::Qualifier::IN;
	else if (val == "out") v = BufferBlock::Qualifier::OUT;
	else {
		REGEN_WARN("Unknown storage qualifier '" << val << "'. Using UNIFORM.");
		v = BufferBlock::Qualifier::UNIFORM;
	}
	return in;
}
