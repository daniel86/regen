#include <regen/gl-types/render-state.h>
#include <regen/gl-types/shader-input.h>

#include "ubo.h"

using namespace regen;

UBO::UBO() :
	GLObject(glGenBuffers, glDeleteBuffers),
	allocatedSize_(0),
	requiredSize_(0) {
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

void UBO::update(bool forceUpdate) {
	if (requiredSize_ != allocatedSize_) { forceUpdate = true; }
	bool needUpdate = forceUpdate;
	if (!needUpdate) {
		for (auto & uniform : uniformMap_) {
			UBO_Input &input = uniform.second;
			if (input.input->stamp() != input.lastStamp) {
				needUpdate = true;
				break;
			}
		}
	}
	if (!needUpdate) { return; }

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
}

void UBO::bindBufferBase(GLuint bindingPoint) const {
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, id());
}
