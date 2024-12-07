/*
 * shader-input.cpp
 *
 *  Created on: 15.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/animations/animation.h>

#include "shader-input.h"
#include "ubo.h"

using namespace regen;

/**
 * Writes attribute to GL server on invokation.
 */
class DataUploadAnimation : public Animation {
public:
	/**
	 * @param input Pointer to attribute.
	 */
	explicit DataUploadAnimation(ShaderInput *input)
			: Animation(GL_TRUE, GL_FALSE, GL_FALSE),
			  input_(input) {}

	// Override
	void glAnimate(RenderState *rs, GLdouble dt) override {
		input_->writeServerData(rs);
		stopAnimation();
	}

protected:
	ShaderInput *input_;
};

NamedShaderInput::NamedShaderInput(const ref_ptr<ShaderInput> &in,
						 const std::string &name,
						 const std::string &type)
				: in_(in), name_(name), type_(type) {
	if (name_.empty()) {
		name_ = in->name();
	}
	if (type_.empty()) {
		type_ = glenum::glslDataType(in->dataType(), in->valsPerElement());
	}
}

ShaderInput::ShaderInput(
		const std::string &name,
		GLenum dataType,
		GLuint dataTypeBytes,
		GLuint valsPerElement,
		GLuint elementCount,
		GLboolean normalize)
		: name_(name),
		  dataType_(dataType),
		  dataTypeBytes_(dataTypeBytes),
		  stride_(0),
		  offset_(0),
		  inputSize_(0),
		  elementSize_(0),
		  elementCount_(elementCount),
		  numVertices_(0u),
		  numInstances_(0u),
		  valsPerElement_(valsPerElement),
		  divisor_(0),
		  buffer_(0),
		  bufferStamp_(0),
		  normalize_(normalize),
		  isVertexAttribute_(GL_TRUE),
		  transpose_(GL_FALSE),
		  data_(nullptr),
		  stamp_(1u),
		  isConstant_(GL_FALSE),
		  isUniformBlock_(GL_FALSE),
		  forceArray_(GL_FALSE),
		  active_(GL_TRUE) {
	elementSize_ = dataTypeBytes_ * valsPerElement_ * elementCount_;
	// make data_ stack root
	dataStack_.push(data_);
	enableAttribute_ = &ShaderInput::enableAttributef;
	dataUpload_ = ref_ptr<DataUploadAnimation>::alloc(this);
}

ShaderInput::ShaderInput(const ShaderInput &o)
		: name_(o.name_),
		  dataType_(o.dataType_),
		  dataTypeBytes_(o.dataTypeBytes_),
		  stride_(o.stride_),
		  offset_(o.offset_),
		  inputSize_(o.inputSize_),
		  elementSize_(o.elementSize_),
		  elementCount_(o.elementCount_),
		  numVertices_(o.numVertices_),
		  numInstances_(o.numInstances_),
		  valsPerElement_(o.valsPerElement_),
		  divisor_(o.divisor_),
		  buffer_(o.buffer_),
		  bufferStamp_(o.bufferStamp_),
		  normalize_(o.normalize_),
		  isVertexAttribute_(o.isVertexAttribute_),
		  transpose_(o.transpose_),
		  data_(o.data_),
		  stamp_(o.stamp_),
		  isConstant_(o.isConstant_),
		  isUniformBlock_(o.isUniformBlock_),
		  forceArray_(o.forceArray_),
		  active_(o.active_) {
	elementSize_ = dataTypeBytes_ * valsPerElement_ * elementCount_;
	// make data_ stack root
	dataStack_.push(data_);
	enableAttribute_ = &ShaderInput::enableAttributef;
	enableUniform_ = o.enableUniform_;
	dataUpload_ = ref_ptr<DataUploadAnimation>::alloc(this);
}

ShaderInput::~ShaderInput() {
	if (bufferIterator_.get()) {
		VBO::free(bufferIterator_.get());
	}
	deallocateClientData();
}

GLenum ShaderInput::dataType() const { return dataType_; }

GLuint ShaderInput::dataTypeBytes() const { return dataTypeBytes_; }

GLboolean ShaderInput::active() const { return active_; }

void ShaderInput::set_active(GLboolean v) { active_ = v; }

