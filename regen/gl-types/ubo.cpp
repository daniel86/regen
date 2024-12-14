#include <regen/gl-types/render-state.h>
#include <regen/gl-types/shader-input.h>

#include "ubo.h"

using namespace regen;

UBO::UBO() :
	GLObject(glGenBuffers, glDeleteBuffers),
	allocatedSize_(0),
	requiredSize_(0),
	requiresResize_(GL_FALSE),
	stamp_(0) {
}

void UBO::addUniform(const ref_ptr<ShaderInput> &input) {
	UBO_Input uboInput;
	uboInput.input = input;
	uboInput.offset = requiredSize_;
	uboInput.lastStamp = 0;
	uniformMap_[input->name()] = uboInput;
	uniforms_.push_back(input);
	requiredSize_ += input->inputSize();
	requiresResize_ = GL_TRUE;
}

GLboolean UBO::needsUpdate() const {
	if (requiresResize_) { return GL_TRUE; }
	for (auto & uniform : uniformMap_) {
		const UBO_Input &input = uniform.second;
		if (input.input->stamp() != input.lastStamp) {
			return GL_TRUE;
		}
	}
	return GL_FALSE;
}

GLuint UBO::stamp() const {
	if (needsUpdate()) {
		return stamp_ + 1;
	} else {
		return stamp_;
	}
}

void UBO::computePaddedSize() {
	requiredSize_ = 0;
	for (auto &uniform : uniforms_) {
		// note we need to compute the "aligned" offset for std140 layout
		GLuint baseSize = uniform->inputSize() / uniform->elementCount();

		// Compute the alignment based on the type
		GLuint baseAlignment = baseSize;
		GLuint alignmentCount = 1;
		if (baseSize == 12) { // vec3
			baseAlignment = 16;
		}
		else if (baseSize == 48) { // mat3
			baseAlignment = 16;
			alignmentCount = 3;
		}
		else if (baseSize == 64) { // mat4
			baseAlignment = 16;
			alignmentCount = 4;
		}
		else if (uniform->elementCount() > 1) {
			baseAlignment = 16;
		}

		// Align the offset to the required alignment
		if (requiredSize_ % baseAlignment != 0) {
			requiredSize_ += baseAlignment - (requiredSize_ % baseAlignment);
		}
		uniformMap_[uniform->name()].offset = requiredSize_;
		if (uniform->elementCount() > 1) {
			requiredSize_ += baseAlignment * alignmentCount * uniform->elementCount();
		} else {
			requiredSize_ += uniform->inputSize();
		}
	}
}

void UBO::update(bool forceUpdate) {
	bool needUpdate = forceUpdate || needsUpdate();
	if (!needUpdate) { return; }

	glBindBuffer(GL_UNIFORM_BUFFER, id());

	if (requiresResize_) {
		computePaddedSize();
		glBufferData(GL_UNIFORM_BUFFER,
					 requiredSize_,
					 nullptr,
					 GL_DYNAMIC_DRAW);
		allocatedSize_ = requiredSize_;
		forceUpdate = GL_TRUE;
		requiresResize_ = GL_FALSE;
	}

	void* bufferData = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	if (bufferData) {
		for (auto & uniform : uniforms_) {
			UBO_Input &input = uniformMap_[uniform->name()];
			if (forceUpdate || input.input->stamp() != input.lastStamp) {
				if (!input.input->clientData()) {
					REGEN_WARN("UBO::update: no client data for " << input.input->name());
					continue;
				}
				memcpy(static_cast<char*>(bufferData) + input.offset,
					   input.input->clientData(),
					   input.input->inputSize());
				/*glBufferSubData(
						GL_UNIFORM_BUFFER,
						input.offset,
						input.input->inputSize(),
						input.input->clientData());*/
				input.lastStamp = input.input->stamp();
			}
		}
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	} else {
		REGEN_WARN("UBO::update: failed to map buffer");
	}

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
	stamp_ += 1;
}

void UBO::bindBufferBase(GLuint bindingPoint) const {
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, id());
}
