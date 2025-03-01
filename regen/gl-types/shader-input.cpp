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
#include "uniform-block.h"

using namespace regen;

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
		GLint valsPerElement,
		GLsizei numArrayElements,
		GLboolean normalize)
		: name_(name),
		  dataType_(dataType),
		  dataTypeBytes_(dataTypeBytes),
		  stride_(0),
		  offset_(0),
		  inputSize_(0),
		  numArrayElements_(numArrayElements),
		  numVertices_(1u),
		  numInstances_(1u),
		  numElements_(numArrayElements),
		  valsPerElement_(valsPerElement),
		  divisor_(0),
		  buffer_(0),
		  bufferStamp_(0),
		  normalize_(normalize),
		  isVertexAttribute_(false),
		  transpose_(false),
		  dataSlots_({nullptr, nullptr}),
		  isConstant_(false),
		  isUniformBlock_(false),
		  forceArray_(false),
		  active_(true) {
	elementSize_ = dataTypeBytes_ * valsPerElement_ * numArrayElements_;
	enableAttribute_ = &ShaderInput::enableAttributef;
}

ShaderInput::ShaderInput(const ShaderInput &o)
		: name_(o.name_),
		  dataType_(o.dataType_),
		  dataTypeBytes_(o.dataTypeBytes_),
		  stride_(o.stride_),
		  offset_(o.offset_),
		  inputSize_(o.inputSize_),
		  elementSize_(o.elementSize_),
		  numArrayElements_(o.numArrayElements_),
		  numVertices_(o.numVertices_),
		  numInstances_(o.numInstances_),
		  numElements_(o.numElements_),
		  valsPerElement_(o.valsPerElement_),
		  divisor_(o.divisor_),
		  buffer_(o.buffer_),
		  bufferStamp_(o.bufferStamp_),
		  normalize_(o.normalize_),
		  isVertexAttribute_(o.isVertexAttribute_),
		  transpose_(o.transpose_),
		  dataSlots_({nullptr, nullptr}),
		  isConstant_(o.isConstant_),
		  isUniformBlock_(o.isUniformBlock_),
		  forceArray_(o.forceArray_),
		  active_(o.active_) {
	enableAttribute_ = &ShaderInput::enableAttributef;
	enableUniform_ = o.enableUniform_;
	// copy client data, if any
	if (o.hasClientData()) {
		dataSlots_[0] = new byte[inputSize_];
		auto mapped = o.mapClientData(ShaderData::READ);
		std::memcpy(dataSlots_[0], mapped.r, inputSize_);
	}
}

ShaderInput::~ShaderInput() {
	if (bufferIterator_.get()) {
		VBO::free(bufferIterator_.get());
	}
	deallocateClientData();
}

void ShaderInput::set_numArrayElements(GLsizei v) {
	numArrayElements_ = v;
	elementSize_ = dataTypeBytes_ * valsPerElement_ * numArrayElements_;
	if (isVertexAttribute_) {
		numElements_ = numArrayElements_ * numVertices_;
	} else {
		numElements_ = numArrayElements_ * numInstances_;
	}
}

unsigned int ShaderInput::stamp() const {
	return dataStamp_.load(std::memory_order_relaxed);
}

int ShaderInput::lastDataSlot() const {
	return lastDataSlot_.load(std::memory_order_relaxed);
}

void ShaderInput::set_buffer(GLuint buffer, const VBOReference &it) {
	buffer_ = buffer;
	bufferIterator_ = it;
	bufferStamp_ = stamp();
}

void ShaderInput::enableAttribute(GLint loc) const {
	if (requiresReUpload_) {
		writeServerData();
		requiresReUpload_ = false;
	}
	(this->*(this->enableAttribute_))(loc);
}

void ShaderInput::enableUniform(GLint loc) const {
	enableUniform_(loc);
}

/////////////
/////////////
////////////