GLuint ShaderInput::numInstances() const { return numInstances_; }

GLuint ShaderInput::numVertices() const { return numVertices_; }

void ShaderInput::set_numVertices(GLuint numVertices) { numVertices_ = numVertices; }

GLboolean ShaderInput::isVertexAttribute() const { return isVertexAttribute_; }

void ShaderInput::set_isConstant(GLboolean isConstant) { isConstant_ = isConstant; }

GLboolean ShaderInput::isConstant() const { return isConstant_; }

void ShaderInput::set_forceArray(GLboolean forceArray) { forceArray_ = forceArray; }

GLboolean ShaderInput::forceArray() const { return forceArray_; }

GLuint ShaderInput::stamp() const { return stamp_; }

void ShaderInput::nextStamp() {
	stamp_ += 1;
	if (hasServerData()) {
		dataUpload_->startAnimation();
	}
}

void ShaderInput::set_stride(GLuint stride) { stride_ = stride; }

GLuint ShaderInput::stride() const { return stride_; }

void ShaderInput::set_offset(GLuint offset) { offset_ = offset; }

GLuint ShaderInput::offset() const { return offset_; }

GLuint ShaderInput::divisor() const { return divisor_; }

GLuint ShaderInput::inputSize() const { return inputSize_; }

void ShaderInput::set_inputSize(GLuint size) { inputSize_ = size; }

GLuint ShaderInput::elementSize() const { return elementSize_; }

void ShaderInput::set_elementCount(GLuint v) {
	elementCount_ = v;
	elementSize_ = dataTypeBytes_ * valsPerElement_ * elementCount_;
}

GLuint ShaderInput::elementCount() const { return elementCount_; }

GLuint ShaderInput::valsPerElement() const { return valsPerElement_; }

GLboolean ShaderInput::normalize() const { return normalize_; }

void ShaderInput::set_transpose(GLboolean transpose) { transpose_ = transpose; }

GLboolean ShaderInput::transpose() const { return transpose_; }

void ShaderInput::set_buffer(GLuint buffer, const VBOReference &it) {
	buffer_ = buffer;
	bufferIterator_ = it;
	bufferStamp_ = stamp_;
}

GLuint ShaderInput::buffer() const { return buffer_; }

GLuint ShaderInput::bufferStamp() const { return bufferStamp_; }

ref_ptr<VBO::Reference> ShaderInput::bufferIterator() { return bufferIterator_; }

/////////////
/////////////
/////////////

void ShaderInput::enableAttributef(GLint location) const {
	for (GLuint i = 0; i < elementCount_; ++i) {
		GLint loc = location + i;
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(
				loc,
				valsPerElement_,
				dataType_,
				normalize_,
				stride_,
				BUFFER_OFFSET(offset_));
		if (divisor_ != 0) {
			glVertexAttribDivisor(loc, divisor_);
		}
	}
}

void ShaderInput::enableAttributei(GLint location) const {
	for (GLuint i = 0; i < elementCount_; ++i) {
		GLint loc = location + i;
		glEnableVertexAttribArray(loc);
		// use glVertexAttribIPointer, otherwise OpenGL
		// would convert integers to float
		glVertexAttribIPointer(
				loc,
				valsPerElement_,
				dataType_,
				stride_,
				BUFFER_OFFSET(offset_));
		if (divisor_ != 0) {
			glVertexAttribDivisor(loc, divisor_);
		}
	}
}

void ShaderInput::enableAttributeMat4(GLint location) const {
	for (GLuint i = 0; i < elementCount_ * 4; i += 4) {
		GLint loc0 = location + i;
		GLint loc1 = location + i + 1;
		GLint loc2 = location + i + 2;
		GLint loc3 = location + i + 3;

		glEnableVertexAttribArray(loc0);
		glEnableVertexAttribArray(loc1);
		glEnableVertexAttribArray(loc2);
		glEnableVertexAttribArray(loc3);

		glVertexAttribPointer(loc0,
							  4, dataType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, dataType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 4));
		glVertexAttribPointer(loc2,
							  4, dataType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 8));
		glVertexAttribPointer(loc3,
							  4, dataType_, normalize_, stride_,
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
	for (GLuint i = 0; i < elementCount_ * 3; i += 4) {
		GLint loc0 = location + i;
		GLint loc1 = location + i + 1;
		GLint loc2 = location + i + 2;

		glEnableVertexAttribArray(loc0);
		glEnableVertexAttribArray(loc1);
		glEnableVertexAttribArray(loc2);

		glVertexAttribPointer(loc0,
							  4, dataType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, dataType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 4));
		glVertexAttribPointer(loc2,
							  4, dataType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 8));

		if (divisor_ != 0) {
			glVertexAttribDivisor(loc0, divisor_);
			glVertexAttribDivisor(loc1, divisor_);
			glVertexAttribDivisor(loc2, divisor_);
		}
	}
}

