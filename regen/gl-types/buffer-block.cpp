#include "buffer-block.h"
#include "ubo.h"
#include "ssbo.h"
#include "regen/states/state.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

//#define REGEN_BUFFER_BLOCK_DEBUG
//#define BUFFER_BLOCK_DISABLE_PERSISTENT
//#define BUFFER_BLOCK_DISABLE_PERSISTENT_MEDIUM_SIZE
//#define BUFFER_BLOCK_DISABLE_PERSISTENT_SMALL_SIZE
//#define BUFFER_BLOCK_DISABLE_EXPLICIT_FLUSHING

uint32_t BufferBlock::MIN_SIZE_MEDIUM = 256; // Bytes
//uint32_t BufferBlock::MIN_SIZE_MEDIUM = 512; // Bytes
uint32_t BufferBlock::MIN_SIZE_LARGE = 64 * 1024; // 64 KiB
uint32_t BufferBlock::MIN_SIZE_VERY_LARGE = 1024 * 1024; // 1 MiB

uint32_t BufferBlock::temporaryMappingPartialMinSegments = 6;
float BufferBlock::temporaryMappingPartialMaxUpdateRatio = 0.33f;

BufferBlock::BufferBlock(
		BufferTarget target,
		const BufferUpdateFlags &hints,
		Qualifier blockQualifier,
		BufferMemoryLayout memoryLayout)
		: BufferObject(target, hints),
		  blockQualifier_(blockQualifier),
		  memoryLayout_(memoryLayout),
		  stagingFlags_(target, hints) {
	drawBufferRange_ = ref_ptr<BufferRange>::alloc();
	// initially assume it is a GPU-only buffer.
	// the flag will be switched to something else based on the inputs added.
	setBufferAccessMode(BUFFER_GPU_ONLY);
	setBufferMapMode(BUFFER_MAP_DISABLED);
	// if the buffer will never be updated, we can use implicit staging.
	if (hints.frequency == BUFFER_UPDATE_NEVER) {
		setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
	}
}

BufferBlock::BufferBlock(const BufferBlock &other)
		: BufferObject(other),
		  blockQualifier_(other.blockQualifier_),
		  memoryLayout_(other.memoryLayout_),
		  bindingIndex_(other.bindingIndex_),
		  hasClientData_(other.hasClientData_),
		  isBlockValid_(other.isBlockValid_),
		  inputs_(other.inputs_),
		  ref_(other.ref_),
		  requiredSize_(other.requiredSize_),
		  estimatedSize_(other.estimatedSize_),
		  updatedSize_(other.updatedSize_),
		  stamp_(other.stamp_),
		  blockInputs_(other.blockInputs_),
		  stagingFlags_(other.stagingFlags_),
		  userDefinedBufferingMode_(other.userDefinedBufferingMode_),
		  stagingBuffer_(other.stagingBuffer_),
		  drawBufferRange_(other.drawBufferRange_) {
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
		stagingBuffer_ = block->stagingBuffer_;
		bindingIndex_ = block->bindingIndex_;
		hasClientData_ = block->hasClientData_;
		isBlockValid_ = block->isBlockValid_;
		inputs_ = block->inputs_;
		ref_ = block->ref_;
		requiredSize_ = block->requiredSize_;
		estimatedSize_ = block->estimatedSize_;
		updatedSize_ = block->updatedSize_;
		stamp_ = block->stamp_;
		blockInputs_ = block->blockInputs_;
		stagingFlags_ = block->stagingFlags_;
		userDefinedBufferingMode_ = block->userDefinedBufferingMode_;
		stagingBuffer_ = block->stagingBuffer_;
		drawBufferRange_ = block->drawBufferRange_;
	} else {
		auto tbo = dynamic_cast<const TBO *>(&other);
		if (tbo != nullptr) {
			inputs_.emplace_back(tbo->input(), tbo->input()->name());

			auto bufferInput = ref_ptr<BlockInput>::alloc();
			bufferInput->input = tbo->input();
			bufferInput->offset = 0;
			bufferInput->lastStamp[0] = tbo->input()->stamp();
			blockInputs_.emplace_back(bufferInput);

			if (!tbo->allocations().empty()) {
				ref_ = tbo->allocations()[0];
			}
		} else {
			REGEN_WARN("BufferBlock: Unable to copy buffer object of unknown type.");
		}
	}
}

