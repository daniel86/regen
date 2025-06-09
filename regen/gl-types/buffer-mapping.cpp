#include "buffer-mapping.h"

using namespace regen;

BufferMapping::BufferMapping(uint32_t storageFlags, Buffering storageBuffering) :
		GLObject(glGenBuffers, glDeleteBuffers, (int)storageBuffering),
		storageFlags_(storageFlags),
		storageBuffering_(storageBuffering),
		storageMappedData_((int)storageBuffering),
		hasData_((int)storageBuffering),
		storageClientData_(nullptr) {
	for (int i = 0; i < (int)storageBuffering; ++i) {
		storageMappedData_[i] = nullptr;
		hasData_[i] = false;
	}
	if (storageBuffering != Buffering::SINGLE_BUFFER) {
		// write into the buffer that was read last frame,
		// but delay the read buffer by number of buffers in use.
		writeBufferIndex_ = 0;
		readBufferIndex_ = 1;
	}
}

BufferMapping::~BufferMapping() {
	// free client data
	delete[] storageClientData_;
	storageClientData_ = nullptr;
}

void BufferMapping::initializeMapping(GLuint numBytes) {
	auto *rs = RenderState::get();
	if (storageClientData_ != nullptr) {
		delete[] storageClientData_;
		for (int i = 0; i < (int)storageBuffering_; ++i) {
			if (storageMappedData_[i] != nullptr) {
				glUnmapBuffer(GL_COPY_READ_BUFFER);
				storageMappedData_[i] = nullptr;
			}
		}
	}
	// allocate CPU data
	storageClientData_ = new byte[numBytes];
	storageSize_ = numBytes;
	// allocate GPU data
	for (int i = 0; i < (int)storageBuffering_; ++i) {
		// allocate buffer
		rs->pixelPackBuffer().push(ids_[i]);
		glBufferStorage(GL_PIXEL_PACK_BUFFER, numBytes, nullptr, storageFlags_);
		rs->pixelPackBuffer().pop();
		// map buffer
		if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
			rs->copyReadBuffer().push(ids_[i]);
			storageMappedData_[i] = (byte *) glMapBufferRange(
					GL_COPY_READ_BUFFER, 0, numBytes, storageFlags_);
			rs->copyReadBuffer().pop();
			if (!storageMappedData_[i]) {
				REGEN_WARN("failed to map buffer " << ids_[i] << " with flags " << storageFlags_ <<
					" and size " << numBytes/ 1024 << "kB");
				GL_ERROR_LOG();
			}
		} else {
			storageMappedData_[i] = nullptr;
		}
		hasData_[i] = false;
	}
}

void* BufferMapping::mapCopyWrite() {
	if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
		return storageMappedData_[writeBufferIndex_];
	}
	else {
		auto *rs = RenderState::get();
		rs->copyWriteBuffer().push(ids_[writeBufferIndex_]);
		auto mapped = (byte *) glMapBufferRange(
				GL_COPY_WRITE_BUFFER, 0, storageSize_, GL_MAP_WRITE_BIT);
		if (mapped) {
			return mapped;
		} else {
			rs->copyWriteBuffer().pop();
			return nullptr;
		}
	}
}

void BufferMapping::unmapCopyWrite(const ref_ptr<BufferReference> &outputBuffer, GLenum outputTarget) {
	if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
		// nothing to do, persistent mapping does not need unmapping
		return;
	}
	auto *rs = RenderState::get();
	if (!glUnmapBuffer(GL_COPY_WRITE_BUFFER)) {
		REGEN_WARN("failed to unmap buffer");
	}
	rs->copyWriteBuffer().pop();
	// at this point data was written to ids_[writeBufferIndex_].
	// next, copy the read buffer to the target buffer
	rs->copyWriteBuffer().push(ids_[readBufferIndex_]);
	rs->buffer(outputTarget).apply(outputBuffer->bufferID());
	glCopyBufferSubData(
			GL_COPY_WRITE_BUFFER,
			outputTarget,
			0,
			outputBuffer->address(),
			storageSize_);
	rs->copyWriteBuffer().pop();

	// swap buffers
	readBufferIndex_ = (readBufferIndex_ + 1) % (int)storageBuffering_;
	writeBufferIndex_ = (writeBufferIndex_ + 1) % (int)storageBuffering_;
}

void BufferMapping::updateMapping(const ref_ptr<BufferReference> &inputReference, GLenum inputTarget) {
	auto *rs = RenderState::get();
	// copy data from inputReference to write buffer
	rs->buffer(inputTarget).apply(inputReference->bufferID());
	rs->copyWriteBuffer().push(ids_[writeBufferIndex_]);
	glCopyBufferSubData(
			inputTarget,
			GL_COPY_WRITE_BUFFER,
			inputReference->address(),
			0,
			storageSize_);
	hasData_[writeBufferIndex_] = true;
	rs->copyWriteBuffer().pop();

	// copy data from read buffer to client data
	if (hasData_[readBufferIndex_]) {
		if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
			std::memcpy(storageClientData_, storageMappedData_[readBufferIndex_], storageSize_);
		} else {
			// map read buffer, copy data to client data, unmap read buffer
			rs->copyReadBuffer().push(ids_[readBufferIndex_]);
			auto mapped = (byte *) glMapBufferRange(
					GL_COPY_READ_BUFFER, 0, storageSize_, GL_MAP_READ_BIT);
			if (mapped) {
				std::memcpy(storageClientData_, mapped, storageSize_);
				glUnmapBuffer(GL_COPY_READ_BUFFER);
			}
			rs->copyReadBuffer().pop();
		}
	}

	// swap buffers
	readBufferIndex_ = (readBufferIndex_ + 1) % (int)storageBuffering_;
	writeBufferIndex_ = (writeBufferIndex_ + 1) % (int)storageBuffering_;
}
