#include "regen/utility/strings.h"
#include "regen/utility/logging.h"
#include "regen/animation/animation.h"
#include "regen/memory/ssbo.h"
#include <stack>

#include "shader-input.h"
#include "../objects/mesh-processor.h"

using namespace regen;

NamedShaderInput::NamedShaderInput(
		const ref_ptr<ShaderInput> &in,
		const std::string &name,
		const std::string &type,
		const std::string &memberSuffix)
		: in_(in), name_(name), type_(type), memberSuffix_(memberSuffix) {
	if (name_.empty()) {
		name_ = in->name();
	}
	if (type_.empty()) {
		type_ = glenum::glslDataType(in->baseType(), in->valsPerElement());
	}
}

ShaderInput::ShaderInput(
		std::string_view name,
		GLenum baseType,
		uint32_t dataTypeBytes,
		int32_t valsPerElement,
		uint32_t numArrayElements,
		bool normalize)
		: name_(name),
		  baseType_(baseType),
		  dataType_(glenum::dataType(baseType, valsPerElement)),
		  dataTypeBytes_(dataTypeBytes),
		  baseSize_(dataTypeBytes * valsPerElement),
		  valsPerElement_(valsPerElement),
		  numArrayElements_(numArrayElements),
		  numElements_i_(static_cast<int32_t>(numArrayElements)),
		  numElements_ui_(numArrayElements),
		  bufferStamp_(0),
		  clientBuffer_(ref_ptr<ClientBuffer>::alloc()),
		  normalize_(normalize) {
	elementSize_ = baseSize_ * numArrayElements_;
	enableAttribute_ = &ShaderInput::enableAttribute_f;
	updateAlignment();
}

ShaderInput::ShaderInput(const ShaderInput &o)
		: name_(o.name_),
		  baseType_(o.baseType_),
		  dataType_(o.dataType_),
		  dataTypeBytes_(o.dataTypeBytes_),
		  baseSize_(o.baseSize_),
		  valsPerElement_(o.valsPerElement_),
		  baseAlignment_(o.baseAlignment_),
		  alignmentCount_(o.alignmentCount_),
		  alignedElementSize_(o.alignedElementSize_),
		  unalignedSize_(o.unalignedSize_),
		  vertexStride_(o.vertexStride_),
		  offset_(o.offset_),
		  inputSize_(o.inputSize_),
		  elementSize_(o.elementSize_),
		  numArrayElements_(o.numArrayElements_),
		  numVertices_(o.numVertices_),
		  numInstances_(o.numInstances_),
		  numElements_i_(o.numElements_i_),
		  numElements_ui_(o.numElements_ui_),
		  divisor_(o.divisor_),
		  memoryLayout_(o.memoryLayout_),
		  buffer_(o.buffer_),
		  bufferStamp_(o.bufferStamp_),
		  clientBuffer_(o.clientBuffer_),
		  normalize_(o.normalize_),
		  isVertexAttribute_(o.isVertexAttribute_),
		  transpose_(o.transpose_),
		  isConstant_(o.isConstant_),
		  isBufferBlock_(o.isBufferBlock_),
		  isStagedBuffer_(o.isStagedBuffer_),
		  forceArray_(o.forceArray_),
		  active_(o.active_),
		  schema_(o.schema_) {
	enableAttribute_ = &ShaderInput::enableAttribute_f;
	enableInput_ = o.enableInput_;
	// copy client data, if any
	if (o.hasClientData()) {
		auto mapped = o.mapClientDataRaw(BUFFER_GPU_READ);
		if (o.isVertexAttribute()) {
			setVertexData(o.numVertices_, mapped.r);
		} else {
			setInstanceData(o.numInstances_, o.divisor_, mapped.r);
		}
	}
}

ShaderInput::~ShaderInput() {
	if (bufferRef_.get()) {
		BufferObject::orphanBufferRange(bufferRef_.get());
	}
	bufferRef_ = {};
	if (clientBuffer_->isDataOwner()) {
		clientBuffer_->deallocateClientData();
	}
	clientBuffer_ = {};
}

