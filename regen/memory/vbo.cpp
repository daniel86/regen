#include "vbo.h"

using namespace regen;

VBO::VBO(BufferTarget target, const BufferUpdateFlags &hints, VertexLayout vertexLayout)
		: BufferObject(target, hints),
		  vertexLayout_(vertexLayout) {
	// set default storage flags
	flags_.mapMode = BUFFER_MAP_DISABLED;
	// default case: mesh data is loaded from CPU and written to GPU
	flags_.accessMode = BUFFER_CPU_WRITE;
}

ref_ptr<BufferReference> &VBO::alloc(const ref_ptr<ShaderInput> &att) {
	std::list<ref_ptr<ShaderInput> > atts;
	atts.push_back(att);
	return allocSequential(atts);
}

ref_ptr<BufferReference> &VBO::alloc(const std::list<ref_ptr<ShaderInput>> &attributes) {
	if (vertexLayout_ == VERTEX_LAYOUT_INTERLEAVED) {
		return allocInterleaved(attributes);
	} else {
		return allocSequential(attributes);
	}
}

ref_ptr<BufferReference> &VBO::allocSequential(
		const std::list<ref_ptr<ShaderInput> > &attributes) {
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

ref_ptr<BufferReference> &VBO::allocInterleaved(
		const std::list<ref_ptr<ShaderInput> > &attributes) {
	const uint32_t numBytes = attributeSize(attributes);
	ref_ptr<BufferReference> &ref = adoptBufferRange(numBytes);
	if (ref->allocatedSize() < numBytes) return ref;
	const uint32_t startByte = ref->address();
	// get the attribute struct size
	uint32_t attributeVertexSize = 0;
	uint32_t numVertices = attributes.front()->numVertices();
	byte *data = new byte[numBytes];

	uint32_t currOffset = startByte;
	for (const auto &attribute: attributes) {
		ShaderInput *att = attribute.get();

		att->set_buffer(ref->bufferID(), ref);
		if (att->divisor() == 0) {
			attributeVertexSize += att->elementSize();

			att->set_offset(currOffset);
			currOffset += att->elementSize();
		}
	}

	currOffset = (currOffset - startByte) * numVertices;
	for (const auto &attribute: attributes) {
		ShaderInput *att = attribute.get();
		if (att->divisor() == 0) {
			att->set_stride(static_cast<int>(attributeVertexSize));
		} else {
			// add instanced attributes to the end of the buffer
			att->set_stride(static_cast<int>(att->elementSize()));
			att->set_offset(currOffset + startByte);
			if (att->hasClientData()) {
				auto m = att->mapClientDataRaw(BUFFER_GPU_READ);
				std::memcpy(data + currOffset, m.r, att->inputSize());
			}
			currOffset += att->inputSize();
		}
	}

	uint32_t count = 0;
	for (uint32_t i = 0; i < numVertices; ++i) {
		for (const auto &attribute: attributes) {
			ShaderInput *att = attribute.get();
			if (att->divisor() != 0) { continue; }

			// size of a value for a single vertex in bytes
			uint32_t valueSize = att->valsPerElement() * att->dataTypeBytes() * att->numArrayElements();
			// copy data
			if (att->hasClientData()) {
				auto m = att->mapClientDataRaw(BUFFER_GPU_READ);
				std::memcpy(data + count, m.r + i * valueSize, valueSize);
			}
			count += valueSize;
		}
	}

	glNamedBufferSubData(ref->bufferID(), startByte, numBytes, data);
	delete[]data;
	return ref;
}
