#include "buffer-block.h"
#include "ubo.h"
#include "ssbo.h"
#include "regen/states/state.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

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
	// TODO: avoid duplicate copy of data in update!
}

BufferBlock::BufferBlock(const BufferObject &other)
		: BufferObject(other),
		  storageQualifier_(BufferBlock::BUFFER),
		  memoryLayout_(BufferBlock::STD430),
		  usePersistentMapping_(usePersistentMapping(usage_)) {
	// TODO: avoid duplicate copy of data in update!
	auto block = dynamic_cast<const BufferBlock *>(&other);
	if (block != nullptr) {
		storageQualifier_ = block->storageQualifier_;
		memoryLayout_ = block->memoryLayout_;
		inputs_ = block->inputs_;
		blockInputs_ = block->blockInputs_;
		requiredSize_ = block->requiredSize_;
		stamp_ = block->stamp_;
		ref_ = block->ref_;
	}
	else {
		auto tbo = dynamic_cast<const TBO *>(&other);
		if (tbo != nullptr) {
			inputs_.emplace_back(tbo->input(), tbo->input()->name());
			auto &bi = blockInputs_.emplace_back();
			bi.input = tbo->input();
			bi.offset = 0;
			bi.lastStamp = tbo->input()->stamp();
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
	auto &uboInput = blockInputs_.emplace_back();
	uboInput.input = input;
	uboInput.offset = 0;
	uboInput.lastStamp = 0;
	inputs_.emplace_back(input, name);
}

void BufferBlock::updateBlockInputs() {
	requiredSize_ = 0;
	hasNewStamp_ = false;
	hasClientData_ = true;
	for (auto &blockInput: blockInputs_) {
		auto &in = blockInput.input;
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
		blockInput.offset = requiredSize_;
		if (in->numElements() > 1) {
			requiredSize_ += baseAlignment * alignmentCount * in->numElements();
		} else {
			requiredSize_ += baseSize * in->numElements();
		}
		if (blockInput.input->stamp() != blockInput.lastStamp) {
			hasNewStamp_ = true;
		}
		if (!in->hasClientData()) {
			hasClientData_ = false;
		}
	}
	// Round total size up to next multiple of 16 (vec4 alignment for std140)
	if (memoryLayout_ == MemoryLayout::STD140) {
		static constexpr size_t std140Alignment = 16;
		size_t remainder = requiredSize_ % std140Alignment;
		if (remainder != 0) {
			requiredSize_ += std140Alignment - remainder;
		}
	}
	// ignore stamps if there is no client data
	hasNewStamp_ = hasNewStamp_ && hasClientData_;
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

void BufferBlock::copyBufferData(char *bufferData, bool forceUpdate, bool partialWrite) {
	for (auto &uboInput: blockInputs_) {
		if (!forceUpdate && partialWrite &&
		    uboInput.input->stamp() == uboInput.lastStamp) { continue; }
		uboInput.lastStamp = uboInput.input->stamp();

		if (!uboInput.input->hasClientData()) { continue; }
		// copy the data to the buffer.
		updateStridedData(uboInput);
		if (uboInput.alignedData) {
			memcpy(bufferData + uboInput.offset,
				   uboInput.alignedData, uboInput.alignedSize);
		} else {
			auto mapped = uboInput.input->mapClientDataRaw(ShaderData::READ);
			memcpy(bufferData + uboInput.offset,
				   mapped.r,
				   uboInput.input->inputSize());
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
	// TODO: Consider using GL_MAP_UNSYNCHRONIZED_BIT with manual sync over GL_MAP_INVALIDATE_RANGE_BIT.
	// TODO: Build ranges of changed attributes and update only those ranges if num ranges is small, i.e. 1 or 2,
	//       and buffer is not mapped persistently.
	if (!isBlockValid_) return;
	updateBlockInputs();
	bool needsResize = allocatedSize_ != requiredSize_;
	bool needUpdate = hasNewStamp_ || needsResize || forceUpdate;
	if (!needUpdate) { return; }
	std::unique_lock<SpinLock> lock(lock_);

	if (needsResize) {
		resize();
	}
	if (!hasClientData_) {
		// do not copy data if there is no client data
		return;
	}
	static const bool partialUpdate = false;

	// temporary disable persistent mapping until it is working nicely...
	if (usePersistentMapping_) {
		static constexpr uint32_t mappingFlags = MAP_WRITE | MAP_PERSISTENT | MAP_COHERENT;
		// TODO: Enable FLUSH_EXPLICIT + disable COHERENT -> should be faster!
		//       However, I keep getting nullptr from glMapBufferRange with FLUSH_EXPLICIT on AMD GPU.
		//static constexpr uint32_t mappingFlags = MAP_WRITE | MAP_PERSISTENT | MAP_FLUSH_EXPLICIT;
		if (!persistentMapping_.get()) {
			persistentMapping_ = ref_ptr<BufferMapping>::alloc(
					mappingFlags,
					TRIPLE_BUFFER,
					BufferMapping::RING_BUFFER);
			// avoid any waiting for fences, if we hit a fence, we will just skip the update to keep it fast!
			// NOTE: for some reason this causes flickering on my test with AMD GPU, so I disable it for now.
			//persistentMapping_->setAllowFrameDropping(true);

			if(!persistentMapping_->initializeMapping(ref_->allocatedSize(), glTarget_)) {
				REGEN_WARN("something went wrong with persistent mapping initialization.");
				isBlockValid_ = false;
			}
		} else if (needsResize) {
			if(!persistentMapping_->initializeMapping(ref_->allocatedSize(), glTarget_)) {
				REGEN_WARN("something went wrong with persistent mapping re-initialization.");
				isBlockValid_ = false;
			}
		}
		auto *mappedData = persistentMapping_->beginWriteBuffer(partialUpdate);
		if (mappedData) {
			//RenderState::get()->buffer(glTarget_).apply(ref_->bufferID());
			copyBufferData(static_cast<char *>(mappedData), forceUpdate, partialUpdate);
			persistentMapping_->endWriteBuffer(ref_, glTarget_);
			stamp_ += 1;
		}
		return;
	} else {
		if (persistentMapping_.get()) {
			persistentMapping_ = {};
		}
		RenderState::get()->buffer(glTarget_).apply(ref_->bufferID());
		uint32_t mappingFlags = MAP_WRITE;
		if (!partialUpdate) {
			mappingFlags |= MAP_INVALIDATE_RANGE;
		}
		void *bufferData = map(ref_, mappingFlags);
		if (bufferData) {
			copyBufferData(static_cast<char *>(bufferData), forceUpdate, partialUpdate);
			unmap();
			stamp_ += 1;
		} else {
			REGEN_WARN("failed to map buffer");
			isBlockValid_ = false;
		}
	}
}

void BufferBlock::enableBufferBlock(GLint loc) {
	if (!isBlockValid_) return;
	if (bindingIndex_ != loc && bindingIndex_ != -1) {
		auto &actual = RenderState::get()->bufferRange(glTarget_).value(bindingIndex_);
		if (actual.buffer_ == ref_->bufferID() &&
			actual.offset_ == ref_->address() &&
			actual.size_ == ref_->allocatedSize()) {
			RenderState::get()->bufferRange(glTarget_).apply(bindingIndex_, BufferRange::nullReference());
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