void ShaderInput::updateAlignment() {
	if (memoryLayout_ == BUFFER_MEMORY_PACKED) {
		baseAlignment_ = PACKED_BASE_ALIGNMENT;
		alignedElementSize_ = baseSize_;
		alignmentCount_ = 1u;
		alignedInputSize_ = alignedElementSize_ * numElements_ui_;
	} else {
		baseAlignment_ = baseSize_;
		alignedElementSize_ = baseSize_;
		alignmentCount_ = 1u;
		if (baseSize_ == 12u) { // vec3
			baseAlignment_ = 16u;
		} else if (baseSize_ == 48u) { // mat3
			baseAlignment_ = 16u;
			alignmentCount_ = 3u;
		} else if (baseSize_ == 64u) { // mat4
			baseAlignment_ = 16u;
			alignmentCount_ = 4u;
		} else if (numElements() > 1u && memoryLayout_ == BUFFER_MEMORY_STD140) {
			// with STD140, each array element must be padded to a multiple of 16 bytes
			baseAlignment_ = 16u;
		}
		if (numElements() > 1u) {
			alignedElementSize_ = baseAlignment_ * alignmentCount_;
		}
		alignedInputSize_ = alignedElementSize_ * numElements_ui_;
	}
	vertexStride_ = alignedElementSize_ * numArrayElements_;
	clientBuffer_->setBaseAlignment(baseAlignment_);
}

void ShaderInput::setMemoryLayout(BufferMemoryLayout layout) {
	memoryLayout_ = layout;
	clientBuffer_->setMemoryLayout(layout);
	updateAlignment();
}

void ShaderInput::set_numArrayElements(uint32_t v) {
	if (v == numArrayElements_) {
		return;
	}
	numArrayElements_ = v;
	elementSize_ = dataTypeBytes_ * valsPerElement_ * numArrayElements_;
	if (isVertexAttribute_) {
		numElements_ui_ = numArrayElements_ * numVertices_;
	} else {
		numElements_ui_ = numArrayElements_ * numInstances_;
	}
	numElements_i_ = static_cast<int32_t>(numElements_ui_);
	updateAlignment();
}

void ShaderInput::set_isVertexAttribute(bool isVertexAttribute) {
	isVertexAttribute_ = isVertexAttribute;
	if (isVertexAttribute_) {
		numInstances_ = 1u;
		divisor_ = 0u;
	}
}

void ShaderInput::setMainBuffer(const ref_ptr<BufferReference> &ref, uint32_t offset) {
	buffer_ = ref->bufferID();
	bufferRef_ = ref;
	offset_ = offset;
	bufferStamp_ = stampOfReadData();
}

void ShaderInput::setMainBufferOffset(uint32_t offset) {
	offset_ = offset;
}

void ShaderInput::enableAttribute(int loc) const {
	(this->*(this->enableAttribute_))(loc);
}

void ShaderInput::enableUniform(int loc) const {
	enableInput_(loc);
}

/////////////
/////////////
////////////

void ShaderInput::writeVertex(uint32_t index, const byte *data) {
	// NOTE: it is maybe a bit confusing, but the semantics of writeVertex is currently
	//       different for uniform array data vs vertex data.
	//       For vertex data, it is assumed that data is one vertex including all array elements.
	//       For uniform array data, it is assumed that data is one array element.
	if (isVertexAttribute_) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_WRITE, index * elementSize_, elementSize_);
		std::memcpy(mapped.w, data, elementSize_);
	} else {
		auto arrayElementSize = dataTypeBytes_ * valsPerElement_;
		auto mapped = mapClientDataRaw(BUFFER_GPU_WRITE, index * arrayElementSize, arrayElementSize);
		std::memcpy(mapped.w, data, arrayElementSize);
	}
}

void ShaderInput::deallocateClientData() {
	clientBuffer_->deallocateClientData();
}

void ShaderInput::setUniformUntyped(const byte *data) {
	setInstanceData(1, 1, data);
}

void ShaderInput::updateAlignedSize() {
	// Check if we need to apply padding per element.
	// e.g. in case of STD140, each array element must be padded to a multiple of 16 bytes,
	// so if we have an array of 3 vec3f, the size will be 3 * 16 = 48 bytes,
	// but the unaligned size will be 3 * 12 = 36 bytes.
	if (memoryLayout_ != BUFFER_MEMORY_PACKED &&
			numElements() > 1 &&
			alignedInputSize_ != unalignedSize_) {
		// allocate space in client buffer for aligned data.
		// note: this will make it more difficult to update the data on the client side,
		// but it enables us to form contiguous buffers for the GPU.
		REGEN_INFO("Re-alignment needed for " << name()
			<< "(" << unalignedSize_ << " to " << alignedInputSize_ << ")");
		inputSize_ = alignedInputSize_;
		// use strided data access in mapClient* functions
		mapClientStride_ = alignedElementSize_;
	}
	else {
		// no re-alignment needed
		inputSize_ = unalignedSize_;
	}
}