const byte* ShaderInput::readLock(int dataSlot) const {
	auto &slotLock = slotLocks_[dataSlot];
	// First get a unique lock on the slot, such that we can safely write its members.
	std::unique_lock<std::mutex> lk(slotLock.lock);
	while( slotLock.waitingWriters != 0 ) {
		// in case the slot is write-locked, wait until all writers have finished.
		slotLock.readerQ.wait(lk);
	}
	// here we know there are no waiting nor active writers, increment active readers and return the data,
	// it can be used safely by the caller until the lock is released.
	++slotLock.activeReaders;
	byte *data = dataSlots_[dataSlot];
	lk.unlock();
	return data;
}

const byte* ShaderInput::readLockTry(int dataSlot) const {
	auto &slotLock = slotLocks_[dataSlot];
	// First get a unique lock on the slot, such that we can safely write its members.
	std::unique_lock<std::mutex> lk(slotLock.lock, std::try_to_lock);
	if (!lk || slotLock.waitingWriters != 0) {
		return nullptr;
	}
	// here we know there are no waiting nor active writers, increment active readers and return the data,
	// it can be used safely by the caller until the lock is released.
	++slotLock.activeReaders;
	const byte *data = dataSlots_[dataSlot];
	lk.unlock();
	return data;
}

byte* ShaderInput::writeLock(int dataSlot) const {
	auto &slotLock = slotLocks_[dataSlot];
	// First get a unique lock on the slot, such that we can safely write its members.
	std::unique_lock<std::mutex> lk(slotLock.lock);
	// increment waiting writers and wait until there are no active readers or writers.
	++slotLock.waitingWriters;
	while( slotLock.activeReaders != 0 || slotLock.activeWriters != 0 ) {
		slotLock.writerQ.wait(lk);
	}
	// also increment active writers counter
	++slotLock.activeWriters;
	byte *data = dataSlots_[dataSlot];
	lk.unlock();
	return data;
}

byte* ShaderInput::writeLockTry(int dataSlot) const {
	auto &slotLock = slotLocks_[dataSlot];
	// First get a unique lock on the slot, such that we can safely write its members.
	std::unique_lock<std::mutex> lk(slotLock.lock, std::try_to_lock);
	if (!lk || slotLock.activeReaders != 0 || slotLock.activeWriters != 0) {
		return nullptr;
	}
	++slotLock.waitingWriters;
	++slotLock.activeWriters;
	byte *data = dataSlots_[dataSlot];
	lk.unlock();
	return data;
}

void ShaderInput::readUnlock(int dataSlot) const {
	auto &slotLock = slotLocks_[dataSlot];
	// decrement active readers
	std::unique_lock<std::mutex> lk(slotLock.lock);
	--slotLock.activeReaders;
	lk.unlock();
	// notify any waiting writers
	slotLock.writerQ.notify_one();
}

void ShaderInput::writeUnlock(int dataSlot, bool hasDataChanged) const {
	auto &slotLock = slotLocks_[dataSlot];
	std::unique_lock<std::mutex> lk(slotLock.lock);
	--slotLock.waitingWriters;
	--slotLock.activeWriters;
	// TODO: if the data is small, then we could do a check here with memcmp to avoid incrementing the stamp, and
	//       switching the data slot. This could be more efficient in case the data is not changed.
	//if (numInstances > 1 || std::memcmp(data_, data, inputSize_) != 0) { }
	//if (std::memcmp(data_, data, inputSize_) != 0) { }
	if(hasDataChanged) {
		// increment the data stamp, and remember the last slot that was written to.
		// consecutive reads will be done from this slot, next write will be done to the other slot.
		// If the write operation did not change the data, the stamp is not incremented,
		// and the last slot is not updated.
		dataStamp_.fetch_add(1, std::memory_order_relaxed);
		lastDataSlot_.exchange(dataSlot, std::memory_order_relaxed);
		if (hasServerData()) {
			requiresReUpload_ = true;
		}
	}
	if(slotLock.waitingWriters > 0) {
		// first notify the next writer if any
		slotLock.writerQ.notify_one();
	}
	else {
		// else notify all waiting readers
		slotLock.readerQ.notify_all();
	}
	lk.unlock();
}

