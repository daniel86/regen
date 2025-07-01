#include "buffer-block.h"
#include "ubo.h"
#include "ssbo.h"
#include "regen/states/state.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

//#define REGEN_BUFFER_BLOCK_DEBUG
#define BUFFER_BLOCK_DISABLE_PERSISTENT

static bool usePersistentMapping(BufferUsage usage) {
#ifdef BUFFER_BLOCK_DISABLE_PERSISTENT
	return false;
#else
	return usage == BUFFER_USAGE_STREAM_COPY || usage == BUFFER_USAGE_STREAM_DRAW;
#endif
}

BufferBlock::BufferBlock(
		BufferTarget target,
		BufferUsage usage,
		StorageQualifier storageQualifier,
		MemoryLayout memoryLayout)
		: BufferObject(target, usage),
		  storageQualifier_(storageQualifier),
		  memoryLayout_(memoryLayout),
		  usePersistentMapping_(usePersistentMapping(usage)) {
}

BufferBlock::BufferBlock(const BufferBlock &other)
		: BufferObject(other),
		  storageQualifier_(other.storageQualifier_),
		  memoryLayout_(other.memoryLayout_),
		  usePersistentMapping_(other.usePersistentMapping_),
		  blockInputs_(other.blockInputs_),
		  inputs_(other.inputs_),
		  ref_(other.ref_),
		  requiredSize_(other.requiredSize_),
		  stamp_(other.stamp_) {
}

BufferBlock::BufferBlock(const BufferObject &other)
		: BufferObject(other),
		  storageQualifier_(BufferBlock::BUFFER),
		  memoryLayout_(BufferBlock::STD430),
		  usePersistentMapping_(usePersistentMapping(usage_)) {
	auto block = dynamic_cast<const BufferBlock *>(&other);
	if (block != nullptr) {
		storageQualifier_ = block->storageQualifier_;
		memoryLayout_ = block->memoryLayout_;
		inputs_ = block->inputs_;
		blockInputs_ = block->blockInputs_;
		requiredSize_ = block->requiredSize_;
		stamp_ = block->stamp_;
		ref_ = block->ref_;
	} else {
		auto tbo = dynamic_cast<const TBO *>(&other);
		if (tbo != nullptr) {
			inputs_.emplace_back(tbo->input(), tbo->input()->name());

			auto uboInput = ref_ptr<BlockInput>::alloc();
			uboInput->input = tbo->input();
			uboInput->offset = 0;
			uboInput->lastStamp = tbo->input()->stamp();
			blockInputs_.emplace_back(uboInput);

			if (!tbo->allocations().empty()) {
				ref_ = tbo->allocations()[0];
			}
		} else {
			REGEN_WARN("BufferBlock: Unable to copy buffer object of unknown type.");
		}
	}
}

void BufferBlock::setPersistentMapping(bool isPersistent) {
	usePersistentMapping_ = isPersistent;
}

void BufferBlock::addBlockInput(const ref_ptr<ShaderInput> &input, const std::string &name) {
	auto uboInput = ref_ptr<BlockInput>::alloc();
	uboInput->input = input;
	blockInputs_.emplace_back(uboInput);
	inputs_.emplace_back(input, name);
}

void BufferBlock::resetSegments() {
	// note: we never clear the segments_ vector, we just reset the counters
	numNextSegments_ = 0;
}

BufferBlock::BlockSegment &BufferBlock::getLastSegment() {
	return nextSegments_[numNextSegments_ - 1];
}

BufferBlock::BlockSegment &BufferBlock::getNextSegment() {
	if (numNextSegments_ >= nextSegments_.size()) {
		// allocate a new segment if we have no more space
		numNextSegments_ += 1;
		return nextSegments_.emplace_back();
	} else {
		return nextSegments_[numNextSegments_++];
	}
}


