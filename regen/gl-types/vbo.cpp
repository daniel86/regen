#include "vbo.h"

using namespace regen;

VBO::VBO(BufferTarget target, BufferUsage usage)
		: BufferObject(target, usage) {
}

ref_ptr<BufferReference> &VBO::alloc(const ref_ptr<ShaderInput> &att) {
	std::list<ref_ptr<ShaderInput> > atts;
	atts.push_back(att);
	return allocSequential(atts);
}

ref_ptr<BufferReference> &VBO::allocInterleaved(
		const std::list<ref_ptr<ShaderInput> > &attributes) {
	GLuint numBytes = attributeSize(attributes);
	ref_ptr<BufferReference> &ref = createReference(numBytes);
	if (ref->allocatedSize() < numBytes) return ref;
	GLuint offset = ref->address();
	// set buffer sub data
	uploadInterleaved(offset, offset + numBytes, attributes, ref);
	return ref;
}

ref_ptr<BufferReference> &VBO::allocSequential(
		const std::list<ref_ptr<ShaderInput> > &attributes) {
	GLuint numBytes = attributeSize(attributes);
	ref_ptr<BufferReference> &ref = createReference(numBytes);
	if (ref->allocatedSize() < numBytes) return ref;
	GLuint offset = ref->address();
	// set buffer sub data
	uploadSequential(offset, offset + numBytes, attributes, ref);
	return ref;
}

void VBO::uploadSequential(
		GLuint startByte,
		GLuint endByte,
		const std::list<ref_ptr<ShaderInput> > &attributes,
		ref_ptr<BufferReference> &ref) {
	GLuint bufferSize = endByte - startByte;
	GLuint currOffset = 0;
	byte *data = new byte[bufferSize];

	for (const auto &attribute: attributes) {
		ShaderInput *att = attribute.get();
		att->set_offset(currOffset + startByte);
		att->set_stride(att->elementSize());
		att->set_buffer(ref->bufferID(), ref);
		// copy data
		if (att->hasClientData()) {
			std::memcpy(
					data + currOffset,
					att->mapClientDataRaw(ShaderData::READ).r,
					att->inputSize()
			);
		}
		currOffset += att->inputSize();
	}

	glNamedBufferSubData(ref->bufferID(), startByte, bufferSize, data);
	delete[]data;
}

void VBO::uploadInterleaved(
		GLuint startByte,
		GLuint endByte,
		const std::list<ref_ptr<ShaderInput> > &attributes,
		ref_ptr<BufferReference> &ref) {
	GLuint bufferSize = endByte - startByte;
	GLuint currOffset = startByte;
	// get the attribute struct size
	GLuint attributeVertexSize = 0;
	GLuint numVertices = attributes.front()->numVertices();
	byte *data = new byte[bufferSize];

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
				std::memcpy(
						data + currOffset,
						att->mapClientDataRaw(ShaderData::READ).r,
						att->inputSize()
				);
			}
			currOffset += att->inputSize();
		}
	}

	GLuint count = 0;
	for (GLuint i = 0; i < numVertices; ++i) {
		for (const auto &attribute: attributes) {
			ShaderInput *att = attribute.get();
			if (att->divisor() != 0) { continue; }

			// size of a value for a single vertex in bytes
			GLuint valueSize = att->valsPerElement() * att->dataTypeBytes() * att->numArrayElements();
			// copy data
			if (att->hasClientData()) {
				std::memcpy(
						data + count,
						att->mapClientDataRaw(ShaderData::READ).r + i * valueSize,
						valueSize
				);
			}
			count += valueSize;
		}
	}

	glNamedBufferSubData(ref->bufferID(), startByte, bufferSize, data);
	delete[]data;
}