void BufferBlock::setBufferingMode(BufferingMode mode) {
	userDefinedBufferingMode_ = mode;
	stagingFlags_.bufferingMode = mode;
}

std::string BufferBlock::getBlockName() const {
	auto *si = dynamic_cast<const ShaderInput*>(this);
	if (si != nullptr) {
		return si->name();
	} else if (!blockInputs_.empty()) {
		return REGEN_STRING("{" << blockInputs_[0]->input->name() << "}");
	} else {
		return "BufferBlock";
	}
}

void BufferBlock::setStagingUpdateHints(const BufferUpdateFlags &hints) {
	if (!flags_.useExplicitStaging()) {
		// if we are not using separate staging buffers, we need to set the same update hint
		// for the main buffer as well.
		setBufferUpdateHint(hints);
		stagingFlags_.updateHints = flags_.updateHints;
	} else {
		stagingFlags_.updateHints = hints;
	}
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
	if (!userDefinedBufferingMode_.has_value()) {
		stagingFlags_.bufferingMode = mode;
	}
}

void BufferBlock::enablePersistentMapping(bool useFlushExplicit) {
	if (useFlushExplicit) {
		setStagingMapMode(BUFFER_MAP_PERSISTENT_FLUSH);
	} else {
		setStagingMapMode(BUFFER_MAP_PERSISTENT_COHERENT);
	}
}

void BufferBlock::enablePersistentMapping_(bool useFlushExplicit) {
#ifdef BUFFER_BLOCK_DISABLE_PERSISTENT_SMALL_SIZE
	// use temporary mapping.
	setStagingMapMode(BUFFER_MAP_TEMPORARY);
#else
	// use coherent persistent mapping.
	enablePersistentMapping(useFlushExplicit);
#endif
}

BufferSizeClass BufferBlock::getBufferSizeClass(uint32_t size) {
	if (size < BufferBlock::MIN_SIZE_MEDIUM) {
		return BUFFER_SIZE_SMALL;
	} else if (size < BufferBlock::MIN_SIZE_LARGE) {
		return BUFFER_SIZE_MEDIUM;
	} else if (size < BufferBlock::MIN_SIZE_VERY_LARGE) {
		return BUFFER_SIZE_LARGE;
	} else {
		return BUFFER_SIZE_VERY_LARGE;
	}
}