void ShaderInput::writeLockAll() const {
	writeLock(1);
	writeLock(0);
}

void ShaderInput::writeUnlockAll(bool hasDataChanged) const {
	writeUnlock(1, false);
	writeUnlock(0, hasDataChanged);
}

void ShaderInput::allocateSecondSlot() const {
	writeLock(1);
	auto data_r = readLock(0);
	// need to lock the slot 1 for the memcpy below, to avoid that any mapping happens in between.
	std::unique_lock<std::mutex> lk(slotLocks_[1].lock);
	if (dataSlots_[1] == nullptr) {
		auto data_w = new byte[inputSize_];
		std::memcpy(data_w, data_r, inputSize_);
		dataSlots_[1] = data_w;
	}
	lk.unlock();
	readUnlock(0);
}

MappedData ShaderInput::mapClientData(int mapMode) const {
	int r_index = lastDataSlot();

	if ((mapMode & ShaderData::WRITE) != 0) {
		// NOTE: assuming we can only have two slots, which makes sense IMO, then
		// if read slot=0, then write slot=1, else write slot=0
		int w_index = (int)(r_index == 0);
		byte *data_w;
		// index mapping should e avoided! It might require to copy one slot to the other.
		// NOTE: no index mapping needed if there is only one vertex/array element
		bool isPartialWrite = ((mapMode & ShaderData::INDEX) != 0 && inputSize_ > (dataTypeBytes_ * valsPerElement_));

		// ShaderInput initially has only one slot, the second is allocated on demand in case
		// multiple threads are concurrently reading/writing the data.
		// here we keep writing to the active slot as long as no one has to wait,
		// but as soon as there is waiting time we allocate the second slot and copy the data
		// to avoid waiting in the future.
		if (!hasTwoSlots()) {
			data_w = writeLockTry(r_index);
			if (data_w) {
				// got it!
				w_index = r_index;
				r_index = lastDataSlot();
			}
			else {
				// read slot is locked, and second slot was not allocated yet.
				// create it now, and copy the data to it.
				// note: allocateSecondSlot also acquires a lock in slot 1
				allocateSecondSlot();
				data_w = dataSlots_[1];
				w_index = 1;
				r_index = w_index;
			}
		} else {
			// for now, if there are two slots, we always write to the other slot.
			// NOTE: only in case of partial update other strategies might be better (in some cases)
			data_w = writeLock(w_index);
			// it could be that active slot changed in the meantime, e.g. in case writeLock above had to wait
			// for another write operation to finish. Make sure we read from latest slot.
			r_index = lastDataSlot();
		}

		if (isPartialWrite) {
			// write only updates some vertices, potentially not all.
			// this means we need to ensure the target slot for write has the current data.
			// NOTE: as write slot must have the current data anyways, the "read" toggle can be ignored here.
			// TODO: in some cases a better strategy could be to write into the current slot instead of copying
			//       the data to the other slot. Maybe a sensible heuristic would be the data size:
			//       for small data, especially non-array, non-vertex data, always prefer copy.
			//       for larger array and vertex data prefer write into current slot.
			if (r_index != w_index) {
				auto data_r = readLock(r_index); {
					// copy FULL data into write slot.
					// NOTE: this will be inefficient if the data is large!
					std::memcpy(data_w, data_r, inputSize_);
				} readUnlock(r_index);
			}
			return { data_w, -1, data_w, w_index };
		}
		else { // full write.
			const byte* data_r = nullptr;
			if (r_index != w_index && (mapMode & ShaderData::READ) != 0) {
				// NOTE: only need a read lock if the read slot is different from the write slot.
				data_r = readLock(r_index);
			}
			else {
				data_r = data_w;
				r_index = -1;
			}
			return { data_r, r_index, data_w, w_index };
		}
	}
	else {
		// read only. the case of reading at index is not handled differently here.
		if (hasTwoSlots()) {
			auto readData = readLock(r_index);
			return { readData, r_index };
		}
		else {
			// make an attempt to get direct access
			auto readData = readLockTry(r_index);
			if (readData) {
				return { readData, r_index };
			} else {
				// read slot is write locked, plus the second slot was not allocated yet.
				// allocate it now, copy the data to it, then read from it.
				allocateSecondSlot();
				writeUnlock(1, false);
				r_index = lastDataSlot();
				readData = readLock(r_index);
				return { readData, r_index };
			}
		}
	}
}

