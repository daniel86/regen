#include "buffer-block.h"
#include "ubo.h"
#include "ssbo.h"
#include "regen/states/state.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

//#define REGEN_BUFFER_BLOCK_DEBUG
#define BUFFER_BLOCK_DISABLE_GLOBAL_STAGING
//#define BUFFER_BLOCK_DISABLE_RING_BUFFER
//#define BUFFER_BLOCK_DISABLE_PERSISTENT
//#define BUFFER_BLOCK_DISABLE_EXPLICIT_FLUSHING
//#define BUFFER_BLOCK_FORCE_IMPLICIT_STAGING

uint32_t BufferBlock::MIN_SEGMENTS_PARTIAL_TEMPORARY = 6;
float BufferBlock::MAX_UPDATE_RATIO_PARTIAL_TEMPORARY = 0.33f;
// Default to 60 frames for update rate computation.
uint32_t BufferBlock::UPDATE_RATE_RANGE = 60;

BufferBlock::BufferBlock(
		BufferTarget target,
		const BufferUpdateFlags &hints,
		Qualifier blockQualifier,
		BufferMemoryLayout memoryLayout)
		: BufferObject(target, hints),
		  blockQualifier_(blockQualifier),
		  memoryLayout_(memoryLayout),
		  stagingFlags_(target, hints) {
	shared_ = ref_ptr<Shared>::alloc();
	shared_->updatedFrames_ = new bool[UPDATE_RATE_RANGE];
	drawBufferRange_ = ref_ptr<BufferRange>::alloc();
	// initially assume it is a GPU-only buffer.
	// the flag will be switched to something else based on the inputs added.
	setBufferAccessMode(BUFFER_GPU_ONLY);
	setBufferMapMode(BUFFER_MAP_DISABLED);
	// if the buffer will never be updated, we can use implicit staging.
	if (hints.frequency == BUFFER_UPDATE_NEVER) {
		setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
	}
#ifdef BUFFER_BLOCK_DISABLE_RING_BUFFER
	setBufferingMode(SINGLE_BUFFER);
#endif
#ifdef BUFFER_BLOCK_FORCE_IMPLICIT_STAGING
	setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
#endif
}

BufferBlock::BufferBlock(const BufferBlock &other)
		: BufferObject(other),
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
}

BufferBlock::BufferBlock(const BufferObject &other)
		: BufferObject(other),
		  blockQualifier_(BufferBlock::BUFFER),
		  memoryLayout_(BUFFER_MEMORY_STD430),
		  stagingFlags_(other.bufferTarget(), other.bufferUpdateHints()) {
	auto block = dynamic_cast<const BufferBlock *>(&other);
	if (block != nullptr) {
		blockQualifier_ = block->blockQualifier_;
		memoryLayout_ = block->memoryLayout_;
		shared_ = block->shared_;
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
	} else {
		shared_ = ref_ptr<Shared>::alloc();
		shared_->updatedFrames_ = new bool[UPDATE_RATE_RANGE];
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
}

BufferBlock::~BufferBlock() = default;

void BufferBlock::enableBufferBlock(GLint loc) {
	if (!isBlockValid_) return;
	auto *rs = RenderState::get();

	prepareRebind(loc);
	if (!shared_->isGloballyStaged_ && shared_->useAutoUpdate_) {
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
			//REGEN_INFO("Rebinding buffer block " << getBlockName()
			//	<< " from binding index " << bindingIndex_ << " to " << loc);
			rs->bufferRange(glTarget_).apply(bindingIndex_, BufferRange::nullReference());
			bindingIndex_ = -1;
		}
	}
}

std::string BufferBlock::getBlockName() const {
	auto *si = dynamic_cast<const ShaderInput *>(this);
	if (si != nullptr) {
		return si->name();
	} else if (!blockInputs_.empty()) {
		return REGEN_STRING("{" << blockInputs_[0]->input->name() << "}");
	} else {
		return "BufferBlock";
	}
}