void ShaderInput::setInstanceData(uint32_t numInstances, uint32_t divisor, const byte *data) {
	numInstances_ = numInstances;
	if (numInstances == 0u) {
		REGEN_WARN("Requested zero instances for input '" << name() << "', setting to one to avoid issues.");
		numInstances_ = 1u;
	}

	auto dataSize_bytes = elementSize_ * numInstances / divisor;
	if (dataSize_bytes != unalignedSize_ || isVertexAttribute_ || !hasClientData()) {
		// size of the data has changed, need to reallocate the data buffer.
		clientBuffer_->writeLockAll();
		isVertexAttribute_ = false;
		divisor_ = std::max(1u, divisor);
		numVertices_ = 1u;
		numElements_ui_ = numArrayElements_ * numInstances_;
		numElements_i_ = static_cast<int32_t>(numElements_ui_);
		unalignedSize_ = dataSize_bytes;
		updateAlignment();
		updateAlignedSize();

		if (!clientBuffer_->resize(alignedInputSize_, data)) {
			REGEN_ERROR("Failed to resize client buffer for " << name()
				<< " to " << alignedInputSize_ << " bytes.");
		}
		clientBuffer_->writeUnlockAll(0u, 0u);
	} else if (data) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(mapped.w, data, dataSize_bytes);
	}
}

void ShaderInput::setVertexData(uint32_t numVertices, const byte *data) {
	auto dataSize_bytes = elementSize_ * numVertices;

	if (dataSize_bytes != unalignedSize_ || !isVertexAttribute_ || !hasClientData()) {
		// size of the data has changed, need to reallocate the data buffer.
		clientBuffer_->writeLockAll();
		isVertexAttribute_ = true;
		numInstances_ = 1u;
		divisor_ = 0u;
		numVertices_ = numVertices;
		numElements_ui_ = numArrayElements_ * numVertices_;
		numElements_i_ = static_cast<int32_t>(numElements_ui_);
		unalignedSize_ = dataSize_bytes;
		updateAlignment();
		updateAlignedSize();

		if (!clientBuffer_->resize(alignedInputSize_, data)) {
			REGEN_ERROR("Failed to resize client buffer for " << name()
				<< " to " << alignedInputSize_ << " bytes.");
		}
		clientBuffer_->writeUnlockAll(0u, 0u);
	} else if (data) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(mapped.w, data, dataSize_bytes);
	}
}

void ShaderInput::writeServerData() const {
	if (!hasClientData() || !hasServerData()) return;
	if (bufferStamp_ == stampOfReadData()) return;
	auto mappedClientData = clientBuffer_->mapRange(BUFFER_GPU_READ, 0, inputSize_);
	auto clientData = mappedClientData.r;
	auto count = std::max(numVertices_, numInstances_);

	if (static_cast<uint32_t>(vertexStride_) == elementSize_) {
		glNamedBufferSubData(buffer_, offset_, inputSize_, clientData);
	} else {
		uint32_t offset = offset_;
		for (uint32_t i = 0; i < count; ++i) {
			glNamedBufferSubData(buffer_, offset, elementSize_, clientData);
			offset += vertexStride_;
			clientData += elementSize_;
		}
	}

	bufferStamp_ = stampOfReadData();
	clientBuffer_->unmapRange(BUFFER_GPU_READ, 0, inputSize_, mappedClientData.r_index);
}

void ShaderInput::readServerData() {
	if (!hasServerData()) return;
	auto mappedClientData = clientBuffer_->mapRange(BUFFER_GPU_WRITE, 0, inputSize_);
	auto clientData = mappedClientData.w;

	byte *serverData = (byte *) glMapNamedBufferRange(
			mainBufferName(),
			offset_,
			numVertices_ * vertexStride_ + elementSize_,
			GL_MAP_READ_BIT);

	if (static_cast<uint32_t>(vertexStride_) == elementSize_) {
		std::memcpy(clientData, serverData, inputSize_);
	} else {
		for (uint32_t i = 0; i < numVertices_; ++i) {
			std::memcpy(clientData, serverData, elementSize_);
			serverData += vertexStride_;
			clientData += elementSize_;
		}
	}

	glUnmapNamedBuffer(mainBufferName());
	clientBuffer_->unmapRange(BUFFER_GPU_WRITE, 0, inputSize_, mappedClientData.w_index);
}