void ShaderInput::enableAttributeMat2(GLint location) const {
	for (GLuint i = 0; i < elementCount_ * 2; i += 4) {
		GLint loc0 = location + i;
		GLint loc1 = location + i + 1;

		glEnableVertexAttribArray(loc0);
		glEnableVertexAttribArray(loc1);

		glVertexAttribPointer(loc0,
							  4, dataType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_));
		glVertexAttribPointer(loc1,
							  4, dataType_, normalize_, stride_,
							  BUFFER_OFFSET(offset_ + sizeof(float) * 4));

		if (divisor_ != 0) {
			glVertexAttribDivisor(loc0, divisor_);
			glVertexAttribDivisor(loc1, divisor_);
		}
	}
}

void ShaderInput::enableAttribute(GLint loc) const {
	(this->*(this->enableAttribute_))(loc);
}

void ShaderInput::enableUniform1f(GLint loc) const {
	glUniform1fv(loc, elementCount_, (const GLfloat *) clientData());
}

void ShaderInput::enableUniform2f(GLint loc) const {
	glUniform2fv(loc, elementCount_, (const GLfloat *) clientData());
}

void ShaderInput::enableUniform3f(GLint loc) const {
	glUniform3fv(loc, elementCount_, (const GLfloat *) clientData());
}

void ShaderInput::enableUniform4f(GLint loc) const {
	glUniform4fv(loc, elementCount_, (const GLfloat *) clientData());
}

void ShaderInput::enableUniformMat3(GLint loc) const {
	glUniformMatrix3fv(loc, elementCount_, transpose_, (const GLfloat *) clientData());
}

void ShaderInput::enableUniformMat4(GLint loc) const {
	glUniformMatrix4fv(loc, elementCount_, transpose_, (const GLfloat *) clientData());
}

void ShaderInput::enableUniform1d(GLint loc) const {
	glUniform1dv(loc, elementCount_, (const GLdouble *) clientData());
}

void ShaderInput::enableUniform2d(GLint loc) const {
	glUniform2dv(loc, elementCount_, (const GLdouble *) clientData());
}

void ShaderInput::enableUniform3d(GLint loc) const {
	glUniform3dv(loc, elementCount_, (const GLdouble *) clientData());
}

void ShaderInput::enableUniform4d(GLint loc) const {
	glUniform4dv(loc, elementCount_, (const GLdouble *) clientData());
}

void ShaderInput::enableUniform1i(GLint loc) const {
	glUniform1iv(loc, elementCount_, (const GLint *) clientData());
}

void ShaderInput::enableUniform2i(GLint loc) const {
	glUniform2iv(loc, elementCount_, (const GLint *) clientData());
}

void ShaderInput::enableUniform3i(GLint loc) const {
	glUniform3iv(loc, elementCount_, (const GLint *) clientData());
}

void ShaderInput::enableUniform4i(GLint loc) const {
	glUniform4iv(loc, elementCount_, (const GLint *) clientData());
}

void ShaderInput::enableUniform1ui(GLint loc) const {
	glUniform1uiv(loc, elementCount_, (const GLuint *) clientData());
}

void ShaderInput::enableUniform2ui(GLint loc) const {
	glUniform2uiv(loc, elementCount_, (const GLuint *) clientData());
}

void ShaderInput::enableUniform3ui(GLint loc) const {
	glUniform3uiv(loc, elementCount_, (const GLuint *) clientData());
}

void ShaderInput::enableUniform4ui(GLint loc) const {
	glUniform4uiv(loc, elementCount_, (const GLuint *) clientData());
}

void ShaderInput::enableUniform(GLint loc) const {
	enableUniform_(loc);
}