void BufferBlock::setBufferingMode(BufferingMode mode) {
#ifndef BUFFER_BLOCK_DISABLE_RING_BUFFER
	userDefinedBufferingMode_ = mode;
	stagingFlags_.bufferingMode = mode;
#endif
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
		// if the access mode is CPU_READ, we need to switch it to CPU_READ_WRITE
		setStagingAccessMode(BUFFER_CPU_READ_WRITE);
	}
}

void BufferBlock::setStagingBuffering(BufferingMode mode) {
#ifndef BUFFER_BLOCK_DISABLE_RING_BUFFER
	if (!userDefinedBufferingMode_.has_value()) {
		stagingFlags_.bufferingMode = mode;
	}
#endif
}

void BufferBlock::enablePersistentMapping(bool useFlushExplicit) {
	if (useFlushExplicit) {
		setStagingMapMode(BUFFER_MAP_PERSISTENT_FLUSH);
	} else {
		setStagingMapMode(BUFFER_MAP_PERSISTENT_COHERENT);
	}
}

void BufferBlock::enablePersistentMapping_(bool useFlushExplicit) {
#ifdef BUFFER_BLOCK_DISABLE_PERSISTENT
	// use temporary mapping.
	setStagingMapMode(BUFFER_MAP_TEMPORARY);
#else
	// use coherent persistent mapping.
	enablePersistentMapping(useFlushExplicit);
#endif
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
			// Use ring-buffer in staging with persistent mapping for frequent updates.
			// In addition, use explicit flushing in case of partial updates.
			setStagingBuffering(DOUBLE_BUFFER);
			enablePersistentMapping_(stagingFlags_.areUpdatesPartial());
		} else if (stagingFlags_.areUpdatesVeryFrequent()) {
			// The current local fencing would not work well with very frequent updates!
			// So better use temporary mapping in this case.
			setStagingMapMode(BUFFER_MAP_TEMPORARY);
			setStagingBuffering(DOUBLE_BUFFER);
		} else {
			// Use single-buffering in staging with unmapped copy
			// or temporary mapping for infrequent updates.
			// TODO: could move to implicit staging here
			if (stagingFlags_.areUpdatesPartial()) {
				setStagingMapMode(BUFFER_MAP_DISABLED);
			} else {
				setStagingMapMode(BUFFER_MAP_TEMPORARY);
			}
		}
	} else if (sizeClass == BUFFER_SIZE_LARGE) {
		// If the buffer is large (e.g. < 1MB)
		if (stagingFlags_.areUpdatesFrequent()) {
			// If updates are frequent, then use ring buffer with range invalidation.
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
			return;
		}
	}
	REGEN_WARN("BufferBlock: Unable to remove input '" << name << "'. Input not found.");
}

