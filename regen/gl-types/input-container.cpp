#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>

#include "input-container.h"
#include "ubo.h"
#include "draw-command.h"

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

using namespace regen;

InputContainer::InputContainer(BufferTarget target, BufferUsage usage) {
	uploadLayout_ = LAYOUT_LAST;
	inputBuffer_ = ref_ptr<VBO>::alloc(target, usage);
}

InputContainer::InputContainer(
		const ref_ptr<ShaderInput> &in, const std::string &name, BufferUsage usage) {
	uploadLayout_ = LAYOUT_LAST;
	inputBuffer_ = ref_ptr<VBO>::alloc(ARRAY_BUFFER, usage);
	setInput(in, name);
}

InputContainer::~InputContainer() {
	while (!inputs_.empty()) { removeInput(inputs_.begin()->name_); }
}

ref_ptr<ShaderInput> InputContainer::getInput(const std::string &name) const {
	for (const auto &input: inputs_) {
		if (name == input.name_) return input.in_;
	}
	return {};
}

GLboolean InputContainer::hasInput(const std::string &name) const {
	return inputMap_.count(name) > 0;
}

void InputContainer::set_numInstances(GLuint v) {
	numInstances_ = static_cast<int32_t>(v);
	numVisibleInstances_ = static_cast<int32_t>(v);
}

void InputContainer::begin(DataLayout layout) {
	uploadLayout_ = layout;
}

ref_ptr<BufferReference> InputContainer::end() {
	ref_ptr<BufferReference> ref;
	if (!uploadAttributes_.empty()) {
		if (uploadLayout_ == SEQUENTIAL) {
			ref = inputBuffer_->allocSequential(uploadAttributes_);
		} else if (uploadLayout_ == INTERLEAVED) {
			ref = inputBuffer_->allocInterleaved(uploadAttributes_);
		}
		uploadAttributes_.clear();
	}
	uploadInputs_.clear();
	uploadLayout_ = LAYOUT_LAST;
	return ref;
}

ShaderInputList::const_iterator InputContainer::setInput(
		const ref_ptr<ShaderInput> &in, const std::string &name) {
	const std::string &inputName = (name.empty() ? in->name() : name);

	if (in->isVertexAttribute() && in->numVertices() > static_cast<uint32_t>(numVertices_)) {
		numVertices_ = static_cast<int>(in->numVertices());
	}
	if (in->numInstances() > 1) {
		numInstances_ = static_cast<int>(in->numInstances());
		numVisibleInstances_ = numInstances_;
	}
	// check for instances of attributes within UBO
	if (in->isBufferBlock()) {
		auto *block = dynamic_cast<BufferBlock *>(in.get());
		for (auto &namedInput: block->blockInputs()) {
			if (namedInput.in_->isVertexAttribute() &&
			    namedInput.in_->numVertices() > static_cast<uint32_t>(numVertices_)) {
				numVertices_ = static_cast<int>(namedInput.in_->numVertices());
			}
			if (namedInput.in_->numInstances() > 1) {
				numInstances_ = static_cast<int>(namedInput.in_->numInstances());
				numVisibleInstances_ = numInstances_;
			}
		}
	}

	if (inputMap_.count(inputName) > 0) {
		removeInput(inputName);
	} else { // insert into map of known attributes
		inputMap_.insert(inputName);
	}

	inputs_.emplace_front(in, inputName);

	if (uploadLayout_ != LAYOUT_LAST) {
		if (in->isVertexAttribute())
			uploadAttributes_.push_front(in);
		uploadInputs_.push_front(*inputs_.begin());
	}

	return inputs_.begin();
}

ref_ptr<BufferReference> InputContainer::setIndices(const ref_ptr<ShaderInput> &indices, GLuint maxIndex) {
	indices_ = indices;
	numIndices_ = static_cast<int32_t>(indices_->numVertices());
	maxIndex_ = maxIndex;
	return inputBuffer_->alloc(indices_);
}

void InputContainer::set_indexOffset(GLuint v) {
	if (indices_.get()) { indices_->set_offset(v); }
}

GLuint InputContainer::indexBuffer() const { return indices_.get() ? indices_->buffer() : 0; }

void InputContainer::removeInput(const ref_ptr<ShaderInput> &in) {
	inputMap_.erase(in->name());
	removeInput(in->name());
}

void InputContainer::removeInput(const std::string &name) {
	ShaderInputList::iterator it;
	for (it = inputs_.begin(); it != inputs_.end(); ++it) {
		if (it->name_ == name) { break; }
	}
	if (it == inputs_.end()) { return; }

	if (uploadLayout_ != LAYOUT_LAST) {
		auto &ref = it->in_->bufferIterator();
		if (ref.get()) {
			BufferObject::free(ref.get());
			it->in_->set_buffer(0u, {});
		}
	}

	inputs_.erase(it);
}

void InputContainer::setIndirectDrawBuffer(const ref_ptr<SSBO> &indirectDrawBuffer, uint32_t baseDrawIdx) {
	indirectDrawBuffer_ = indirectDrawBuffer;
	baseDrawIdx_ = baseDrawIdx;
	if (indirectDrawBuffer_.get()) {
		indirectOffset_ = indirectDrawBuffer_->offset() + baseDrawIdx_ * sizeof(DrawCommand);
	} else {
		indirectOffset_ = 0u;
	}
}

void InputContainer::draw(GLenum primitive) const {
	glDrawArrays(primitive, vertexOffset_, numVertices_);
}

void InputContainer::drawIndexed(GLenum primitive) const {
	glDrawElements(
			primitive,
			numIndices_,
			indices_->baseType(),
			BUFFER_OFFSET(indices_->offset()));
}

void InputContainer::drawInstances(GLenum primitive) const {
	glDrawArraysInstancedEXT(
			primitive,
			vertexOffset_,
			numVertices_,
			numVisibleInstances_);
}

void InputContainer::drawInstancesIndexed(GLenum primitive) const {
	glDrawElementsInstancedEXT(
			primitive,
			numIndices_,
			indices_->baseType(),
			BUFFER_OFFSET(indices_->offset()),
			numVisibleInstances_);
}

void InputContainer::drawBaseInstances(GLenum primitive) const {
	glDrawArraysInstancedBaseInstance(
			primitive,
			vertexOffset_,
			numVertices_,
			numVisibleInstances_,
			baseInstance_);
}

void InputContainer::drawBaseInstancesIndexed(GLenum primitive) const {
	glDrawElementsInstancedBaseInstance(
			primitive,
			numIndices_,
			indices_->baseType(),
			BUFFER_OFFSET(indices_->offset()),
			numVisibleInstances_,
			baseInstance_);
}

void InputContainer::drawIndirect(GLenum primitive) const {
	glDrawArraysIndirect(
		primitive,
		BUFFER_OFFSET(indirectOffset_));
}

void InputContainer::drawIndirectIndexed(GLenum primitive) const {
	glDrawElementsIndirect(
			primitive,
			indices_->baseType(),
			BUFFER_OFFSET(indirectOffset_));
}

void InputContainer::drawMultiIndirect(GLenum primitive) const {
	glMultiDrawArraysIndirect(
		primitive,
		BUFFER_OFFSET(indirectOffset_),
		multiDrawCount_,
		sizeof(DrawCommand));
}

void InputContainer::drawMultiIndirectIndexed(GLenum primitive) const {
	glMultiDrawElementsIndirect(
		primitive,
		indices_->baseType(),
		BUFFER_OFFSET(indirectOffset_),
		multiDrawCount_,
		sizeof(DrawCommand));
}