/////////////
/////////////
////////////

ref_ptr<ShaderInput> ShaderInput::create(const ref_ptr<ShaderInput> &in) {
	if (in->isBufferBlock()) {
		if (auto oldBlock = dynamic_cast<BufferBlock *>(in.get())) {
			if (oldBlock->isUBO()) {
				auto newBlock = ref_ptr<UBO>::alloc(in->name(), oldBlock->bufferUpdateHints());
				for (auto &namedInput: oldBlock->stagedInputs()) {
					newBlock->addStagedInput(create(namedInput.in_), namedInput.name_);
				}
				return newBlock;
			}
			if (oldBlock->isSSBO()) {
				auto newBlock = ref_ptr<SSBO>::alloc(in->name(), oldBlock->bufferUpdateHints());
				for (auto &namedInput: oldBlock->stagedInputs()) {
					newBlock->addStagedInput(create(namedInput.in_), namedInput.name_);
				}
				return newBlock;
			}
		}
		REGEN_WARN("Unknown BufferBlock type for ShaderInput::create: " << in->name());
		return {};
	}

	const std::string &name = in->name();
	GLenum baseType = in->baseType();
	uint32_t valsPerElement = in->valsPerElement();

	switch (baseType) {
		case GL_FLOAT:
			switch (valsPerElement) {
				case 16:
					return ref_ptr<ShaderInputMat4>::alloc(name, in->numArrayElements(), in->normalize());
				case 9:
					return ref_ptr<ShaderInputMat3>::alloc(name, in->numArrayElements(), in->normalize());
				case 4:
					return ref_ptr<ShaderInput4f>::alloc(name, in->numArrayElements(), in->normalize());
				case 3:
					return ref_ptr<ShaderInput3f>::alloc(name, in->numArrayElements(), in->normalize());
				case 2:
					return ref_ptr<ShaderInput2f>::alloc(name, in->numArrayElements(), in->normalize());
				default:
					return ref_ptr<ShaderInput1f>::alloc(name, in->numArrayElements(), in->normalize());
			}
		case GL_DOUBLE:
			switch (valsPerElement) {
				case 4:
					return ref_ptr<ShaderInput4d>::alloc(name, in->numArrayElements(), in->normalize());
				case 3:
					return ref_ptr<ShaderInput3d>::alloc(name, in->numArrayElements(), in->normalize());
				case 2:
					return ref_ptr<ShaderInput2d>::alloc(name, in->numArrayElements(), in->normalize());
				default:
					return ref_ptr<ShaderInput1d>::alloc(name, in->numArrayElements(), in->normalize());
			}
		case GL_BOOL:
		case GL_INT:
			switch (valsPerElement) {
				case 4:
					return ref_ptr<ShaderInput4i>::alloc(name, in->numArrayElements(), in->normalize());
				case 3:
					return ref_ptr<ShaderInput3i>::alloc(name, in->numArrayElements(), in->normalize());
				case 2:
					return ref_ptr<ShaderInput2i>::alloc(name, in->numArrayElements(), in->normalize());
				default:
					return ref_ptr<ShaderInput1i>::alloc(name, in->numArrayElements(), in->normalize());
			}
		case GL_UNSIGNED_INT:
			switch (valsPerElement) {
				case 4:
					return ref_ptr<ShaderInput4ui>::alloc(name, in->numArrayElements(), in->normalize());
				case 3:
					return ref_ptr<ShaderInput3ui>::alloc(name, in->numArrayElements(), in->normalize());
				case 2:
					return ref_ptr<ShaderInput2ui>::alloc(name, in->numArrayElements(), in->normalize());
				default:
					return ref_ptr<ShaderInput1ui>::alloc(name, in->numArrayElements(), in->normalize());
			}
		default:
			REGEN_WARN("Unknown shader input type: " << glenum::glslDataType(baseType, valsPerElement));
			return {};
	}
}