/////////////
/////////////
////////////

void ShaderInput::setUniformDataUntyped(byte *data) {
	setInstanceData(1, 1, data);
	isVertexAttribute_ = GL_FALSE;
}

void ShaderInput::setVertexData(
		GLuint numVertices,
		const byte *vertexData) {
	isVertexAttribute_ = GL_TRUE;
	numVertices_ = numVertices;
	numInstances_ = 1u;
	divisor_ = 0u;
	GLuint size = elementSize_ * numVertices_;
	if (inputSize_ != size) {
		if (data_) {
			delete[] data_;
		}
		data_ = new byte[size];
		inputSize_ = size;
	}
	if (vertexData) {
		std::memcpy(data_, vertexData, inputSize_);
	}
	stamp_ += 1;
	// make new data stack root
	dataStack_.popBottom();
	dataStack_.pushBottom(data_);
}

void ShaderInput::setInstanceData(
		GLuint numInstances,
		GLuint divisor,
		const byte *instanceData) {
	isVertexAttribute_ = GL_TRUE;
	numInstances_ = std::max(1u, numInstances);
	divisor_ = std::max(1u, divisor);
	numVertices_ = 1u;
	GLuint size = elementSize_ * numInstances_ / divisor_;
	if (inputSize_ != size) {
		if (data_) {
			delete[] data_;
		}
		data_ = new byte[size];
		inputSize_ = size;
	}
	if (instanceData) {
		std::memcpy(data_, instanceData, inputSize_);
	}
	stamp_ += 1;
	// make new data stack root
	dataStack_.popBottom();
	dataStack_.pushBottom(data_);
}

void ShaderInput::deallocateClientData() {
	// set null data pointer
	dataStack_.popBottom();
	dataStack_.pushBottom(nullptr);
	if (data_ != nullptr) {
		delete[]data_;
		data_ = nullptr;
	}
}

void ShaderInput::writeServerData(RenderState *rs, GLuint index) {
	if (!hasClientData()) return;
	if (!hasServerData()) return;
	GLuint offset = offset_ + stride_ * index;
	byte *data = data_ + elementSize_ * index;

	rs->copyWriteBuffer().push(buffer_);
	glBufferSubData(GL_COPY_WRITE_BUFFER, offset, elementSize_, data);
	rs->copyWriteBuffer().pop();
}

void ShaderInput::writeServerData(RenderState *rs) {
	if (!hasClientData()) return;
	if (!hasServerData()) return;
	if (bufferStamp_ == stamp_) return;
	GLuint count = std::max(numVertices_, numInstances_);

	rs->copyWriteBuffer().push(buffer_);
	if (stride_ == elementSize_) {
		glBufferSubData(GL_COPY_WRITE_BUFFER, offset_, inputSize_, data_);
	} else {
		GLuint offset = offset_;
		byte *data = data_;
		for (GLuint i = 0; i < count; ++i) {
			glBufferSubData(GL_COPY_WRITE_BUFFER, offset, elementSize_, data);
			offset += stride_;
			data += elementSize_;
		}
	}
	rs->copyWriteBuffer().pop();

	bufferStamp_ = stamp_;
}

void ShaderInput::readServerData() {
	if (!hasServerData()) return;

	RenderState *rs = RenderState::get();
	if (data_ == nullptr) data_ = new byte[inputSize_];

	rs->arrayBuffer().push(buffer());
	byte *serverData = (byte *) glMapBufferRange(
			GL_ARRAY_BUFFER,
			offset_,
			numVertices_ * stride_ + elementSize_,
			GL_MAP_READ_BIT);
	byte *clientData = data_;

	if (stride_ == elementSize_) {
		std::memcpy(clientData, serverData, inputSize_);
	} else {
		for (GLuint i = 0; i < numVertices_; ++i) {
			std::memcpy(clientData, serverData, elementSize_);
			serverData += stride_;
			clientData += elementSize_;
		}
	}


	glUnmapBuffer(GL_ARRAY_BUFFER);
	rs->arrayBuffer().pop();
}

GLboolean ShaderInput::hasClientData() {
	return data_ != nullptr;
}

GLboolean ShaderInput::hasServerData() {
	return buffer_ != 0;
}

