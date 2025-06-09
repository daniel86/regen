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

bool BufferMapping::initializeMapping(GLuint numBytes, GLenum target) {
	auto *rs = RenderState::get();
	// allocate CPU data
	delete[] storageClientData_;
	storageClientData_ = new byte[numBytes];
	storageSize_ = numBytes;
	// allocate GPU data
	for (int i = 0; i < (int)storageBuffering_; ++i) {
		if (storageMappedData_[i]) {
			if (storageFlags_ & GL_MAP_WRITE_BIT) {
				rs->buffer(target).push(ids_[i]);
				glUnmapBuffer(target);
				rs->buffer(target).pop();
			} else {
				rs->copyReadBuffer().push(ids_[i]);
				glUnmapBuffer(GL_COPY_READ_BUFFER);
				rs->copyReadBuffer().pop();
			}
			if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
				glDeleteBuffers(1, &ids_[i]);
				glGenBuffers(1, &ids_[i]);
			}
		}

		// allocate buffer
		rs->buffer(target).push(ids_[i]);
		glBufferStorage(target, numBytes, nullptr, storageFlags_);
		rs->buffer(target).pop();

		// map buffer
		if (storageFlags_ & GL_MAP_PERSISTENT_BIT) {
			if (storageFlags_ & GL_MAP_WRITE_BIT) {
				rs->buffer(target).push(ids_[i]);
				storageMappedData_[i] = (byte *) glMapBufferRange(
						target, 0, numBytes, storageFlags_);
				rs->buffer(target).pop();
			} else {
				rs->copyReadBuffer().push(ids_[i]);
				storageMappedData_[i] = (byte *) glMapBufferRange(
						GL_COPY_READ_BUFFER, 0, numBytes, storageFlags_);
				rs->copyReadBuffer().pop();
			}
			if (!storageMappedData_[i]) {
				REGEN_WARN("failed to map buffer " << ids_[i] <<
					" with write flag " << (storageFlags_ & GL_MAP_WRITE_BIT) <<
					" read flag " << (storageFlags_ & GL_MAP_READ_BIT) <<
					" persistent flag " << (storageFlags_ & GL_MAP_PERSISTENT_BIT) <<
					" coherent flag " << (storageFlags_ & GL_MAP_COHERENT_BIT) <<
					" target " << std::hex << target << std::dec <<
					" and size " << numBytes/ 1024 << "kB");
				GL_ERROR_LOG();
				return false;
			}
		} else {
			storageMappedData_[i] = nullptr;
		}
		hasData_[i] = false;
	}
	return true;
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
	auto *rs = RenderState::get();
	if (!(storageFlags_ & GL_MAP_PERSISTENT_BIT)) {
		if (!glUnmapBuffer(GL_COPY_WRITE_BUFFER)) {
			REGEN_WARN("failed to unmap buffer");
		}
		rs->copyWriteBuffer().pop();
	}
	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
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