void BufferBlock::addBlockInput(const ref_ptr<ShaderInput> &input, const std::string &name) {
	auto bufferInput = ref_ptr<BlockInput>::alloc();
	bufferInput->input = input;
	blockInputs_.emplace_back(bufferInput);
	inputs_.emplace_back(input, name);
	estimatedSize_ += input->numElements() * input->elementSize();

	// update the storage flags based on added inputs
	if (input->hasClientData() && flags_.updateHints.frequency != BUFFER_UPDATE_NEVER) {
		auto sizeClass = getBufferSizeClass(estimatedSize_);
		// input has client data, so we need to set the access mode such that the CPU can write to it.
		enableWriteAccess();

		if (sizeClass == BUFFER_SIZE_SMALL) {
			// If the buffer is small (e.g. < 512 Byte), then ...
			if (stagingFlags_.areUpdatesFrequent()) {
				// (a) use single-buffered coherent persistent mapping for frequent updates.
				setStagingBuffering(SINGLE_BUFFER);
				enablePersistentMapping_(false);
			} else if (stagingFlags_.areUpdatesVeryFrequent()) {
				// (b) use double-buffered coherent persistent mapping for very frequent updates.
				// TODO: Consider using UNSYNCHRONIZED for very high frequency updates.
				setStagingBuffering(DOUBLE_BUFFER);
				enablePersistentMapping_(false);
			} else {
				// (c) use single-buffered coherent persistent mapping for rare updates.
				setStagingBuffering(SINGLE_BUFFER);
				enablePersistentMapping_(false);
				// TODO: Consider using implicit staging instead for small writable buffers with infrequent full updates.
				//       - It could be worthwhile to enable multi-buffering with implicit staging for slightly
				//       larger buffers, e.g. 256 Bytes to a few KB e.g. 8KB. then classify >8KB as medium.
				//if (!stagingFlags_.areUpdatesPartial()) {
				//	stagingFlags_.syncFlags |= BUFFER_SYNC_IMPLICIT_STAGING;
				//	setStagingMapMode(BUFFER_MAP_DISABLED);
				//} else {}
			}
		} else if (sizeClass == BUFFER_SIZE_MEDIUM) {
			// If the buffer is medium sized (e.g. < 64KB), then ...
			if (stagingFlags_.areUpdatesFrequent() || stagingFlags_.areUpdatesVeryFrequent()) {
				// (a) use 3-ring staging buffer with persistent mapping for frequent updates.
				//     In addition, use explicit flushing in case of partial updates.
				setStagingBuffering(TRIPLE_BUFFER);
#ifdef BUFFER_BLOCK_DISABLE_EXPLICIT_FLUSHING
				enablePersistentMapping_(false);
#else
				enablePersistentMapping_(stagingFlags_.areUpdatesPartial());
#endif
			} else {
				// (b) use single-buffering in staging with unmapped copy
				//     or temporary mapping for infrequent updates.
				setStagingBuffering(SINGLE_BUFFER);
				if (stagingFlags_.areUpdatesPartial()) {
					setStagingMapMode(BUFFER_MAP_DISABLED);
				} else {
					setStagingMapMode(BUFFER_MAP_TEMPORARY);
				}
			}
		} else if (sizeClass == BUFFER_SIZE_LARGE) {
			// If the buffer is large (e.g. < 1MB)
			if (stagingFlags_.areUpdatesFrequent() || stagingFlags_.areUpdatesVeryFrequent()) {
				// (a) if updates are frequent, then use 2-ring staging buffer with range invalidation.
				setStagingBuffering(DOUBLE_BUFFER);
				setStagingMapMode(BUFFER_MAP_TEMPORARY);
			} else {
				// (b) if updates are infrequent, then use single-buffering in staging and avoid mapping
				//     the buffer to CPU memory.
				setStagingBuffering(SINGLE_BUFFER);
				setStagingMapMode(BUFFER_MAP_DISABLED);
			}
		} else { // sizeClass == BUFFER_SIZE_VERY_LARGE
			// If the buffer is very large (e.g. > 1MB), avoid mapping it to CPU memory.
			setStagingBuffering(SINGLE_BUFFER);
			setStagingMapMode(BUFFER_MAP_DISABLED);
		}
	}
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

uint32_t& BufferBlock::lastInputStamp(BlockInput &blockInput) {
	return stagingBuffer_.get() ?
		blockInput.lastStamp[stagingBuffer_->nextWriteIndex()] :
		blockInput.lastStamp[0];
}

void BufferBlock::updateBlockInputs() {
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
					<< " first input: " << blockInputs_[0]->input->name());
			}
		}
	}
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
			   " with " << numElements << " elements, unaligned size: " << elementSizeUnaligned);
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

void BufferBlock::resize() {
	// enforce rebinding
	bindingIndex_ = -1;
	if (ref_.get()) {
		free(ref_.get());
	}

	// if neither read nor write access is allowed, we can use implicit staging.
	if (!stagingFlags_.isWritable() && !stagingFlags_.isReadable()) {
		setSyncFlag(BUFFER_SYNC_IMPLICIT_STAGING);
	}

	if (!flags_.useExplicitStaging()) {
		// in case of implicit staging with multi-buffering, we need to allocate space for each segment
		// of a ring buffer in the draw buffer.
		ref_ = adoptBufferRange(requiredSize_ * (int)flags_.bufferingMode);
	} else {
		ref_ = adoptBufferRange(requiredSize_);
	}

	// validate the allocation
	if (!ref_.get()) {
		REGEN_ERROR("failed to allocate buffer for buffer flags " << flags_);
		isBlockValid_ = false;
		return;
	}
	if (isMapModePersistent(flags_.mapMode) && !ref_->mappedData()) {
		REGEN_WARN("something went wrong with persistent mapping for buffer flags " << flags_);
		isBlockValid_ = false;
		return;
	}
	isBlockValid_ = true;

	allocatedSize_ = requiredSize_;
	// set draw buffer range to first segment in the ring buffer
	drawBufferRange_->buffer_ = ref_->bufferID();
	drawBufferRange_->offset_ = ref_->address();
	drawBufferRange_->size_ = requiredSize_;

	// initialize the staging buffer.
	// all access will be tunneled through the staging buffer.
	// depending on configuration the staging is explicit (default) or implicit.
	stagingBuffer_ = ref_ptr<StagingBuffer>::alloc(ref_, stagingFlags_);

	// reset the update stamps: set their state to zero to force reloading
	// all segments. this is needed as staging buffer adopts fresh buffer range.
	for (auto &input: blockInputs_) {
		input->lastStamp.resize((int)stagingFlags_.bufferingMode);
		std::memset(input->lastStamp.data(), 0, input->lastStamp.size() * sizeof(uint32_t));
	}

	REGEN_INFO("Created buffer \"" << getBlockName() << "\""
			<< " size-class: " << getBufferSizeClass(requiredSize_)
			<< " required-size: " << requiredSize_
			<< " estimated-size: " << estimatedSize_
			<< "\n\t   draw-flags: " << flags_
			<< "\n\tstaging-flags: " << stagingFlags_);
}