void ShaderInput::unmapClientData(int mapMode, int slotIndex) const {
	if ((mapMode & ShaderData::WRITE) != 0) {
		writeUnlock(slotIndex, true);
	}
	else {
		readUnlock(slotIndex);
	}
}

void ShaderInput::writeVertex(GLuint index, const byte *data) {
	auto mapped = mapClientDataRaw(ShaderData::WRITE | ShaderData::INDEX);
	// NOTE: it is maybe a bit confusing, but the semantics of writeVertex is currently
	//       different for uniform array data vs vertex data.
	//       For vertex data, it is assumed that data is one vertex including all array elements.
	//       For uniform array data, it is assumed that data is one array element.
	if (isVertexAttribute_) {
		std::memcpy(mapped.w + index*elementSize_, data, elementSize_);
	}
	else {
		auto arrayElementSize = dataTypeBytes_ * valsPerElement_;
		std::memcpy(mapped.w + index*arrayElementSize, data, arrayElementSize);
	}
}

void ShaderInput::deallocateClientData() {
	for (int i = 0; i < 2; ++i) {
		std::unique_lock<std::mutex> lk(slotLocks_[i].lock);
		if (dataSlots_[i]) {
			delete[] dataSlots_[i];
			dataSlots_[i] = nullptr;
		}
	}
}

void ShaderInput::reallocateClientData(size_t size) {
	{
		std::unique_lock<std::mutex> lk(slotLocks_[0].lock);
		if (dataSlots_[0]) {
			delete[] dataSlots_[0];
		}
		dataSlots_[0] = new byte[size];
	}
	{
		std::unique_lock<std::mutex> lk(slotLocks_[1].lock);
		if (dataSlots_[1]) {
			delete[] dataSlots_[1];
			dataSlots_[1] = new byte[size];
		}
	}
}

void ShaderInput::setUniformUntyped(const byte *data) {
	setInstanceData(1, 1, data);
}

void ShaderInput::setInstanceData(GLuint numInstances, GLuint divisor, const byte *data) {
	auto dataSize_bytes = elementSize_ * numInstances / divisor;

	if (dataSize_bytes != inputSize_ || isVertexAttribute_ || !hasClientData()) {
		// size of the data has changed, need to reallocate the data buffer.
		writeLockAll();
		reallocateClientData(dataSize_bytes);
		isVertexAttribute_ = false;
		numInstances_ = std::max(1u, numInstances);
		divisor_ = std::max(1u, divisor);
		numVertices_ = 1u;
		numElements_ = static_cast<int>(numArrayElements_ * numInstances_);
		inputSize_ = dataSize_bytes;
		writeUnlockAll(writeClientData_(data));
	} else if(data) {
		auto mapped = mapClientDataRaw(ShaderData::WRITE);
		std::memcpy(mapped.w, data, dataSize_bytes);
	}
}

void ShaderInput::setVertexData(GLuint numVertices, const byte *data) {
	auto dataSize_bytes = elementSize_ * numVertices;

	if (dataSize_bytes != inputSize_ || !isVertexAttribute_ || !hasClientData()) {
		// size of the data has changed, need to reallocate the data buffer.
		writeLockAll();
		reallocateClientData(dataSize_bytes);
		isVertexAttribute_ = true;
		numInstances_ = 1u;
		divisor_ = 0u;
		numVertices_ = numVertices;
		numElements_ = static_cast<int>(numArrayElements_ * numVertices_);
		inputSize_ = dataSize_bytes;
		writeUnlockAll(writeClientData_(data));
	} else if(data) {
		auto mapped = mapClientDataRaw(ShaderData::WRITE);
		std::memcpy(mapped.w, data, dataSize_bytes);
	}
}