ref_ptr<ShaderInput> ShaderInput::copy(const ref_ptr<ShaderInput> &in, bool copyData) {
	ref_ptr<ShaderInput> cp = create(in);
	cp->vertexStride_ = in->vertexStride_;
	cp->offset_ = in->offset_;
	cp->inputSize_ = in->inputSize_;
	cp->elementSize_ = in->elementSize_;
	cp->numArrayElements_ = in->numArrayElements_;
	cp->numVertices_ = in->numVertices_;
	cp->numInstances_ = in->numInstances_;
	cp->divisor_ = in->divisor_;
	cp->buffer_ = 0;
	cp->bufferStamp_ = 0;
	cp->normalize_ = in->normalize_;
	cp->isVertexAttribute_ = in->isVertexAttribute_;
	cp->isBufferBlock_ = in->isBufferBlock_;
	cp->isStagedBuffer_ = in->isStagedBuffer_;
	cp->isConstant_ = in->isConstant_;
	cp->transpose_ = in->transpose_;
	cp->forceArray_ = in->forceArray_;
	cp->schema_ = in->schema_;
	cp->setMemoryLayout(in->memoryLayout_);
	if (in->hasClientData()) {
		// allocate memory for one slot, copy most recent data
		if (copyData) {
			auto mapped = in->mapClientDataRaw(BUFFER_GPU_READ);
			if (in->isVertexAttribute()) {
				cp->setVertexData(in->numVertices_, mapped.r);
			} else {
				cp->setInstanceData(in->numInstances_, in->divisor_, mapped.r);
			}
		} else {
			if (!cp->clientBuffer_->resize(in->alignedInputSize_)) {
				REGEN_ERROR("Failed to allocate client data for " << in->name());
			}
		}
	}
	return cp;
}


/////////////
/////////////
/////////////

void ShaderInput::enableAttribute_f(int location) const {
	for (unsigned int i = 0; i < numArrayElements_; ++i) {
		auto loc = location + i;
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(
				loc,
				valsPerElement_,
				baseType_,
				normalize_,
				vertexStride_,
				REGEN_BUFFER_OFFSET(offset_));
		if (divisor_ != 0) {
			glVertexAttribDivisor(loc, divisor_);
		}
	}
}

void ShaderInput::enableAttribute_i(int location) const {
	for (unsigned int i = 0; i < numArrayElements_; ++i) {
		auto loc = location + i;
		glEnableVertexAttribArray(loc);
		// use glVertexAttribIPointer, otherwise OpenGL
		// would convert integers to float
		glVertexAttribIPointer(
				loc,
				valsPerElement_,
				baseType_,
				vertexStride_,
				REGEN_BUFFER_OFFSET(offset_));
		if (divisor_ != 0) {
			glVertexAttribDivisor(loc, divisor_);
		}
	}
}

void ShaderInput::enableAttributeMat4(int location) const {
	for (unsigned int i = 0; i < numArrayElements_ * 4; i += 4) {
		auto loc0 = location + i;
		auto loc1 = location + i + 1;
		auto loc2 = location + i + 2;
		auto loc3 = location + i + 3;

		glEnableVertexAttribArray(loc0);
		glEnableVertexAttribArray(loc1);
		glEnableVertexAttribArray(loc2);
		glEnableVertexAttribArray(loc3);

		glVertexAttribPointer(loc0,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_ + sizeof(float) * 4));
		glVertexAttribPointer(loc2,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_ + sizeof(float) * 8));
		glVertexAttribPointer(loc3,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_ + sizeof(float) * 12));

		if (divisor_ != 0) {
			glVertexAttribDivisor(loc0, divisor_);
			glVertexAttribDivisor(loc1, divisor_);
			glVertexAttribDivisor(loc2, divisor_);
			glVertexAttribDivisor(loc3, divisor_);
		}
	}
}

void ShaderInput::enableAttributeMat3(int location) const {
	for (unsigned int i = 0; i < numArrayElements_ * 3; i += 4) {
		auto loc0 = location + i;
		auto loc1 = location + i + 1;
		auto loc2 = location + i + 2;

		glEnableVertexAttribArray(loc0);
		glEnableVertexAttribArray(loc1);
		glEnableVertexAttribArray(loc2);

		glVertexAttribPointer(loc0,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_ + sizeof(float) * 4));
		glVertexAttribPointer(loc2,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_ + sizeof(float) * 8));

		if (divisor_ != 0) {
			glVertexAttribDivisor(loc0, divisor_);
			glVertexAttribDivisor(loc1, divisor_);
			glVertexAttribDivisor(loc2, divisor_);
		}
	}
}

