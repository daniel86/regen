#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/animations/animation.h>
#include <regen/buffer/ssbo.h>
#include <stack>

#include "shader-input.h"
#include "regen/scene/mesh-processor.h"

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

using namespace regen;

NamedShaderInput::NamedShaderInput(const ref_ptr<ShaderInput> &in,
								   const std::string &name,
								   const std::string &type)
		: in_(in), name_(name), type_(type) {
	if (name_.empty()) {
		name_ = in->name();
	}
	if (type_.empty()) {
		type_ = glenum::glslDataType(in->baseType(), in->valsPerElement());
	}
}

ShaderInput::ShaderInput(
		const std::string &name,
		GLenum baseType,
		uint32_t dataTypeBytes,
		int32_t valsPerElement,
		uint32_t numArrayElements,
		bool normalize)
		: name_(name),
		  baseType_(baseType),
		  dataTypeBytes_(dataTypeBytes),
		  stride_(0),
		  offset_(0),
		  numArrayElements_(numArrayElements),
		  numVertices_(1u),
		  numInstances_(1u),
		  numElements_i_(static_cast<int32_t>(numArrayElements)),
		  numElements_ui_(numArrayElements),
		  valsPerElement_(valsPerElement),
		  divisor_(0),
		  buffer_(0),
		  bufferStamp_(0),
		  normalize_(normalize),
		  isVertexAttribute_(false),
		  transpose_(false),
		  isConstant_(false),
		  isBufferBlock_(false),
		  forceArray_(false),
		  active_(true) {
	elementSize_ = dataTypeBytes_ * valsPerElement_ * numArrayElements_;
	enableAttribute_ = &ShaderInput::enableAttribute_f;
}

ShaderInput::ShaderInput(const ShaderInput &o)
		: name_(o.name_),
		  baseType_(o.baseType_),
		  dataTypeBytes_(o.dataTypeBytes_),
		  stride_(o.stride_),
		  offset_(o.offset_),
		  inputSize_(o.inputSize_),
		  elementSize_(o.elementSize_),
		  numArrayElements_(o.numArrayElements_),
		  numVertices_(o.numVertices_),
		  numInstances_(o.numInstances_),
		  numElements_i_(o.numElements_i_),
		  numElements_ui_(o.numElements_ui_),
		  valsPerElement_(o.valsPerElement_),
		  divisor_(o.divisor_),
		  buffer_(o.buffer_),
		  bufferStamp_(o.bufferStamp_),
		  normalize_(o.normalize_),
		  isVertexAttribute_(o.isVertexAttribute_),
		  transpose_(o.transpose_),
		  isConstant_(o.isConstant_),
		  isBufferBlock_(o.isBufferBlock_),
		  forceArray_(o.forceArray_),
		  active_(o.active_),
		  schema_(o.schema_) {
	enableAttribute_ = &ShaderInput::enableAttribute_f;
	enableInput_ = o.enableInput_;
	// copy client data, if any
	if (o.hasClientData()) {
		auto mapped = o.clientBuffer_.mapClientData(ShaderData::READ);
		clientBuffer_.reallocateClientData(inputSize_, elementSize_, mapped.r);
	}
}

ShaderInput::~ShaderInput() {
	if (bufferIterator_.get()) {
		BufferObject::orphanBufferRange(bufferIterator_.get());
	}
	deallocateClientData();
}

GLenum ShaderInput::dataType() const {
	return glenum::dataType(baseType_, valsPerElement_);
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
	nextStamp();
}

void ShaderInput::set_isVertexAttribute(bool isVertexAttribute) {
	isVertexAttribute_ = isVertexAttribute;
	if (isVertexAttribute_) {
		numInstances_ = 1u;
		divisor_ = 0u;
	}
}

void ShaderInput::set_buffer(GLuint buffer, const ref_ptr<BufferReference> &it) {
	buffer_ = buffer;
	// FIXME: remove this
	clientBuffer_.setHasServerData(true);
	bufferIterator_ = it;
	bufferStamp_ = stamp();
}

void ShaderInput::enableAttribute(GLint loc) const {
	if (clientBuffer_.requiresReUpload()) {
		writeServerData();
		clientBuffer_.setRequiresReUpload(false);
	}
	(this->*(this->enableAttribute_))(loc);
}

