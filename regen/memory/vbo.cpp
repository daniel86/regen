#include "vbo.h"

using namespace regen;

VBO::VBO(BufferTarget target, const BufferUpdateFlags &hints)
		: BufferObject(target, hints) {
	// set default storage flags
	flags_.mapMode = BUFFER_MAP_DISABLED;
	// default case: mesh data is loaded from CPU and written to GPU
	flags_.accessMode = BUFFER_CPU_WRITE;
}

ref_ptr<BufferReference> &VBO::alloc(const std::vector<ref_ptr<ShaderInput>> &attributes) {
	const uint32_t numBytes = attributeSize(attributes);
	ref_ptr<BufferReference> &ref = adoptBufferRange(numBytes);
	if (ref->allocatedSize() < numBytes) return ref;
	const uint32_t startByte = ref->address();
	uint32_t currOffset = 0;

	for (const auto &att: attributes) {
		att->set_offset(currOffset + startByte);
		att->set_stride(att->elementSize());
		att->set_buffer(ref->bufferID(), ref);
		// copy data
		if (att->hasClientData()) {
			auto mapped = att->mapClientDataRaw(BUFFER_GPU_READ);
			glNamedBufferSubData(
					ref->bufferID(),
					currOffset + startByte,
					att->inputSize(),
					mapped.r);
		}
		currOffset += att->inputSize();
	}

	return ref;
}