void ShaderInput::enableAttributeMat2(int location) const {
	for (unsigned int i = 0; i < numArrayElements_ * 2; i += 4) {
		auto loc0 = location + i;
		auto loc1 = location + i + 1;

		glEnableVertexAttribArray(loc0);
		glEnableVertexAttribArray(loc1);

		glVertexAttribPointer(loc0,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, baseType_, normalize_, vertexStride_,
							  REGEN_BUFFER_OFFSET(offset_ + sizeof(float) * 4));

		if (divisor_ != 0) {
			glVertexAttribDivisor(loc0, divisor_);
			glVertexAttribDivisor(loc1, divisor_);
		}
	}
}

/////////////
/////////////
/////////////

ShaderInput1f::ShaderInput1f(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform1fv(loc, numElements_i_, (float *) mapped.r);
	};
}

ShaderInput2f::ShaderInput2f(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform2fv(loc, numElements_i_, (float *) mapped.r);
	};
}

ShaderInput3f::ShaderInput3f(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform3fv(loc, numElements_i_, (float *) mapped.r);
	};
}

ShaderInput4f::ShaderInput4f(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform4fv(loc, numElements_i_, (float *) mapped.r);
	};
}

ShaderInputMat3::ShaderInputMat3(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	transpose_ = false;
	enableAttribute_ = &ShaderInput::enableAttributeMat3;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniformMatrix3fv(loc, numElements_i_, transpose_, (float *) mapped.r);
	};
}

ShaderInputMat4::ShaderInputMat4(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	transpose_ = false;
	enableAttribute_ = &ShaderInput::enableAttributeMat4;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniformMatrix4fv(loc, numElements_i_, transpose_, (float *) mapped.r);
	};
}

ShaderInput1d::ShaderInput1d(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform1dv(loc, numElements_i_, (double *) mapped.r);
	};
}

ShaderInput2d::ShaderInput2d(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform2dv(loc, numElements_i_, (double *) mapped.r);
	};
}

ShaderInput3d::ShaderInput3d(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform3dv(loc, numElements_i_, (double *) mapped.r);
	};
}

ShaderInput4d::ShaderInput4d(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform4dv(loc, numElements_i_, (double *) mapped.r);
	};
}

ShaderInput1i::ShaderInput1i(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform1iv(loc, numElements_i_, (int *) mapped.r);
	};
}

ShaderInput2i::ShaderInput2i(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform2iv(loc, numElements_i_, (int *) mapped.r);
	};
}

ShaderInput3i::ShaderInput3i(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform3iv(loc, numElements_i_, (int *) mapped.r);
	};
}

ShaderInput4i::ShaderInput4i(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform4iv(loc, numElements_i_, (int *) mapped.r);
	};
}

ShaderInput1ui::ShaderInput1ui(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform1uiv(loc, numElements_i_, (unsigned int *) mapped.r);
	};
}

ShaderInput1ui_16::ShaderInput1ui_16(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform1uiv(loc, numElements_i_, (unsigned int *) mapped.r);
	};
}

ShaderInput1ui_8::ShaderInput1ui_8(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform1uiv(loc, numElements_i_, (unsigned int *) mapped.r);
	};
}

ShaderInput2ui::ShaderInput2ui(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform2uiv(loc, numElements_i_, (unsigned int *) mapped.r);
	};
}

ShaderInput3ui::ShaderInput3ui(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform3uiv(loc, numElements_i_, (unsigned int *) mapped.r);
	};
}

ShaderInput4ui::ShaderInput4ui(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](int loc) {
		auto mapped = mapClientDataRaw(BUFFER_GPU_READ);
		glUniform4uiv(loc, numElements_i_, (unsigned int *) mapped.r);
	};
}

ref_ptr<ShaderInput> regen::createIndexInput(uint32_t numIndices, uint32_t maxVertexIdx) {
	ref_ptr<ShaderInput> indices;
	if (maxVertexIdx < 256) {
		indices = ref_ptr<ShaderInput1ui_8>::alloc("i");
	} else if (maxVertexIdx < 65536) {
		indices = ref_ptr<ShaderInput1ui_16>::alloc("i");
	} else {
		indices = ref_ptr<ShaderInput1ui_32>::alloc("i");
	}
	indices->setVertexData(numIndices);
	return indices;
}

void regen::setIndexValue(byte *data, GLenum t, uint32_t i, uint32_t v) {
	if (t == GL_UNSIGNED_BYTE) {
		((uint8_t *) data)[i] = (uint8_t) v;
	} else if (t == GL_UNSIGNED_SHORT) {
		((uint16_t *) data)[i] = (uint16_t) v;
	} else {
		((uint32_t *) data)[i] = (uint32_t) v;
	}
}