void ShaderInput::enableUniform(GLint loc) const {
	enableInput_(loc);
}

/////////////
/////////////
////////////

void ShaderInput::writeVertex(GLuint index, const byte *data) {
	auto mapped = mapClientDataRaw(ShaderData::WRITE | ShaderData::INDEX);
	// NOTE: it is maybe a bit confusing, but the semantics of writeVertex is currently
	//       different for uniform array data vs vertex data.
	//       For vertex data, it is assumed that data is one vertex including all array elements.
	//       For uniform array data, it is assumed that data is one array element.
	if (isVertexAttribute_) {
		std::memcpy(mapped.w + index * elementSize_, data, elementSize_);
	} else {
		auto arrayElementSize = dataTypeBytes_ * valsPerElement_;
		std::memcpy(mapped.w + index * arrayElementSize, data, arrayElementSize);
	}
}

void ShaderInput::deallocateClientData() {
	clientBuffer_.deallocateClientData();
}

void ShaderInput::setUniformUntyped(const byte *data) {
	setInstanceData(1, 1, data);
}

void ShaderInput::setInstanceData(GLuint numInstances, GLuint divisor, const byte *data) {
	auto dataSize_bytes = elementSize_ * numInstances / divisor;

	if (dataSize_bytes != inputSize_ || isVertexAttribute_ || !hasClientData()) {
		// size of the data has changed, need to reallocate the data buffer.
		clientBuffer_.writeLockAll();
		clientBuffer_.reallocateClientData(dataSize_bytes, elementSize_);
		isVertexAttribute_ = false;
		numInstances_ = std::max(1u, numInstances);
		divisor_ = std::max(1u, divisor);
		numVertices_ = 1u;
		numElements_ui_ = numArrayElements_ * numInstances_;
		numElements_i_ = static_cast<int32_t>(numElements_ui_);
		inputSize_ = dataSize_bytes;
		clientBuffer_.writeUnlockAll(clientBuffer_.writeClientData_(data));
	} else if (data) {
		auto mapped = mapClientDataRaw(ShaderData::WRITE);
		std::memcpy(mapped.w, data, dataSize_bytes);
	}
}

void ShaderInput::setVertexData(GLuint numVertices, const byte *data) {
	auto dataSize_bytes = elementSize_ * numVertices;

	if (dataSize_bytes != inputSize_ || !isVertexAttribute_ || !hasClientData()) {
		// size of the data has changed, need to reallocate the data buffer.
		clientBuffer_.writeLockAll();
		clientBuffer_.reallocateClientData(dataSize_bytes, elementSize_);
		isVertexAttribute_ = true;
		numInstances_ = 1u;
		divisor_ = 0u;
		numVertices_ = numVertices;
		numElements_ui_ = numArrayElements_ * numVertices_;
		numElements_i_ = static_cast<int32_t>(numElements_ui_);
		inputSize_ = dataSize_bytes;
		clientBuffer_.writeUnlockAll(clientBuffer_.writeClientData_(data));
	} else if (data) {
		auto mapped = mapClientDataRaw(ShaderData::WRITE);
		std::memcpy(mapped.w, data, dataSize_bytes);
	}
}

void ShaderInput::writeServerData(GLuint index) const {
	if (!hasClientData() || !hasServerData()) return;
	auto mappedClientData = clientBuffer_.mapClientData(ShaderData::READ);
	auto clientData = mappedClientData.r;
	auto subDataStart = clientData + elementSize_ * index;
	glNamedBufferSubData(
			buffer_,
			offset_ + stride_ * index,
			elementSize_,
			subDataStart);
}

void ShaderInput::writeServerData() const {
	if (!hasClientData() || !hasServerData()) return;
	if (bufferStamp_ == stamp()) return;
	auto mappedClientData = clientBuffer_.mapClientData(ShaderData::READ);
	auto clientData = mappedClientData.r;
	auto count = std::max(numVertices_, numInstances_);

	if (static_cast<uint32_t>(stride_) == elementSize_) {
		glNamedBufferSubData(buffer_, offset_, inputSize_, clientData);
	} else {
		GLuint offset = offset_;
		for (GLuint i = 0; i < count; ++i) {
			glNamedBufferSubData(buffer_, offset, elementSize_, clientData);
			offset += stride_;
			clientData += elementSize_;
		}
	}

	bufferStamp_ = stamp();
}