void BufferBlock::updateBlockInputs() {
	bool lastChanged = false; // whether the last input changed or not
	bool hasNewSize = (requiredSize_ == 0); // whether the size of the block has changed
	hasClientData_ = true;
	updatedSize_ = 0u; // total size of the inputs that have changed
	resetSegments();

	for (int32_t inputIdx = 0; inputIdx < static_cast<int32_t>(blockInputs_.size()); ++inputIdx) {
		auto &blockInput = *blockInputs_[inputIdx].get();
		hasNewSize = hasNewSize || (blockInput.inputSize != blockInput.input->inputSize());
		hasClientData_ = hasClientData_ && blockInput.input->hasClientData();

		// construct contiguous segments of inputs that have changed
		if (blockInput.input->stamp() != blockInput.lastStamp) {
			updatedSize_ += blockInput.input->inputSize();
			if (lastChanged) {
				// this input adds to the current segment
				getLastSegment().append(blockInput, inputIdx);
			} else {
				// this input starts a new segment
				getNextSegment().set(blockInput, inputIdx);
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
			} else if (in->numElements() > 1 && memoryLayout_ == MemoryLayout::STD140) {
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
		if (memoryLayout_ == MemoryLayout::STD140) {
			static constexpr size_t std140Alignment = 16;
			size_t remainder = requiredSize_ % std140Alignment;
			if (remainder != 0) {
				requiredSize_ += std140Alignment - remainder;
			}
		}
	}
}

void BufferBlock::updateStridedData(BlockInput &uboInput) {
	// Some attributes cannot be stored tightly packed in the buffer,
	// especially vec3 arrays or mat3 arrays cannot be stored tightly packed
	// in STD140 or STD430 layouts, so we need to align them to 16 bytes.
	// Which means there is a stride between array elements, which unfortunately
	// means that we need to copy element-by-element to the buffer instead of
	// copying the whole array at once using memcpy.
	auto &in = uboInput.input;
	auto numElements = in->numArrayElements() * in->numInstances();
	if (numElements == 1) {
		return;
	}
	auto elementSizeUnaligned = in->valsPerElement() * in->dataTypeBytes();
	if (memoryLayout_ == MemoryLayout::STD140) {
		// the GL specification states that the stride between array elements must be
		// rounded up to 16 bytes for STD140.
		if (elementSizeUnaligned % 16 == 0) {
			return;
		}
	} else if (memoryLayout_ == MemoryLayout::STD430) {
		// only vec3 and mat3 types need to be aligned to 16 bytes with STD430.
		if (elementSizeUnaligned != 12 && elementSizeUnaligned != 48) {
			return;
		}
	} else {
		return;
	}
	//REGEN_WARN("RE-ALIGN needed for input " << in->name() <<
	//		   " with " << numElements << " elements, unaligned size: " << elementSizeUnaligned);
	auto elementSizeAligned = elementSizeUnaligned + (16 - elementSizeUnaligned % 16);
	auto dataSizeAligned = elementSizeAligned * numElements;
	if (dataSizeAligned != uboInput.alignedSize) {
		delete[] uboInput.alignedData;
		uboInput.alignedSize = dataSizeAligned;
		uboInput.alignedData = new byte[uboInput.alignedSize];
	}
	auto clientData = in->mapClientDataRaw(ShaderData::READ);
	auto *src = clientData.r;
	auto *dst = uboInput.alignedData;
	for (unsigned int i = 0; i < numElements; ++i) {
		memcpy(dst, src, elementSizeUnaligned);
		src += elementSizeUnaligned;
		dst += elementSizeAligned;
	}
}

void BufferBlock::copyBufferData1(char *bufferData, BlockInput &uboInput) {
	updateStridedData(uboInput);
	if (uboInput.alignedData) {
		// NOTE: the buffer is mapped starting from the first segment, so we need to
		//       adjust the offset to the first segment's offset.
		//       However, ths is not the case for persistent mapping, where the whole
		//       buffer is mapped and the offset is relative to the start of the buffer.
		uint32_t offset = uboInput.offset;
		if (!usePersistentMapping_) offset -= nextSegments_[0].offset;
		memcpy(bufferData + offset,
			   uboInput.alignedData, uboInput.alignedSize);
	} else {
		auto mapped = uboInput.input->mapClientDataRaw(ShaderData::READ);
		uint32_t offset = uboInput.offset;
		if (!usePersistentMapping_) offset -= nextSegments_[0].offset;
		memcpy(bufferData + offset,
			   mapped.r,
			   uboInput.input->inputSize());
	}
}

void BufferBlock::copyBufferData(char *bufferData, bool partialWrite) {
	if (partialWrite) {
		// iterate over the changed segments and copy only those
		for (uint32_t segmentIdx = 0; segmentIdx < numNextSegments_; ++segmentIdx) {
			auto &segment = nextSegments_[segmentIdx];

			for (uint32_t inputIdx = segment.startIdx; inputIdx <= segment.endIdx; ++inputIdx) {
				auto &uboInput = *blockInputs_[inputIdx].get();
				copyBufferData1(bufferData, uboInput);
				uboInput.lastStamp = uboInput.input->stamp();
			}
		}
	} else { // full write of mapped range
		// get start and end indices from first and last segment
		// note: in case of persistent mapping, we always must write the whole buffer range,
		//       as the whole range is mapped.
		uint32_t startIdx = (usePersistentMapping_ ? 0u : nextSegments_[0].startIdx);
		uint32_t endIdx = (usePersistentMapping_ ?
						   (blockInputs_.size() - 1) :
						   nextSegments_[numNextSegments_ - 1].endIdx);

		for (uint32_t inputIdx = startIdx; inputIdx <= endIdx; ++inputIdx) {
			auto &uboInput = *blockInputs_[inputIdx].get();
			copyBufferData1(bufferData, uboInput);
			uboInput.lastStamp = uboInput.input->stamp();
		}
	}
}

void BufferBlock::resize() {
	// enforce rebinding
	bindingIndex_ = -1;
	if (ref_.get()) {
		free(ref_.get());
	}
	ref_ = allocBytes(requiredSize_);
	if (!ref_.get()) {
		REGEN_ERROR("failed to allocate buffer.");
		isBlockValid_ = false;
		return;
	} else {
		isBlockValid_ = true;
	}
	allocatedSize_ = requiredSize_;
}

void BufferBlock::update(bool forceUpdate) {
	// NOTE: this function is performance critical!

	if (!isBlockValid_) return;
	updateBlockInputs();
	bool needsResize = allocatedSize_ != requiredSize_;
	bool needsUpdate = (numNextSegments_ > 0 || forceUpdate) && hasClientData_;
	if (!needsUpdate && !needsResize) { return; }
#ifdef REGEN_BUFFER_BLOCK_DEBUG
	auto t0 = std::chrono::high_resolution_clock::now();
#endif
	if (forceUpdate || needsResize) {
		numNextSegments_ = 0;
		auto &segment = getNextSegment();
		segment.offset = 0;
		segment.size = requiredSize_;
		segment.startIdx = 0;
		segment.endIdx = static_cast<uint32_t>(blockInputs_.size() - 1);
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
		if (usePersistentMapping_) {
			updatePersistent(needsResize);
		} else {
			updateNonPersistent();
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

void BufferBlock::updateNonPersistent() {
	if (persistentMapping_.get()) {
		persistentMapping_ = {};
	}
	// selectively enable partial updates
	bool partialUpdate = false;
	if (numNextSegments_ > 1) {
		// activate partial update if at least two non-contiguous segments
		// of the buffer block are updated. and their size is less than 33% of the total size.
		float updatedRatio = updatedSize_ / static_cast<float>(requiredSize_);
		partialUpdate = (updatedRatio < 0.33f);
	}

	// map the changed segment range such that each updated segment is covered by the mapping.
	const auto &firstSegment = nextSegments_[0];
	const auto &lastSegment = nextSegments_[numNextSegments_ - 1];
	auto mapRangeSize = lastSegment.offset - firstSegment.offset + lastSegment.size;

	uint32_t mappingFlags = MAP_WRITE;
	if (!partialUpdate) { mappingFlags |= MAP_INVALIDATE_RANGE; }

	RenderState::get()->buffer(glTarget_).apply(ref_->bufferID());
	void *bufferData = map(glTarget_,
						   ref_->address() + firstSegment.offset,
						   mapRangeSize,
						   mappingFlags);
	if (bufferData) {
		copyBufferData(static_cast<char *>(bufferData), partialUpdate);
		unmap();
		stamp_ += 1;
	} else {
		REGEN_WARN("failed to map buffer");
		GL_ERROR_LOG();
		isBlockValid_ = false;
	}
}

void BufferBlock::updatePersistent(bool needsResize) {
	// TODO: selectively enable partial updates
	static const bool partialUpdate = false;
	static constexpr uint32_t mappingFlags = MAP_WRITE | MAP_PERSISTENT | MAP_COHERENT;
	// TODO: Enable FLUSH_EXPLICIT + disable COHERENT -> should be faster!
	//       However, I keep getting nullptr from glMapBufferRange with FLUSH_EXPLICIT on AMD GPU.
	//   - XXX: It could be that multiple copies of the buffer create a persistent mapping! but unlikely that's the problem
	//   - Consider using GL_MAP_UNSYNCHRONIZED_BIT with manual sync over GL_MAP_INVALIDATE_RANGE_BIT.
	//static constexpr uint32_t mappingFlags = MAP_WRITE | MAP_PERSISTENT | MAP_FLUSH_EXPLICIT;
	if (!persistentMapping_.get()) {
		persistentMapping_ = ref_ptr<BufferMapping>::alloc(
				mappingFlags,
				TRIPLE_BUFFER,
				BufferMapping::RING_BUFFER);
		// avoid any waiting for fences, if we hit a fence, we will just skip the update to keep it fast!
		// NOTE: for some reason this causes flickering on my test with AMD GPU, so I disable it for now.
		//persistentMapping_->setAllowFrameDropping(true);

		if (!persistentMapping_->initializeMapping(ref_->allocatedSize(), glTarget_)) {
			REGEN_WARN("something went wrong with persistent mapping initialization.");
			isBlockValid_ = false;
		}
	} else if (needsResize) {
		if (!persistentMapping_->initializeMapping(ref_->allocatedSize(), glTarget_)) {
			REGEN_WARN("something went wrong with persistent mapping re-initialization.");
			isBlockValid_ = false;
		}
	}
	auto *mappedData = persistentMapping_->beginWriteBuffer(partialUpdate);
	if (mappedData) {
		//RenderState::get()->buffer(glTarget_).apply(ref_->bufferID());
		copyBufferData(static_cast<char *>(mappedData), partialUpdate);
		persistentMapping_->endWriteBuffer(ref_, glTarget_);
		stamp_ += 1;
	}
}

void BufferBlock::enableBufferBlock(GLint loc) {
	if (!isBlockValid_) return;
	if (bindingIndex_ != loc && bindingIndex_ != -1) {
		// seems the buffer switched to another index!
		// this is something the buffer manager should try to avoid, but there are some situations
		// where it might be difficult.
		// In case of doing the switch, we need to unbind the old binding index.
		auto &actual = RenderState::get()->bufferRange(glTarget_).value(bindingIndex_);
		if (actual.buffer_ == ref_->bufferID() &&
			actual.offset_ == ref_->address() &&
			actual.size_ == ref_->allocatedSize()) {
			RenderState::get()->bufferRange(glTarget_).apply(bindingIndex_, BufferRange::nullReference());
			bindingIndex_ = -1;
		}
	}
	update();
	bind(loc);
	bindingIndex_ = loc;
}

ref_ptr<BufferBlock> BufferBlock::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto blockType = input.getValue<std::string>("type", "ubo");
	auto usageHint = input.getValue<BufferUsage>("usage", BUFFER_USAGE_DYNAMIC_DRAW);
	ref_ptr<BufferBlock> block;
	if (blockType == "ubo") {
		block = ref_ptr<UBO>::alloc(input.getName(), usageHint);
	} else if (blockType == "ssbo") {
		block = ref_ptr<SSBO>::alloc(input.getName(), usageHint);
	} else {
		REGEN_WARN("Unknown buffer block type '" << blockType << "'. Using UBO.");
		block = ref_ptr<UBO>::alloc(input.getName(), usageHint);
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

std::ostream &regen::operator<<(std::ostream &out, const BufferBlock::MemoryLayout &v) {
	switch (v) {
		case BufferBlock::MemoryLayout::STD140:
			out << "std140";
			break;
		case BufferBlock::MemoryLayout::STD430:
			out << "std430";
			break;
		case BufferBlock::MemoryLayout::PACKED:
			out << "packed";
			break;
		case BufferBlock::MemoryLayout::SHARED:
			out << "shared";
			break;
	}
	return out;
}

std::ostream &regen::operator<<(std::ostream &out, const BufferBlock::StorageQualifier &v) {
	switch (v) {
		case BufferBlock::StorageQualifier::UNIFORM:
			out << "uniform";
			break;
		case BufferBlock::StorageQualifier::BUFFER:
			out << "buffer";
			break;
		case BufferBlock::StorageQualifier::IN:
			out << "in";
			break;
		case BufferBlock::StorageQualifier::OUT:
			out << "out";
			break;
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BufferBlock::MemoryLayout &v) {
	std::string val;
	in >> val;
	boost::to_lower(val);
	if (val == "std140") v = BufferBlock::MemoryLayout::STD140;
	else if (val == "std430") v = BufferBlock::MemoryLayout::STD430;
	else if (val == "packed") v = BufferBlock::MemoryLayout::PACKED;
	else if (val == "shared") v = BufferBlock::MemoryLayout::SHARED;
	else {
		REGEN_WARN("Unknown memory layout '" << val << "'. Using STD140.");
		v = BufferBlock::MemoryLayout::STD140;
	}
	return in;
}

std::istream &regen::operator>>(std::istream &in, BufferBlock::StorageQualifier &v) {
	std::string val;
	in >> val;
	boost::to_lower(val);
	if (val == "uniform") v = BufferBlock::StorageQualifier::UNIFORM;
	else if (val == "buffer") v = BufferBlock::StorageQualifier::BUFFER;
	else if (val == "in") v = BufferBlock::StorageQualifier::IN;
	else if (val == "out") v = BufferBlock::StorageQualifier::OUT;
	else {
		REGEN_WARN("Unknown storage qualifier '" << val << "'. Using UNIFORM.");
		v = BufferBlock::StorageQualifier::UNIFORM;
	}
	return in;
}