GLboolean ShaderInput::hasData() {
	return hasClientData() || hasServerData();
}

const byte *ShaderInput::clientData() const {
	return dataStack_.top();
}

byte *ShaderInput::clientDataPtr() {
	return dataStack_.top();
}

byte *ShaderInput::ownedClientData() {
	return data_;
}

void ShaderInput::pushClientData(byte *data) {
	dataStack_.push(data);
	stamp_ += 1;
}

void ShaderInput::popClientData() {
	dataStack_.pop();
	stamp_ += 1;
}

/////////////
/////////////
////////////

ref_ptr<ShaderInput> ShaderInput::create(const ref_ptr<ShaderInput> &in) {
	if (in->isUniformBlock()) {
		auto newBlock = ref_ptr<UniformBlock>::alloc(in->name());
		auto oldBlock = (UniformBlock *)(in.get());
		for (auto &uniform : oldBlock->uniforms()) {
			newBlock->addUniform(create(uniform));
		}
		return newBlock;
	}

	const std::string &name = in->name();
	GLenum dataType = in->dataType();
	GLuint valsPerElement = in->valsPerElement();

	switch (dataType) {
		case GL_FLOAT:
			switch (valsPerElement) {
				case 16:
					return ref_ptr<ShaderInputMat4>::alloc(name);
				case 9:
					return ref_ptr<ShaderInputMat3>::alloc(name);
				case 4:
					return ref_ptr<ShaderInput4f>::alloc(name);
				case 3:
					return ref_ptr<ShaderInput3f>::alloc(name);
				case 2:
					return ref_ptr<ShaderInput2f>::alloc(name);
				default:
					return ref_ptr<ShaderInput1f>::alloc(name);
			}
			break;
		case GL_DOUBLE:
			switch (valsPerElement) {
				case 4:
					return ref_ptr<ShaderInput4d>::alloc(name);
				case 3:
					return ref_ptr<ShaderInput3d>::alloc(name);
				case 2:
					return ref_ptr<ShaderInput2d>::alloc(name);
				default:
					return ref_ptr<ShaderInput1d>::alloc(name);
			}
			break;
		case GL_BOOL:
		case GL_INT:
			switch (valsPerElement) {
				case 4:
					return ref_ptr<ShaderInput4i>::alloc(name);
				case 3:
					return ref_ptr<ShaderInput3i>::alloc(name);
				case 2:
					return ref_ptr<ShaderInput2i>::alloc(name);
				default:
					return ref_ptr<ShaderInput1i>::alloc(name);
			}
			break;
		case GL_UNSIGNED_INT:
			switch (valsPerElement) {
				case 4:
					return ref_ptr<ShaderInput4ui>::alloc(name);
				case 3:
					return ref_ptr<ShaderInput3ui>::alloc(name);
				case 2:
					return ref_ptr<ShaderInput2ui>::alloc(name);
				default:
					return ref_ptr<ShaderInput1ui>::alloc(name);
			}
			break;
		default:
			return {};
	}
}

ref_ptr<ShaderInput> ShaderInput::copy(const ref_ptr<ShaderInput> &in, GLboolean copyData) {
	ref_ptr<ShaderInput> cp = create(in);
	cp->stride_ = in->stride_;
	cp->offset_ = in->offset_;
	cp->inputSize_ = in->inputSize_;
	cp->elementSize_ = in->elementSize_;
	cp->elementCount_ = in->elementCount_;
	cp->numVertices_ = in->numVertices_;
	cp->numInstances_ = in->numInstances_;
	cp->divisor_ = in->divisor_;
	cp->buffer_ = 0;
	cp->bufferStamp_ = 0;
	cp->normalize_ = in->normalize_;
	cp->isVertexAttribute_ = in->isVertexAttribute_;
	cp->isUniformBlock_ = in->isUniformBlock_;
	cp->isConstant_ = in->isConstant_;
	cp->transpose_ = in->transpose_;
	cp->stamp_ = in->stamp_;
	cp->forceArray_ = in->forceArray_;
	cp->data_ = new byte[cp->inputSize_];

	if (copyData && in->data_ != nullptr) {
		std::memcpy(cp->data_, in->data_, cp->inputSize_);
	}
	// make data_ stack root
	cp->dataStack_.push(cp->data_);

	return cp;
}

