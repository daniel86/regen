#include <regen/gl-types/render-state.h>
#include <regen/gl-types/shader-input.h>

#include "ubo.h"

using namespace regen;

UBO::UBO() :
	GLObject(glGenBuffers, glDeleteBuffers),
	allocatedSize_(0),
	requiredSize_(0),
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
}

GLboolean UBO::needsUpdate() const {
	if (requiredSize_ != allocatedSize_) { return GL_TRUE; }
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

void UBO::update(bool forceUpdate) {
	//bool needUpdate = forceUpdate || needsUpdate();
	//if (!needUpdate) { return; }

    glBindBuffer(GL_UNIFORM_BUFFER, id());
	if (requiredSize_ != allocatedSize_) {
		glBufferData(GL_UNIFORM_BUFFER,
			requiredSize_,
			nullptr,
			GL_DYNAMIC_DRAW);
		allocatedSize_ = requiredSize_;
	}
    for (auto & uniform : uniformMap_) {
		UBO_Input &input = uniform.second;
		if (forceUpdate || input.input->stamp() != input.lastStamp) {
    		glBufferSubData(
    			GL_UNIFORM_BUFFER,
    			input.offset,
    			input.input->inputSize(),
    			input.input->clientData());
			input.lastStamp = input.input->stamp();
		}
	}
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
	stamp_ += 1;
}

void UBO::bindBufferBase(GLuint bindingPoint) const {
	// TODO: does this need to be called each frame or only once when shader
	//         is active?
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, id());
}