void BufferBlock::update(bool forceUpdate) {
	if (!isBlockValid_) return;

#ifdef REGEN_BUFFER_BLOCK_DEBUG
	auto t0 = std::chrono::high_resolution_clock::now();
#endif
	updateBlockInputs();
#ifdef REGEN_BUFFER_BLOCK_DEBUG
	auto t1 = std::chrono::high_resolution_clock::now();
#endif
	updateDrawBuffer();
#ifdef REGEN_BUFFER_BLOCK_DEBUG
	auto t2 = std::chrono::high_resolution_clock::now();
#endif
	if (!shared_->isGloballyStaged_) {
		// note: don't mess with the staging buffer if it is managed by the staging system.
		// i.e. in case someone explicitly called update() on the buffer block.
		copyStagingData(forceUpdate);
	}
#ifdef REGEN_BUFFER_BLOCK_DEBUG
	auto t3 = std::chrono::high_resolution_clock::now();
#endif

#ifdef REGEN_BUFFER_BLOCK_DEBUG
	static std::vector<long> resizeTimes;
	static std::vector<long> copyTimes;
	static std::vector<long> totalTimes;
	auto resizeTime = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	auto copyTime = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
	auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t0).count();
	resizeTimes.push_back(resizeTime);
	copyTimes.push_back(copyTime);
	totalTimes.push_back(totalTime);
	if (copyTimes.size() > 1000) {
		// print the average time for the last 100 frames
		long resizeAvg = 0;
		long copyAvg = 0;
		long totalAvg = 0;
		for (size_t i = 0; i < copyTimes.size(); ++i) {
			resizeAvg += resizeTimes[i];
			copyAvg += copyTimes[i];
			totalAvg += totalTimes[i];
		}
		resizeAvg /= static_cast<long>(copyTimes.size());
		copyAvg /= static_cast<long>(copyTimes.size());
		totalAvg /= static_cast<long>(copyTimes.size());
		REGEN_INFO("resize=" << std::fixed << std::setprecision(4)
				<< static_cast<float>(resizeAvg) / 1000.0f << "ms " <<
				"copy=" << std::fixed << std::setprecision(4)
				<< static_cast<float>(copyAvg) / 1000.0f << "ms " <<
				"total=" << std::fixed << std::setprecision(4)
				<< static_cast<float>(totalAvg) / 1000.0f << "ms ");
		resizeTimes.clear();
		copyTimes.clear();
		totalTimes.clear();
	}
#endif
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

float BufferBlock::getUpdateRate() const {
	if (shared_->hasUpdateRotated_) {
		return static_cast<float>(shared_->updateCount_) / static_cast<float>(shared_->updateRange_);
	} else {
		return -1.0f; // not enough frames to compute the update rate
	}
}

void BufferBlock::resetUpdateHistory() {
	shared_->updateCount_ = 0;
	shared_->updateIdx_ = 0;
	shared_->hasUpdateRotated_ = false;
	std::fill(
			shared_->updatedFrames_,
			shared_->updatedFrames_ + shared_->updateRange_,
			false);
}

void BufferBlock::setUpdatedFrame(bool isUpdated) {
	bool wasUpdated = shared_->updatedFrames_[shared_->updateIdx_];
	if (!wasUpdated && isUpdated) {
		shared_->updateCount_++;
	} else if (wasUpdated && !isUpdated) {
		shared_->updateCount_--;
	}
	shared_->updatedFrames_[shared_->updateIdx_++] = isUpdated;
	if (shared_->updateIdx_ >= shared_->updateRange_) {
		shared_->updateIdx_ = 0; // wrap around the index
		shared_->hasUpdateRotated_ = true; // we have rotated the update history
	}
}

