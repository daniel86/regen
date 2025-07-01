#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/animations/animation.h>
#include <stack>

#include "shader-input.h"
#include "ubo.h"
#include "regen/scene/mesh-processor.h"
#include "ssbo.h"

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

constexpr int SPIN_PAUSE_THRESHOLD = 20;

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
		  dataSlots_({nullptr, nullptr}),
		  isConstant_(o.isConstant_),
		  isBufferBlock_(o.isBufferBlock_),
		  forceArray_(o.forceArray_),
		  active_(o.active_),
		  schema_(o.schema_) {
	enableAttribute_ = &ShaderInput::enableAttribute_f;
	enableInput_ = o.enableInput_;
	// copy client data, if any
	if (o.hasClientData()) {
		dataSlots_[0] = new byte[inputSize_];
		auto mapped = o.mapClientData(ShaderData::READ);
		std::memcpy(dataSlots_[0], mapped.r, inputSize_);
	}
}

ShaderInput::~ShaderInput() {
	if (bufferIterator_.get()) {
		BufferObject::free(bufferIterator_.get());
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

unsigned int ShaderInput::stamp() const {
	return dataStamp_.load(std::memory_order_relaxed);
}

void ShaderInput::nextStamp() {
	dataStamp_.fetch_add(1, std::memory_order_relaxed);
}

int ShaderInput::lastDataSlot() const {
	return lastDataSlot_.load(std::memory_order_acquire);
}

void ShaderInput::set_buffer(GLuint buffer, const ref_ptr<BufferReference> &it) {
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
	enableInput_(loc);
}

/////////////
/////////////
////////////

inline void spinWaitUntil1(std::atomic_flag &flag) {
    for (int i = 0; flag.test(std::memory_order_acquire) != 0; ++i) {
        if (i < 20) CPU_PAUSE();
        else std::this_thread::yield();
    }
}

inline void spinWaitUntil2(std::atomic<uint32_t> &count) {
    for (int i = 0; count.load(std::memory_order_acquire) != 0; ++i) {
        if (i < 20) CPU_PAUSE();
        else std::this_thread::yield();
    }
}

int ShaderInput::readLock() const {
	while (true) {
		// get the current slot index for reading.
		// note that every writer will flip the slot index, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try, e.g.
		// because there are active writers on the slot which in turn will flip the slot index once done.
		int dataSlot = lastDataSlot_.load(std::memory_order_acquire);

		// first step: increment the reader count for this slot.
		// this will prevent writers from setting the flag on this slot.
		readerCounts_[dataSlot].fetch_add(1, std::memory_order_relaxed);

		// however, maybe there is an active writer on this slot already, we need to check that.
		if (writerFlags_[dataSlot].test(std::memory_order_acquire) == 0) {
			// no writer has locked the slot, other ones are prevented from doing so,
			// hence we can safely read from this slot.
			return dataSlot;
		}
		else {
			// seems there is an active writer on this slot, we need to wait for them to finish.
			// but first decrement the reader count, so that we do not block writer in the meanwhile.
			readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
			// wait until there are no active writers on `dataSlot`.
			spinWaitUntil1(writerFlags_[dataSlot]);
		}
	}
}

bool ShaderInput::readLock_SingleBuffer() const {
	// we are here in single buffer mode, and only quickly try to get a lock in the one
	// slot (with index 0), or else return false.
	// and the only thing preventing us from doing so would be a writer that is currently writing to the slot
	// which would be indicated by the writerFlags_[0] being set.
	readerCounts_[0].fetch_add(1, std::memory_order_relaxed);
	if (writerFlags_[0].test(std::memory_order_acquire) != 0) {
		readerCounts_[0].fetch_sub(1, std::memory_order_relaxed);
		return false; // Busy writing
	} else {
		return true;
	}
}

void ShaderInput::readUnlock(int dataSlot) const {
	readerCounts_[dataSlot].fetch_sub(1, std::memory_order_relaxed);
}

int ShaderInput::writeLock() const {
	while (true) {
		// get the current slot index for writing.
		// note that every writer will flip the slot index, so we need to keep loading
		// it within this loop in case we cannot obtain the lock on first try.
		int dataSlot = 1 - lastDataSlot_.load(std::memory_order_acquire);

		// check if there are any active readers on the write slot.
		if (readerCounts_[dataSlot].load(std::memory_order_acquire) != 0) {
			// seems there are some remaining readers on the write slot, we need to wait for them to finish.
			spinWaitUntil2(readerCounts_[dataSlot]);
			continue; // try again
		}

		if (writerFlags_[dataSlot].test_and_set(std::memory_order_acquire)) {
			// seems someone else is writing to this slot, we need to wait for them to finish.
			spinWaitUntil1(writerFlags_[dataSlot]);
			continue; // try again
		} else {
			if (readerCounts_[dataSlot].load(std::memory_order_acquire) != 0) {
				// a reader sneaked in while we were waiting for the write lock
				writerFlags_[dataSlot].clear(std::memory_order_relaxed);
				continue;
			}
			// we got the exclusive write lock for this slot, so we can safely write to it.
			return dataSlot;
		}
	}
}

bool ShaderInput::writeLock_SingleBuffer() const {
	// acquire exclusive write lock
	if (writerFlags_[0].test_and_set(std::memory_order_acquire)) {
		return false; // Busy writing
	}
	// check for any active readers.
	if (readerCounts_[0].load(std::memory_order_acquire) != 0) {
		writerFlags_[0].clear(std::memory_order_relaxed);
		return false; // Busy reading
	}
	return true;
}

void ShaderInput::writeUnlock(int dataSlot, bool hasDataChanged) const {
	if (hasDataChanged) {
		// increment the data stamp, and remember the last slot that was written to.
		// consecutive reads will be done from this slot, next write will be done to the other slot.
		// If the write operation did not change the data, the stamp is not incremented,
		// and the last slot is not updated.
		dataStamp_.fetch_add(1, std::memory_order_relaxed);
		lastDataSlot_.store(dataSlot, std::memory_order_release);
		if (hasServerData()) {
			requiresReUpload_ = true;
		}
	}
	// clear the exclusive write lock for this slot, allowing any waiting writer to proceed.
	// NOTE: reader will only proceed once all writing is done.
	writerFlags_[dataSlot].clear(std::memory_order_relaxed);
}

void ShaderInput::writeLockAll() const {
	for (int i = 0; i < 2; ++i) {
		// get exclusive write access to the data slot:
		// block any attempt to write concurrently to this slot.
		while (writerFlags_[i].test_and_set(std::memory_order_acquire)) {
			CPU_PAUSE(); // spin-wait for writers
		}
	}
	for (int i = 0; i < 2; ++i) {
		// wait for any active readers to finish.
		spinWaitUntil2(readerCounts_[i]);
	}
}

void ShaderInput::writeUnlockAll(bool hasDataChanged) const {
	writeUnlock(1, false);
	writeUnlock(0, hasDataChanged);
}

void ShaderInput::allocateSecondSlot() const {
	auto data_w = new byte[inputSize_];
	std::memcpy(data_w, dataSlots_[0], inputSize_);
	dataSlots_[1] = data_w;
}

MappedData ShaderInput::mapClientData(int mapMode) const {
	// ShaderInput initially has only one slot, the second is allocated on demand in case
	// multiple threads are concurrently reading/writing the data.
	// here we keep writing to the active slot as long as no one has to wait,
	// but as soon as there is waiting time we allocate the second slot and copy the data
	// to avoid waiting in the future.

	if ((mapMode & ShaderData::WRITE) != 0) {
		if (!hasTwoSlots()) {
			// partial writing is ok here, as we update the most recent data slot.
			// NOTE: r_index -1 indicates that there is no read lock, i.e. no need to call readUnlock in unmap.
			if (writeLock_SingleBuffer()) {
				// got the write lock, return the data.
				// this means there are currently no readers, nor writers, so we can safely write to the active slot.
				return { dataSlots_[0], -1, dataSlots_[0], 0 };
			} else {
				// write lock failed, which means there is another operation in progress.
				// in this case we allocate the second slot, and copy the data from the first slot to it,
				// i.e. we switch to double-buffered mode.
				writeLockAll();
				if (dataSlots_[1] == nullptr) {
					allocateSecondSlot();
					writeUnlock(0, false);
					return { dataSlots_[1], -1, dataSlots_[1], 1 };
				} else {
					// someone else has already allocated the second slot
					writeUnlockAll(false);
					int w_index = writeLock();
					return { dataSlots_[w_index], -1, dataSlots_[w_index], w_index };
				}
			}
		} else {
			// we are in double-buffered mode, i.e. we have two slots.
			// partial write can be expensive here!
			// NOTE: no index mapping needed if there is only one vertex/array element
			bool isFullWrite = !(((mapMode & ShaderData::INDEX) != 0 && inputSize_ >
				(dataTypeBytes_ * valsPerElement_ * numArrayElements_)));
			int w_index = writeLock();
			byte *data_w = dataSlots_[w_index];

			if (isFullWrite) {
				if ((mapMode & ShaderData::READ) != 0) {
					int r_index = readLock();
					return { dataSlots_[r_index], r_index, data_w, w_index };
				} else {
					return { data_w, -1, data_w, w_index };
				}
			} else {
				// copy FULL data into write slot for partial write.
				// NOTE: this will be inefficient if the data is large!
				// TODO: in some cases a better strategy could be to write into the current slot instead of copying
				//       the data to the other slot. Maybe a sensible heuristic would be the data size:
				//       for small data, especially non-array, non-vertex data, always prefer copy.
				//       for larger array and vertex data prefer write into current slot.
				int r_index = readLock();
				std::memcpy(data_w, dataSlots_[r_index], inputSize_);
				readUnlock(r_index);
				return { data_w, -1, data_w, w_index };
			}
		}
	} else {
		// read only. the case of reading at index is not handled differently here.
		if (!hasTwoSlots()) {
			// we are still in single-buffered mode.
			// first we try to get a read lock on the single slot.
			if (readLock_SingleBuffer()) {
				// got the read lock, return the data.
				return { dataSlots_[0], 0 };
			} else {
				// read lock failed, which means there is a write operation in progress.
				// in this case we allocate the second slot, and copy the data from the first slot to it,
				// i.e. we switch to double-buffered mode.
				writeLockAll();
				if (dataSlots_[1] == nullptr) {
					allocateSecondSlot();
					writeUnlock(0, false);
					writeUnlock(1, true);
				} else {
					writeUnlockAll(false);
				}
			}
		}
		// read lock in double-buffered mode.
		int r_index = readLock();
		return { dataSlots_[r_index], r_index };
	}
}

void ShaderInput::unmapClientData(int mapMode, int slotIndex) const {
	if ((mapMode & ShaderData::WRITE) != 0) {
		writeUnlock(slotIndex, true);
	} else {
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
		std::memcpy(mapped.w + index * elementSize_, data, elementSize_);
	} else {
		auto arrayElementSize = dataTypeBytes_ * valsPerElement_;
		std::memcpy(mapped.w + index * arrayElementSize, data, arrayElementSize);
	}
}

void ShaderInput::deallocateClientData() {
	for (int i = 0; i < 2; ++i) {
		if (dataSlots_[i]) {
			delete[] dataSlots_[i];
			dataSlots_[i] = nullptr;
		}
	}
}

void ShaderInput::reallocateClientData(size_t size) {
	{
		if (dataSlots_[0]) {
			delete[] dataSlots_[0];
		}
		dataSlots_[0] = new byte[size];
	}
	{
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
		numElements_ui_ = numArrayElements_ * numInstances_;
		numElements_i_ = static_cast<int32_t>(numElements_ui_);
		inputSize_ = dataSize_bytes;
		writeUnlockAll(writeClientData_(data));
	} else if (data) {
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
		numElements_ui_ = numArrayElements_ * numVertices_;
		numElements_i_ = static_cast<int32_t>(numElements_ui_);
		inputSize_ = dataSize_bytes;
		writeUnlockAll(writeClientData_(data));
	} else if (data) {
		auto mapped = mapClientDataRaw(ShaderData::WRITE);
		std::memcpy(mapped.w, data, dataSize_bytes);
	}
}

bool ShaderInput::writeClientData_(const byte *data) {
	if (data) {
		// NOTE: writeLockAll locks slot 0 last, so we know it will be the active slot when we unlock.
		std::memcpy(dataSlots_[0], data, inputSize_);
		return true;
	} else {
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
	if (static_cast<uint32_t>(stride_) == elementSize_) {
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

	RenderState::get()->arrayBuffer().apply(buffer());
	byte *serverData = (byte *) glMapBufferRange(
			GL_ARRAY_BUFFER,
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

	glUnmapBuffer(GL_ARRAY_BUFFER);
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
	if (in->isBufferBlock()) {
		auto oldBlock = dynamic_cast<BufferBlock *>(in.get());
		if (oldBlock->isUniformBlock()) {
			auto newBlock = ref_ptr<UBO>::alloc(in->name(), oldBlock->usage());
			for (auto &namedInput: oldBlock->blockInputs()) {
				newBlock->addBlockInput(create(namedInput.in_), namedInput.name_);
			}
			return newBlock;
		}
		if (oldBlock->isShaderStorageBlock()) {
			auto newBlock = ref_ptr<SSBO>::alloc(in->name(), oldBlock->usage());
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
	cp->isBufferBlock_ = in->isBufferBlock_;
	cp->isConstant_ = in->isConstant_;
	cp->transpose_ = in->transpose_;
	cp->forceArray_ = in->forceArray_;
	cp->schema_ = in->schema_;
	if (in->hasClientData()) {
		// allocate memory for one slot, copy most recent data
		auto mapped = in->mapClientData(ShaderData::READ);
		cp->dataSlots_[0] = new byte[cp->inputSize_];
		if (copyData) {
			std::memcpy(cp->dataSlots_[0], mapped.r, cp->inputSize_);
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