void ShaderInput::readServerData() {
	if (!hasServerData()) return;
	auto mappedClientData = clientBuffer_.mapClientData(ShaderData::WRITE);
	auto clientData = mappedClientData.w;

	byte *serverData = (byte *) glMapNamedBufferRange(
			buffer(),
			offset_,
			numVertices_ * stride_ + elementSize_,
			GL_MAP_READ_BIT);

	if (static_cast<uint32_t>(stride_) == elementSize_) {
		std::memcpy(clientData, serverData, inputSize_);
	} else {
		for (GLuint i = 0; i < numVertices_; ++i) {
			std::memcpy(clientData, serverData, elementSize_);
			serverData += stride_;
			clientData += elementSize_;
		}
	}

	glUnmapNamedBuffer(buffer());
}

/////////////
/////////////
////////////

ref_ptr<ShaderInput> ShaderInput::create(const ref_ptr<ShaderInput> &in) {
	if (in->isBufferBlock()) {
		auto oldBlock = dynamic_cast<BufferBlock *>(in.get());
		if (oldBlock->isUBO()) {
			auto newBlock = ref_ptr<UBO>::alloc(in->name(), oldBlock->bufferUpdateHints());
			for (auto &namedInput: oldBlock->blockInputs()) {
				newBlock->addBlockInput(create(namedInput.in_), namedInput.name_);
			}
			return newBlock;
		}
		if (oldBlock->isSSBO()) {
			auto newBlock = ref_ptr<SSBO>::alloc(in->name(), oldBlock->bufferUpdateHints());
			for (auto &namedInput: oldBlock->blockInputs()) {
				newBlock->addBlockInput(create(namedInput.in_), namedInput.name_);
			}
			return newBlock;
		}
	}

	const std::string &name = in->name();
	GLenum baseType = in->baseType();
	GLuint valsPerElement = in->valsPerElement();

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
	cp->stride_ = in->stride_;
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
	cp->isConstant_ = in->isConstant_;
	cp->transpose_ = in->transpose_;
	cp->forceArray_ = in->forceArray_;
	cp->schema_ = in->schema_;
	if (in->hasClientData()) {
		// allocate memory for one slot, copy most recent data
		if (copyData) {
			auto mapped = in->clientBuffer_.mapClientData(ShaderData::READ);
			cp->clientBuffer_.reallocateClientData(in->inputSize_, in->elementSize_, mapped.r);
		} else {
			cp->clientBuffer_.reallocateClientData(in->inputSize_, in->elementSize_);
		}
	}
	return cp;
}


/////////////
/////////////
/////////////

void ShaderInput::enableAttribute_f(GLint location) const {
	for (unsigned int i = 0; i < numArrayElements_; ++i) {
		auto loc = location + i;
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(
				loc,
				valsPerElement_,
				baseType_,
				normalize_,
				stride_,
				BUFFER_OFFSET(offset_));
		if (divisor_ != 0) {
			glVertexAttribDivisor(loc, divisor_);
		}
	}
}

void ShaderInput::enableAttribute_i(GLint location) const {
	for (unsigned int i = 0; i < numArrayElements_; ++i) {
		auto loc = location + i;
		glEnableVertexAttribArray(loc);
		// use glVertexAttribIPointer, otherwise OpenGL
		// would convert integers to float
		glVertexAttribIPointer(
				loc,
				valsPerElement_,
				baseType_,
				stride_,
				BUFFER_OFFSET(offset_));
		if (divisor_ != 0) {
			glVertexAttribDivisor(loc, divisor_);
		}
	}
}

void ShaderInput::enableAttributeMat4(GLint location) const {
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
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 4));
		glVertexAttribPointer(loc2,
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 8));
		glVertexAttribPointer(loc3,
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 12));

		if (divisor_ != 0) {
			glVertexAttribDivisor(loc0, divisor_);
			glVertexAttribDivisor(loc1, divisor_);
			glVertexAttribDivisor(loc2, divisor_);
			glVertexAttribDivisor(loc3, divisor_);
		}
	}
}

