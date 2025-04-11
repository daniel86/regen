#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>

#include "input-container.h"
#include "ubo.h"

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

using namespace regen;

InputContainer::InputContainer(BufferTarget target, BufferUsage usage)
		: numVertices_(0),
		  vertexOffset_(0),
		  numInstances_(1),
		  numVisibleInstances_(1),
		  numIndices_(0),
		  maxIndex_(0) {
	uploadLayout_ = LAYOUT_LAST;
	inputBuffer_ = ref_ptr<VBO>::alloc(target, usage);
}

InputContainer::InputContainer(
		const ref_ptr<ShaderInput> &in, const std::string &name, BufferUsage usage)
		: numVertices_(0),
		  vertexOffset_(0),
		  numInstances_(1),
		  numVisibleInstances_(1),
		  numIndices_(0) {
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
	numInstances_ = v;
	numVisibleInstances_ = v;
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

	if (in->isVertexAttribute() && in->numVertices() > numVertices_) { numVertices_ = in->numVertices(); }
	if (in->numInstances() > 1) {
		numInstances_ = in->numInstances();
		numVisibleInstances_ = numInstances_;
	}
	// check for instances of attributes within UBO
	if (in->isBufferBlock()) {
		auto *block = dynamic_cast<BufferBlock *>(in.get());
		for (auto &namedInput: block->blockInputs()) {
			if (namedInput.in_->isVertexAttribute() && namedInput.in_->numVertices() > numVertices_) {
				numVertices_ = namedInput.in_->numVertices();
			}
			if (namedInput.in_->numInstances() > 1) {
				numInstances_ = namedInput.in_->numInstances();
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
	numIndices_ = indices_->numVertices();
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

void InputContainer::drawArrays(GLenum primitive) {
	glDrawArrays(primitive, vertexOffset_, numVertices_);
}

void InputContainer::drawArraysInstanced(GLenum primitive) {
	glDrawArraysInstancedEXT(
			primitive,
			vertexOffset_,
			numVertices_,
			numVisibleInstances_);
}

void InputContainer::drawElements(GLenum primitive) {
	glDrawElements(
			primitive,
			numIndices_,
			indices_->baseType(),
			BUFFER_OFFSET(indices_->offset()));
}

void InputContainer::drawElementsInstanced(GLenum primitive) {
	glDrawElementsInstancedEXT(
			primitive,
			numIndices_,
			indices_->baseType(),
			BUFFER_OFFSET(indices_->offset()),
			numVisibleInstances_);
}