void BufferBlock::update(bool forceUpdate) {
	// NOTE: this function is performance critical!

	if (!isBlockValid_) return;
	updateBlockInputs();
	bool needsResize = allocatedSize_ != requiredSize_;
	bool needsUpdate = (numDirtySegments_ > 0 || forceUpdate) && hasClientData_;
	if (!needsUpdate && !needsResize) { return; }
#ifdef REGEN_BUFFER_BLOCK_DEBUG
	auto t0 = std::chrono::high_resolution_clock::now();
#endif
	if (forceUpdate || needsResize) {
		numDirtySegments_ = 0;
		createNextDirtySegment();
		dirtyBufferRanges_[0].offset = 0;
		dirtyBufferRanges_[0].size = requiredSize_;
		dirtySegmentRanges_[0].startIdx = 0;
		dirtySegmentRanges_[0].endIdx = static_cast<uint32_t>(blockInputs_.size() - 1);
	}
	lock_.lock();

#ifdef REGEN_BUFFER_BLOCK_DEBUG
	auto t1 = std::chrono::high_resolution_clock::now();
#endif
	if (needsResize) {
		resize();
	}
#ifdef REGEN_BUFFER_BLOCK_DEBUG
	auto t2 = std::chrono::high_resolution_clock::now();
#endif
	if (hasClientData_) {
		if (isMapModePersistent(stagingFlags_.mapMode)) {
			updatePersistentMapped();
		} else if (stagingFlags_.mapMode == BUFFER_MAP_TEMPORARY) {
			updateTemporaryMapped();
		} else {
			updateNonMapped();
		}
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

	lock_.unlock();
}

void BufferBlock::updateNonMapped() {
	// iterate over the changed segments and copy only those into the staging buffer.
	stagingBuffer_->beginNonMappedWrite();

	for (uint32_t segmentIdx = 0; segmentIdx < numDirtySegments_; ++segmentIdx) {
		auto &dirtyRange_s = dirtySegmentRanges_[segmentIdx];

		for (uint32_t inputIdx = dirtyRange_s.startIdx; inputIdx <= dirtyRange_s.endIdx; ++inputIdx) {
			auto &bufferInput = *blockInputs_[inputIdx].get();
			if (bufferInput.alignedData) {
				stagingBuffer_->setSubData(
					bufferInput.offset,
					bufferInput.alignedSize,
					bufferInput.alignedData);
			} else {
				auto mapped = bufferInput.input->mapClientDataRaw(ShaderData::READ);
				stagingBuffer_->setSubData(
					bufferInput.offset,
					bufferInput.inputSize,
					mapped.r);
			}
			lastInputStamp(bufferInput) = bufferInput.input->stamp();
		}
	}
	stagingBuffer_->endNonMappedWrite(*drawBufferRange_.get());
	stamp_ += 1;
}

void BufferBlock::updateTemporaryMapped() {
	// Selectively enable partial updates.
	// However, note that we need to do multiple mappings in case of partial updates,
	// as we need to always should use range invalidation for the mapped range.
	// In case of full updates, we can map the whole buffer range at once with invalidation.
	const bool doPartialUpdate = (numDirtySegments_ > 1) &&
		// disable partial updates for small buffers, as they are fast to update anyway.
		(getBufferSizeClass(estimatedSize_) > BUFFER_SIZE_SMALL) &&
		// disable partial updates in case the update covers a large part of the buffer.
		(updatedSize_ / static_cast<float>(requiredSize_) > BufferBlock::temporaryMappingPartialMaxUpdateRatio) &&
		// disable partial updates for larger number of dirty segments.
		(numDirtySegments_ <= BufferBlock::temporaryMappingPartialMinSegments);

	if (doPartialUpdate) {
		for (uint32_t segmentIdx = 0; segmentIdx < numDirtySegments_; ++segmentIdx) {
			auto &dirtyRange_s = dirtySegmentRanges_[segmentIdx];
			auto &dirtyRange_b = dirtyBufferRanges_[segmentIdx];
			byte *bufferData = (byte*)stagingBuffer_->beginMappedWrite(
					false, dirtyRange_b.offset, dirtyRange_b.size);
			if (bufferData) {
				for (uint32_t inputIdx = dirtyRange_s.startIdx; inputIdx <= dirtyRange_s.endIdx; ++inputIdx) {
					auto &bufferInput = *blockInputs_[inputIdx].get();
					copyBufferData1(bufferData, dirtyRange_b.offset, bufferInput);
				}
				stagingBuffer_->endMappedWrite(*drawBufferRange_.get());
			} // else: frame was dropped
		}
		stamp_ += 1;
	} else { // full update: map the whole range between the first and last dirty segment.
		auto &firstSegment = dirtyBufferRanges_[0];
		auto &lastSegment = dirtyBufferRanges_[numDirtySegments_ - 1];
		uint32_t mapRangeSize = lastSegment.offset - firstSegment.offset + lastSegment.size;

		void *bufferData = stagingBuffer_->beginMappedWrite(
				false, firstSegment.offset, mapRangeSize);
		if (bufferData) {
			copyBufferData(static_cast<byte*>(bufferData), firstSegment.offset, false);
			stagingBuffer_->endMappedWrite(*drawBufferRange_.get());
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
		uint32_t mapRangeSize = lastSegment.offset - firstSegment.offset + lastSegment.size;

		void *bufferData = stagingBuffer_->beginMappedWrite(
				true, firstSegment.offset, mapRangeSize);
		if (bufferData) {
			copyBufferData(static_cast<byte*>(bufferData), firstSegment.offset, true);
			// push the dirty segments to the flush queue for just-in-time flushing.
			stagingBuffer_->pushToFlushQueue(
				(BufferRange2ui*)(&dirtyBufferRanges_.data()[0].offset),
				numDirtySegments_);
			stagingBuffer_->endMappedWrite(*drawBufferRange_.get());
			stamp_ += 1;
		} // else: frame was dropped
	} else {
		// Without explicit flushing, we need to update the whole mapped buffer range.
		auto *mappedData = stagingBuffer_->beginMappedWrite(
			false, 0u, stagingBuffer_->segmentSize());
		if (mappedData) {
			auto bufferData = static_cast<byte*>(mappedData);
			for (auto &blockInput: blockInputs_) {
				auto &bufferInput = *blockInput.get();
				copyBufferData1(bufferData, 0u, bufferInput);
				lastInputStamp(bufferInput) = bufferInput.input->stamp();
			}
			stagingBuffer_->endMappedWrite(*drawBufferRange_.get());
			stamp_ += 1;
		} // else: frame was dropped
	}
}

void BufferBlock::updateAllBuffers() {
	// cycle through the ring buffer once, and write to each segment.
	for (int idx=0; idx < stagingFlags_.bufferingMode; ++idx) {
		updatePersistentMapped();
	}
}

void BufferBlock::enableBufferBlock(GLint loc) {
	if (!isBlockValid_) return;
	auto *rs = RenderState::get();

	if (bindingIndex_ != loc && bindingIndex_ != -1) {
		// seems the buffer switched to another index!
		// this is something the buffer manager should try to avoid, but there are some situations
		// where it might be difficult.
		// In case of doing the switch, we need to unbind the old binding index.
		auto &actual = rs->bufferRange(glTarget_).value(bindingIndex_);
		if (actual.buffer_ == ref_->bufferID() &&
			actual.offset_ == ref_->address() &&
			actual.size_ == ref_->allocatedSize()) {
			rs->bufferRange(glTarget_).apply(bindingIndex_, BufferRange::nullReference());
			bindingIndex_ = -1;
		}
	}
	update();
	rs->bufferRange(glTarget_).apply(loc, *drawBufferRange_.get());
	// mark the point of accessing a mapped buffer segment for reading
	// which is needed to avoid writing to the buffer while it is being read.
	stagingBuffer_->markDrawAccessed(*drawBufferRange_.get());
	bindingIndex_ = loc;
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