void ShaderInput::enableAttributeMat3(GLint location) const {
	for (unsigned int i = 0; i < numArrayElements_ * 3; i += 4) {
		auto loc0 = location + i;
		auto loc1 = location + i + 1;
		auto loc2 = location + i + 2;

		glEnableVertexAttribArray(loc0);
		glEnableVertexAttribArray(loc1);
		glEnableVertexAttribArray(loc2);

		glVertexAttribPointer(loc0,
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 4));
		glVertexAttribPointer(loc2,
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 8));

		if (divisor_ != 0) {
			glVertexAttribDivisor(loc0, divisor_);
			glVertexAttribDivisor(loc1, divisor_);
			glVertexAttribDivisor(loc2, divisor_);
		}
	}
}

void ShaderInput::enableAttributeMat2(GLint location) const {
	for (unsigned int i = 0; i < numArrayElements_ * 2; i += 4) {
		auto loc0 = location + i;
		auto loc1 = location + i + 1;

		glEnableVertexAttribArray(loc0);
		glEnableVertexAttribArray(loc1);

		glVertexAttribPointer(loc0,
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, baseType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 4));

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
	enableInput_ = [this](GLint loc) {
		glUniform1fv(loc, numElements_i_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInput2f::ShaderInput2f(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](GLint loc) {
		glUniform2fv(loc, numElements_i_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInput3f::ShaderInput3f(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](GLint loc) {
		glUniform3fv(loc, numElements_i_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInput4f::ShaderInput4f(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](GLint loc) {
		glUniform4fv(loc, numElements_i_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInputMat3::ShaderInputMat3(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	transpose_ = GL_FALSE;
	enableAttribute_ = &ShaderInput::enableAttributeMat3;
	enableInput_ = [this](GLint loc) {
		glUniformMatrix3fv(loc, numElements_i_, transpose_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInputMat4::ShaderInputMat4(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	transpose_ = GL_FALSE;
	enableAttribute_ = &ShaderInput::enableAttributeMat4;
	enableInput_ = [this](GLint loc) {
		glUniformMatrix4fv(loc, numElements_i_, transpose_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInput1d::ShaderInput1d(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](GLint loc) {
		glUniform1dv(loc, numElements_i_, mapClientData<double>(ShaderData::READ).r);
	};
}

ShaderInput2d::ShaderInput2d(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](GLint loc) {
		glUniform2dv(loc, numElements_i_, mapClientData<double>(ShaderData::READ).r);
	};
}

ShaderInput3d::ShaderInput3d(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](GLint loc) {
		glUniform3dv(loc, numElements_i_, mapClientData<double>(ShaderData::READ).r);
	};
}

ShaderInput4d::ShaderInput4d(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableInput_ = [this](GLint loc) {
		glUniform4dv(loc, numElements_i_, mapClientData<double>(ShaderData::READ).r);
	};
}

ShaderInput1i::ShaderInput1i(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](GLint loc) {
		glUniform1iv(loc, numElements_i_, mapClientData<int>(ShaderData::READ).r);
	};
}

ShaderInput2i::ShaderInput2i(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](GLint loc) {
		glUniform2iv(loc, numElements_i_, mapClientData<int>(ShaderData::READ).r);
	};
}

ShaderInput3i::ShaderInput3i(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](GLint loc) {
		glUniform3iv(loc, numElements_i_, mapClientData<int>(ShaderData::READ).r);
	};
}

ShaderInput4i::ShaderInput4i(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](GLint loc) {
		glUniform4iv(loc, numElements_i_, mapClientData<int>(ShaderData::READ).r);
	};
}

ShaderInput1ui::ShaderInput1ui(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](GLint loc) {
		glUniform1uiv(loc, numElements_i_, mapClientData<unsigned int>(ShaderData::READ).r);
	};
}

ShaderInput2ui::ShaderInput2ui(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](GLint loc) {
		glUniform2uiv(loc, numElements_i_, mapClientData<unsigned int>(ShaderData::READ).r);
	};
}

ShaderInput3ui::ShaderInput3ui(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](GLint loc) {
		glUniform3uiv(loc, numElements_i_, mapClientData<unsigned int>(ShaderData::READ).r);
	};
}

ShaderInput4ui::ShaderInput4ui(
		const std::string &name,
		uint32_t numArrayElements,
		bool normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttribute_i;
	enableInput_ = [this](GLint loc) {
		glUniform4uiv(loc, numElements_i_, mapClientData<unsigned int>(ShaderData::READ).r);
	};
}
