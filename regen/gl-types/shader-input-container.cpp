/*
 * shader-input-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>

#include "shader-input-container.h"

using namespace regen;

ShaderInputContainer::ShaderInputContainer(VBO::Usage usage)
		: numVertices_(0),
		  vertexOffset_(0),
		  numInstances_(1),
		  numIndices_(0),
		  maxIndex_(0) {
	uploadLayout_ = LAYOUT_LAST;
	inputBuffer_ = ref_ptr<VBO>::alloc(usage);
}

ShaderInputContainer::ShaderInputContainer(
		const ref_ptr<ShaderInput> &in, const std::string &name, VBO::Usage usage)
		: numVertices_(0),
		  vertexOffset_(0),
		  numInstances_(1),
		  numIndices_(0) {
	uploadLayout_ = LAYOUT_LAST;
	inputBuffer_ = ref_ptr<VBO>::alloc(usage);
	setInput(in, name);
}

ShaderInputContainer::~ShaderInputContainer() {
	while (!inputs_.empty()) { removeInput(inputs_.begin()->name_); }
}

ref_ptr<ShaderInput> ShaderInputContainer::getInput(const std::string &name) const {
	for (auto it = inputs_.begin(); it != inputs_.end(); ++it) {
		if (name.compare(it->name_) == 0) return it->in_;
	}
	return {};
}

GLboolean ShaderInputContainer::hasInput(const std::string &name) const { return inputMap_.count(name) > 0; }

void ShaderInputContainer::begin(DataLayout layout) {
	uploadLayout_ = layout;
}

VBOReference ShaderInputContainer::end() {
	VBOReference ref;
	if (!uploadAttributes_.empty()) {
		if (uploadLayout_ == SEQUENTIAL) { ref = inputBuffer_->allocSequential(uploadAttributes_); }
		else if (uploadLayout_ == INTERLEAVED) { ref = inputBuffer_->allocInterleaved(uploadAttributes_); }
		uploadAttributes_.clear();
	}
	uploadInputs_.clear();
	uploadLayout_ = LAYOUT_LAST;
	return ref;
}

ShaderInputList::const_iterator ShaderInputContainer::setInput(
		const ref_ptr<ShaderInput> &in, const std::string &name) {
	const std::string &inputName = (name.empty() ? in->name() : name);

	if (in->isVertexAttribute() && in->numVertices() > numVertices_) { numVertices_ = in->numVertices(); }
	if (in->numInstances() > 1) { numInstances_ = in->numInstances(); }

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

ref_ptr<VBO::Reference> ShaderInputContainer::setIndices(const ref_ptr<ShaderInput> &indices, GLuint maxIndex) {
	indices_ = indices;
	numIndices_ = indices_->numVertices();
	maxIndex_ = maxIndex;
	return inputBuffer_->alloc(indices_);
}

void ShaderInputContainer::set_indexOffset(GLuint v) {
	if (indices_.get()) { indices_->set_offset(v); }
}

GLuint ShaderInputContainer::indexBuffer() const { return indices_.get() ? indices_->buffer() : 0; }

void ShaderInputContainer::removeInput(const ref_ptr<ShaderInput> &in) {
	inputMap_.erase(in->name());
	removeInput(in->name());
}

void ShaderInputContainer::removeInput(const std::string &name) {
	ShaderInputList::iterator it;
	for (it = inputs_.begin(); it != inputs_.end(); ++it) {
		if (it->name_ == name) { break; }
	}
	if (it == inputs_.end()) { return; }

	if (uploadLayout_ != LAYOUT_LAST) {
		VBOReference ref = it->in_->bufferIterator();
		if (ref.get()) {
			inputBuffer_->free(ref.get());
			it->in_->set_buffer(0u, VBOReference());
		}
	}

	inputs_.erase(it);
}

void ShaderInputContainer::drawArrays(GLenum primitive) {
	glDrawArrays(primitive, vertexOffset_, numVertices_);
}

void ShaderInputContainer::drawArraysInstanced(GLenum primitive) {
	glDrawArraysInstancedEXT(
			primitive,
			vertexOffset_,
			numVertices_,
			numInstances_);
}

void ShaderInputContainer::drawElements(GLenum primitive) {
	glDrawElements(
			primitive,
			numIndices_,
			indices_->dataType(),
			BUFFER_OFFSET(indices_->offset()));
}

void ShaderInputContainer::drawElementsInstanced(GLenum primitive) {
	glDrawElementsInstancedEXT(
			primitive,
			numIndices_,
			indices_->dataType(),
			BUFFER_OFFSET(indices_->offset()),
			numInstances_);
}