bool ShaderInput::writeClientData_(const byte *data) {
	if (data) {
		// NOTE: writeLockAll locks slot 0 last, so we know it will be the active slot when we unlock.
		std::memcpy(dataSlots_[0], data, inputSize_);
		return true;
	}
	else {
		return false;
	}
}

void ShaderInput::writeServerData(GLuint index) const {
	if (!hasClientData() || !hasServerData()) return;
	auto mappedClientData = mapClientData(ShaderData::READ);
	auto clientData = mappedClientData.r;
	auto subDataStart = clientData + elementSize_ * index;

	RenderState::get()->copyWriteBuffer().push(buffer_);
	glBufferSubData(
		GL_COPY_WRITE_BUFFER,
		offset_ + stride_ * index,
		elementSize_,
		subDataStart);
	RenderState::get()->copyWriteBuffer().pop();
}

void ShaderInput::writeServerData() const {
	if (!hasClientData() || !hasServerData()) return;
	if (bufferStamp_ == stamp()) return;
	auto mappedClientData = mapClientData(ShaderData::READ);
	auto clientData = mappedClientData.r;
	auto count = std::max(numVertices_, numInstances_);

	RenderState::get()->copyWriteBuffer().push(buffer_);
	if (stride_ == elementSize_) {
		glBufferSubData(GL_COPY_WRITE_BUFFER, offset_, inputSize_, clientData);
	} else {
		GLuint offset = offset_;
		for (GLuint i = 0; i < count; ++i) {
			glBufferSubData(GL_COPY_WRITE_BUFFER, offset, elementSize_, clientData);
			offset += stride_;
			clientData += elementSize_;
		}
	}
	RenderState::get()->copyWriteBuffer().pop();

	bufferStamp_ = stamp();
}

void ShaderInput::readServerData() {
	if (!hasServerData()) return;
	auto mappedClientData = mapClientData(ShaderData::WRITE);
	auto clientData = mappedClientData.w;

	RenderState::get()->arrayBuffer().push(buffer());
	byte *serverData = (byte *) glMapBufferRange(
			GL_ARRAY_BUFFER,
			offset_,
			numVertices_ * stride_ + elementSize_,
			GL_MAP_READ_BIT);

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
	RenderState::get()->arrayBuffer().pop();
}

GLboolean ShaderInput::hasClientData() const {
	return dataSlots_[0] != nullptr;
}

GLboolean ShaderInput::hasServerData() const {
	return buffer_ != 0;
}

GLboolean ShaderInput::hasData() const {
	return hasClientData() || hasServerData();
}

/////////////
/////////////
////////////