uint32_t &BufferBlock::lastInputStamp(BlockInput &blockInput) {
	return shared_->stagingBuffer_.get() ?
		   blockInput.lastStamp[shared_->stagingBuffer_->nextWriteIndex()] :
		   blockInput.lastStamp[0];
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

	if (hasNewSize) {
		requiredSize_ = 0;
		for (auto &blockInput: blockInputs_) {
			auto &in = blockInput->input;
			// note we need to compute the "aligned" offset for std140 layout
			auto baseSize = in->dataTypeBytes() * in->valsPerElement();
			// Compute the alignment based on the type
			auto baseAlignment = baseSize;
			auto alignmentCount = 1u;
			if (baseSize == 12u) { // vec3
				baseAlignment = 16u;
			} else if (baseSize == 48u) { // mat3
				baseAlignment = 16;
				alignmentCount = 3;
			} else if (baseSize == 64u) { // mat4
				baseAlignment = 16;
				alignmentCount = 4;
			} else if (in->numElements() > 1 && memoryLayout_ == BUFFER_MEMORY_STD140) {
				// with STD140, each array element must be padded to a multiple of 16 bytes
				baseAlignment = 16u;
			}
			// Align the offset to the required alignment
			auto remainder = requiredSize_ % baseAlignment;
			if (remainder != 0) {
				requiredSize_ += baseAlignment - remainder;
			}
			blockInput->offset = requiredSize_;
			if (in->numElements() > 1) {
				blockInput->inputSize = baseAlignment * alignmentCount * in->numElements();
			} else {
				blockInput->inputSize = baseSize * in->numElements();
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
	}

	return requiredSize_;
}

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
	REGEN_DEBUG("RE-ALIGN needed for input " << in->name() <<
											 " with " << numElements << " elements, unaligned size: "
											 << elementSizeUnaligned);
	auto elementSizeAligned = elementSizeUnaligned + (16 - elementSizeUnaligned % 16);
	auto dataSizeAligned = elementSizeAligned * numElements;
	if (dataSizeAligned != bufferInput.alignedSize) {
		delete[] bufferInput.alignedData;
		bufferInput.alignedSize = dataSizeAligned;
		bufferInput.alignedData = new byte[bufferInput.alignedSize];
	}
	auto clientData = in->mapClientDataRaw(ShaderData::READ);
	auto *src = clientData.r;
	auto *dst = bufferInput.alignedData;
	for (unsigned int i = 0; i < numElements; ++i) {
		memcpy(dst, src, elementSizeUnaligned);
		src += elementSizeUnaligned;
		dst += elementSizeAligned;
	}
}

void BufferBlock::copyBufferData1(byte *mappedBufferData, uint32_t localMapOffset, BlockInput &bufferInput) {
	// NOTE: The buffer maybe is not mapped from the start if the adopted buffer range, e.g.
	//       in case starts at first dirt segment. However, the block input offsets are always
	//       relative to the start of the buffer, so we need to adjust the offset accordingly...
	// TODO: in case of multibuffering, it could be that previous staging segment has the newest data,
	//       in which case we can do buffer-to-buffer copy in staging instead of copying the CPU data.
	const uint32_t offset = bufferInput.offset - localMapOffset;
	updateStridedData(bufferInput);
	if (bufferInput.alignedData) {
		memcpy(mappedBufferData + offset,
			   bufferInput.alignedData, bufferInput.alignedSize);
	} else {
		auto mapped = bufferInput.input->mapClientDataRaw(ShaderData::READ);
		memcpy(mappedBufferData + offset,
			   mapped.r,
			   bufferInput.input->inputSize());
	}
}

void BufferBlock::copyBufferData(byte *mappedBufferData, uint32_t localMapOffset, bool partialWrite) {
	if (partialWrite) {
		// iterate over the changed segments and copy only those
		for (uint32_t segmentIdx = 0; segmentIdx < numDirtySegments_; ++segmentIdx) {
			auto &segment = dirtySegmentRanges_[segmentIdx];

			for (uint32_t inputIdx = segment.startIdx; inputIdx <= segment.endIdx; ++inputIdx) {
				auto &bufferInput = *blockInputs_[inputIdx].get();
				copyBufferData1(mappedBufferData, localMapOffset, bufferInput);
				lastInputStamp(bufferInput) = bufferInput.input->stamp();
			}
		}
	} else { // full write of mapped range
		// get start and end indices from first and last segment
		// note: in case of persistent mapping, we always must write the whole buffer range,
		//       as the whole range is mapped.
		bool isPersistent = isMapModePersistent(stagingFlags_.mapMode);
		uint32_t startIdx = (isPersistent ? 0u : dirtySegmentRanges_[0].startIdx);
		uint32_t endIdx = (isPersistent ? (blockInputs_.size() - 1) :
						   dirtySegmentRanges_[numDirtySegments_ - 1].endIdx);

		for (uint32_t inputIdx = startIdx; inputIdx <= endIdx; ++inputIdx) {
			auto &bufferInput = *blockInputs_[inputIdx].get();
			copyBufferData1(mappedBufferData, localMapOffset, bufferInput);
			lastInputStamp(bufferInput) = bufferInput.input->stamp();
		}
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

void BufferBlock::updateDrawBuffer() {
	if (allocatedSize_ == requiredSize_) {
		// nothing to do, the draw buffer is already up-to-date}
		return;
	}
	// enforce rebinding
	bindingIndex_ = -1;
	if (drawBufferRef_.get()) {
		free(drawBufferRef_.get());
	}

	// if neither read nor write access is requested, we can use implicit staging.
	if (!stagingFlags_.isWritable() && !stagingFlags_.isReadable()) {
		setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
	}

	if (flags_.useExplicitStaging()) {
		drawBufferRef_ = adoptBufferRange(requiredSize_);
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

	if (!shared_->isGloballyStaged_) {
		// reset the local staging buffer, causing it to be reinitialized
		shared_->stagingBuffer_ = {};
	}

	// on resize, create one dirty segment that covers the whole buffer.
	// also reset the last stamps for all inputs and segments.
	markBufferDirty();
	for (auto &input: blockInputs_) {
		std::memset(input->lastStamp.data(), 0, input->lastStamp.size() * sizeof(uint32_t));
	}

	REGEN_INFO("Created "
					   << std::setw(6) << std::setfill(' ')
					   << StagingBuffer::getBufferSizeClass(requiredSize_) << " "
					   << stagingFlags_
					   << " \"" << getBlockName() << "\" with "
					   << " size: " << requiredSize_ / 1024.0 << " Kib"
					   << " staging-offset: " << shared_->stagingOffset_
					   << " segments: " << shared_->numBufferSegments_
					   << " global: " << shared_->isGloballyStaged_
					   << " auto-update: " << shared_->useAutoUpdate_
	);
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
		// FIXME: This is not really safe with the copy constr. etc.
		//         - maybe best to hand out a ref_ptr to this object
		//         - also check if the copy constr. is really needed, would be easier without
		auto buf = StagingSystem::instance().addBufferBlock(this);
#endif
		shared_->stagingOffset_ = 0;
		if (buf.get() != nullptr) {
			// the block was added to the staging system.
			// the system will globally manage updates and resizes of the staging buffer.
			shared_->stagingBuffer_ = buf;
			shared_->isGloballyStaged_ = true;
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
			REGEN_INFO("Created local staging buffer for block \""
							   << getBlockName() << "\" with size " << requiredSize_
							   << " Bytes and " << shared_->stagingBuffer_->numBufferSegments()
							   << " segments.");
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
							   << getBlockName() << "\" to " << desiredNumSegments
							   << " segments due to high stall rate.");
			if (desiredNumSegments < shared_->stagingBuffer_->maxRingSegments()) {
				shared_->stagingBuffer_->resizeBuffer(
						requiredSize_,
						desiredNumSegments);
				shared_->stagingBuffer_->resetStallRate();
			}
		}
	}
	if (!shared_->stagingBuffer_->hasAdoptedRange() && stagingFlags_.useExplicitFlushing()) {
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
			std::memset(input->lastStamp.data(), 0, input->lastStamp.size() * sizeof(uint32_t));
		}
		shared_->numBufferSegments_ = numStagingSegments;
		REGEN_INFO("Resized block \""
						   << getBlockName() << "\" to " << numStagingSegments
						   << " segments.");
	}

	if (shared_->stagingBuffer_->stagingFlags().isReadable()) {
		// Copy from draw buffer to the staging buffer, then read from the staging buffer into CPU memory.
		if (!updateReadBuffer()) {
			REGEN_WARN("BufferBlock: Failed to update read buffer for block \""
							   << getBlockName() << "\". This is likely a bug.");
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
		//REGEN_INFO("Wrote " << updatedSize_ << " bytes to staging buffer for block \""
		//					<< getBlockName() << "\" with "
		//					<< numDirtySegments_ << " dirty segments.");
	} else {
		REGEN_WARN("BufferBlock: No client data to update in staging buffer for block \""
						   << getBlockName() << "\". This is likely a bug.");
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
			updateStridedData(bufferInput);
			if (bufferInput.alignedData) {
				shared_->stagingBuffer_->setSubData(
						drawBufferRef_,
						localOffset,
						bufferInput.alignedSize,
						bufferInput.alignedData);
			} else {
				auto mapped = bufferInput.input->mapClientDataRaw(ShaderData::READ);
				shared_->stagingBuffer_->setSubData(
						drawBufferRef_,
						localOffset,
						bufferInput.inputSize,
						mapped.r);
			}
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
					copyBufferData1(bufferData, dirtyRange_b.offset, bufferInput);
				}
				shared_->stagingBuffer_->endMappedWrite(
						drawBufferRef_,
						*drawBufferRange_.get(),
						shared_->stagingOffset_);
			} // else: frame was dropped
		}
		stamp_ += 1;
	} else { // full update: map the whole range between the first and last dirty segment.
		auto &firstSegment = dirtyBufferRanges_[0];
		auto &lastSegment = dirtyBufferRanges_[numDirtySegments_ - 1];
		const uint32_t mapRangeSize = lastSegment.offset - firstSegment.offset + lastSegment.size;
		const uint32_t localOffset = shared_->stagingOffset_ + firstSegment.offset;

		byte *bufferData = shared_->stagingBuffer_->beginMappedWrite(
				drawBufferRef_,
				false,
				localOffset,
				mapRangeSize);
		if (bufferData) {
			copyBufferData(
					bufferData,
					firstSegment.offset,
					false);
			shared_->stagingBuffer_->endMappedWrite(
					drawBufferRef_,
					*drawBufferRange_.get(),
					shared_->stagingOffset_);
			stamp_ += 1;
		} // else: frame was dropped
	}
}

