#include "element-buffer.h"

using namespace regen;

ElementBuffer::ElementBuffer(const BufferUpdateFlags &hints)
		: BufferObject(ELEMENT_ARRAY_BUFFER, hints) {
	// set default storage flags
	flags_.mapMode = BUFFER_MAP_DISABLED;
	// default case: mesh data is loaded from CPU and written to GPU
	flags_.accessMode = BUFFER_CPU_WRITE;
}

ref_ptr<BufferReference> &ElementBuffer::alloc(const ref_ptr<ShaderInput> &att) {
	const uint32_t numBytes = att->inputSize();
	elementRef_ = adoptBufferRange(numBytes);
	if (elementRef_->allocatedSize() < numBytes) return elementRef_;
	const uint32_t startByte = elementRef_->address();

	att->set_offset(startByte);
	att->set_stride(att->elementSize());
	att->set_buffer(elementRef_->bufferID(), elementRef_);
	// copy data
	if (att->hasClientData()) {
		auto mapped = att->mapClientDataRaw(BUFFER_GPU_READ);
		glNamedBufferSubData(
			elementRef_->bufferID(),
			startByte,
			numBytes,
			mapped.r);
	}

	elements_ = att;
	return elementRef_;
}