ref_ptr<ShaderInput> ShaderInput::create(const ref_ptr<ShaderInput> &in) {
	if (in->isUniformBlock()) {
		auto newBlock = ref_ptr<UniformBlock>::alloc(in->name());
		auto oldBlock = (UniformBlock *) (in.get());
		for (auto &namedUniform: oldBlock->uniforms()) {
			newBlock->addUniform(create(namedUniform.in_), namedUniform.name_);
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
	cp->numArrayElements_ = in->numArrayElements_;
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
	cp->forceArray_ = in->forceArray_;
	if (in->hasClientData()) {
		// allocate memory for one slot, copy most recent data
		auto mapped = in->mapClientData(ShaderData::READ);
		cp->dataSlots_[0] = new byte[cp->inputSize_];
		std::memcpy(cp->dataSlots_[0], mapped.r, cp->inputSize_);
	}
	return cp;
}


/////////////
/////////////
/////////////

void ShaderInput::enableAttributef(GLint location) const {
	for (int i = 0; i < numArrayElements_; ++i) {
		auto loc = location + i;
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
	for (int i = 0; i < numArrayElements_; ++i) {
		auto loc = location + i;
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
	for (int i = 0; i < numArrayElements_ * 4; i += 4) {
		auto loc0 = location + i;
		auto loc1 = location + i + 1;
		auto loc2 = location + i + 2;
		auto loc3 = location + i + 3;

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
	for (int i = 0; i < numArrayElements_ * 3; i += 4) {
		auto loc0 = location + i;
		auto loc1 = location + i + 1;
		auto loc2 = location + i + 2;

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
	for (int i = 0; i < numArrayElements_ * 2; i += 4) {
		auto loc0 = location + i;
		auto loc1 = location + i + 1;

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

/////////////
/////////////
/////////////

ShaderInput1f::ShaderInput1f(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableUniform_ = [this](GLint loc) {
		glUniform1fv(loc, numElements_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInput2f::ShaderInput2f(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableUniform_ = [this](GLint loc) {
		glUniform2fv(loc, numElements_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInput3f::ShaderInput3f(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableUniform_ = [this](GLint loc) {
		glUniform3fv(loc, numElements_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInput4f::ShaderInput4f(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableUniform_ = [this](GLint loc) {
		glUniform4fv(loc, numElements_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInputMat3::ShaderInputMat3(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	transpose_ = GL_FALSE;
	enableAttribute_ = &ShaderInput::enableAttributeMat3;
	enableUniform_ = [this](GLint loc) {
		glUniformMatrix3fv(loc, numElements_, transpose_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInputMat4::ShaderInputMat4(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	transpose_ = GL_FALSE;
	enableAttribute_ = &ShaderInput::enableAttributeMat4;
	enableUniform_ = [this](GLint loc) {
		glUniformMatrix4fv(loc, numElements_, transpose_, mapClientData<float>(ShaderData::READ).r);
	};
}

ShaderInput1d::ShaderInput1d(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableUniform_ = [this](GLint loc) {
		glUniform1dv(loc, numElements_, mapClientData<double>(ShaderData::READ).r);
	};
}

ShaderInput2d::ShaderInput2d(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableUniform_ = [this](GLint loc) {
		glUniform2dv(loc, numElements_, mapClientData<double>(ShaderData::READ).r);
	};
}

ShaderInput3d::ShaderInput3d(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableUniform_ = [this](GLint loc) {
		glUniform3dv(loc, numElements_, mapClientData<double>(ShaderData::READ).r);
	};
}

ShaderInput4d::ShaderInput4d(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableUniform_ = [this](GLint loc) {
		glUniform4dv(loc, numElements_, mapClientData<double>(ShaderData::READ).r);
	};
}

ShaderInput1i::ShaderInput1i(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) {
		glUniform1iv(loc, numElements_, mapClientData<int>(ShaderData::READ).r);
	};
}

ShaderInput2i::ShaderInput2i(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) {
		glUniform2iv(loc, numElements_, mapClientData<int>(ShaderData::READ).r);
	};
}

ShaderInput3i::ShaderInput3i(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) {
		glUniform3iv(loc, numElements_, mapClientData<int>(ShaderData::READ).r);
	};
}

ShaderInput4i::ShaderInput4i(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) {
		glUniform4iv(loc, numElements_, mapClientData<int>(ShaderData::READ).r);
	};
}

ShaderInput1ui::ShaderInput1ui(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) {
		glUniform1uiv(loc, numElements_, mapClientData<unsigned int>(ShaderData::READ).r);
	};
}

ShaderInput2ui::ShaderInput2ui(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) {
		glUniform2uiv(loc, numElements_, mapClientData<unsigned int>(ShaderData::READ).r);
	};
}

ShaderInput3ui::ShaderInput3ui(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) {
		glUniform3uiv(loc, numElements_, mapClientData<unsigned int>(ShaderData::READ).r);
	};
}

ShaderInput4ui::ShaderInput4ui(
		const std::string &name,
		GLuint numArrayElements,
		GLboolean normalize)
		: ShaderInputTyped(name, numArrayElements, normalize) {
	enableAttribute_ = &ShaderInput::enableAttributei;
	enableUniform_ = [this](GLint loc) {
		glUniform4uiv(loc, numElements_, mapClientData<unsigned int>(ShaderData::READ).r);
	};
}