/////////////
/////////////
/////////////

ShaderInput1f::ShaderInput1f(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableUniform_ = [this](GLint loc) { enableUniform1f(loc); };
}

ShaderInput2f::ShaderInput2f(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableUniform_ = [this](GLint loc) { enableUniform2f(loc); };
}

ShaderInput3f::ShaderInput3f(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableUniform_ = [this](GLint loc) { enableUniform3f(loc); };
}

ShaderInput4f::ShaderInput4f(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableUniform_ = [this](GLint loc) { enableUniform4f(loc); };
}

ShaderInputMat3::ShaderInputMat3(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	transpose_ = GL_FALSE;
	enableAttribute_ = &ShaderInput::enableAttributeMat3;
	enableUniform_ = [this](GLint loc) { enableUniformMat3(loc); };
}

ShaderInputMat4::ShaderInputMat4(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	transpose_ = GL_FALSE;
	enableAttribute_ = &ShaderInput::enableAttributeMat4;
	enableUniform_ = [this](GLint loc) { enableUniformMat4(loc); };
}

ShaderInput1d::ShaderInput1d(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableUniform_ = [this](GLint loc) { enableUniform1d(loc); };
}

ShaderInput2d::ShaderInput2d(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableUniform_ = [this](GLint loc) { enableUniform2d(loc); };
}

ShaderInput3d::ShaderInput3d(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableUniform_ = [this](GLint loc) { enableUniform3d(loc); };
}

ShaderInput4d::ShaderInput4d(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableUniform_ = [this](GLint loc) { enableUniform4d(loc); };
}

ShaderInput1i::ShaderInput1i(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) { enableUniform1i(loc); };
}

ShaderInput2i::ShaderInput2i(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) { enableUniform2i(loc); };
}

ShaderInput3i::ShaderInput3i(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) { enableUniform3i(loc); };
}

ShaderInput4i::ShaderInput4i(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) { enableUniform4i(loc); };
}

ShaderInput1ui::ShaderInput1ui(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) { enableUniform1ui(loc); };
}

ShaderInput2ui::ShaderInput2ui(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) { enableUniform2ui(loc); };
}

ShaderInput3ui::ShaderInput3ui(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) { enableUniform3ui(loc); };
}

ShaderInput4ui::ShaderInput4ui(
		const std::string &name,
		GLuint elementCount,
		GLboolean normalize)
		: ShaderInputTyped(name, elementCount, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) { enableUniform4ui(loc); };
}

/////////////
/////////////
/////////////

// declare the private struct
struct UniformBlock::UniformBlockData {
	ref_ptr<UBO> ubo;
};

UniformBlock::UniformBlock(const std::string &name) :
	ShaderInput(name, GL_INVALID_ENUM, 0, 0, 0, GL_FALSE){
	enableUniform_ = [this](GLint loc) { enableUniformBlock(loc); };
	isUniformBlock_ = GL_TRUE;
	isVertexAttribute_ = GL_FALSE;
	isVertexAttribute_ = GL_FALSE;
	priv_ = new UniformBlockData();
	priv_->ubo = ref_ptr<UBO>::alloc();
}

UniformBlock::~UniformBlock() {
	delete priv_;
}

GLuint UniformBlock::stamp() const {
	return priv_->ubo->stamp();
}

void UniformBlock::enableUniformBlock(GLint loc) const {
	priv_->ubo->update();
	priv_->ubo->bindBufferBase(loc);
}

const std::vector<ref_ptr<ShaderInput>> &UniformBlock::uniforms() const {
	return priv_->ubo->uniforms();
}

void UniformBlock::addUniform(const ref_ptr<ShaderInput> &input) {
	priv_->ubo->addUniform(input);
}

void UniformBlock::update(bool forceUpdate) {
	priv_->ubo->update(forceUpdate);
}

void UniformBlock::write(std::ostream &out) const {
	out << "uniform " << name() << " {\n";
	out << "};";
}

/////////////
/////////////
/////////////

VAO::VAO()
		: GLObject(glGenVertexArrays, glDeleteVertexArrays) {
}

void VAO::resetGL() {
	glDeleteVertexArrays(1, ids_);
	glGenVertexArrays(1, ids_);
}