void BufferBlock::updatePersistentMapped() {
	if (stagingFlags_.useExplicitFlushing()) {
		// Explicit flushing is enabled, so we can update only the dirty segments.
		// And then add the segments to the flush queue.
		auto &firstSegment = dirtyBufferRanges_[0];
		auto &lastSegment = dirtyBufferRanges_[numDirtySegments_ - 1];
		const uint32_t mapRangeSize = lastSegment.offset - firstSegment.offset + lastSegment.size;
		const uint32_t localOffset = shared_->stagingOffset_ + firstSegment.offset;

		byte *bufferData = shared_->stagingBuffer_->beginMappedWrite(
				drawBufferRef_,
				true,
				firstSegment.offset,
				mapRangeSize);
		if (bufferData) {
			copyBufferData(bufferData, localOffset, true);
			// push the dirty segments to the flush queue for just-in-time flushing.
			shared_->stagingBuffer_->pushToFlushQueue(shared_->stagingOffset_ +
													  (BufferRange2ui *) (&dirtyBufferRanges_.data()[0].offset),
													  numDirtySegments_);
			shared_->stagingBuffer_->endMappedWrite(
					drawBufferRef_,
					*drawBufferRange_.get(),
					shared_->stagingOffset_);
			stamp_ += 1;
		} // else: frame was dropped
	} else {
		// Without explicit flushing, we need to update the whole mapped buffer range.
		byte *bufferData = shared_->stagingBuffer_->beginMappedWrite(
				drawBufferRef_,
				false,
				shared_->stagingOffset_,
				requiredSize_);
		if (bufferData) {
			for (auto &blockInput: blockInputs_) {
				auto &bufferInput = *blockInput.get();
				copyBufferData1(bufferData, 0u, bufferInput);
				lastInputStamp(bufferInput) = bufferInput.input->stamp();
			}
			shared_->stagingBuffer_->endMappedWrite(
					drawBufferRef_,
					*drawBufferRange_.get(),
					shared_->stagingOffset_);
			stamp_ += 1;
		} // else: frame was dropped
	}
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
