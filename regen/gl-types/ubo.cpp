#include <regen/gl-types/render-state.h>
#include <regen/gl-types/shader-input.h>
#include <optional>

#include "ubo.h"
#include "regen/scene/shader-input-processor.h"
#include "regen/scene/loading-context.h"
#include "gl-util.h"

using namespace regen;

UBO::UBO() :
		GLObject(glGenBuffers, glDeleteBuffers),
		allocatedSize_(0),
		requiredSize_(0),
		requiresResize_(GL_FALSE),
		stamp_(0) {
}

void UBO::addUniform(const ref_ptr<ShaderInput> &input, const std::string &name) {
	auto &uboInput = uboInputs_.emplace_back();
	uboInput.input = input;
	uboInput.offset = requiredSize_;
	uboInput.lastStamp = 0;
	uniforms_.emplace_back(input, name);
	requiredSize_ += input->inputSize();
	requiresResize_ = GL_TRUE;
}

void UBO::updateUniform(const ref_ptr<ShaderInput> &input) {
	for (auto &uboInput: uboInputs_) {
		if (uboInput.input.get() == input.get()) {
			uboInput.lastStamp = 0;
			requiresResize_ = GL_TRUE;
			return;
		}
	}
}

GLboolean UBO::needsUpdate() const {
	if (requiresResize_) { return GL_TRUE; }
	for (auto &uboInput: uboInputs_) {
		if (uboInput.input->stamp() != uboInput.lastStamp) {
			return GL_TRUE;
		}
	}
	return GL_FALSE;
}

void UBO::computePaddedSize() {
	requiredSize_ = 0;
	for (auto &uboInput: uboInputs_) {
		auto &uniform = uboInput.input;
		// note we need to compute the "aligned" offset for std140 layout
		GLuint numElements = uniform->numArrayElements() * uniform->numInstances();
		GLuint baseSize = uniform->inputSize() / numElements;

		// Compute the alignment based on the type
		GLuint baseAlignment = baseSize;
		GLuint alignmentCount = 1;
		if (baseSize == 12) { // vec3
			baseAlignment = 16;
		} else if (baseSize == 48) { // mat3
			baseAlignment = 16;
			alignmentCount = 3;
		} else if (baseSize == 64) { // mat4
			baseAlignment = 16;
			alignmentCount = 4;
		} else if (numElements > 1) {
			baseAlignment = 16;
		}

		// Align the offset to the required alignment
		if (requiredSize_ % baseAlignment != 0) {
			requiredSize_ += baseAlignment - (requiredSize_ % baseAlignment);
		}
		uboInput.offset = requiredSize_;
		if (numElements > 1) {
			requiredSize_ += baseAlignment * alignmentCount * numElements;
		} else {
			requiredSize_ += uniform->inputSize();
		}
	}
}

void UBO::updateAlignedData(UBO_Input &uboInput) {
	// the GL specification states that the stride between array elements must be
	// rounded up to 16 bytes.
	// this is quite ugly to do element-wise memcpy below, hence non 16-byte aligned
	// arrays should be avoided in general.
	auto &in = uboInput.input;
	auto numElements = in->numArrayElements() * in->numInstances();
	if (numElements == 1) {
		return;
	}
	auto elementSizeUnaligned = in->valsPerElement() * in->dataTypeBytes();
	if (elementSizeUnaligned % 16 == 0) {
		return;
	}
	auto elementSizeAligned = elementSizeUnaligned + (16 - elementSizeUnaligned % 16);
	auto dataSizeAligned = elementSizeAligned * numElements;
	if (dataSizeAligned != uboInput.alignedSize) {
		delete[] uboInput.alignedData;
		uboInput.alignedSize = dataSizeAligned;
		uboInput.alignedData = new byte[uboInput.alignedSize];
	}
	auto clientData = in->mapClientDataRaw(ShaderData::READ);
	auto *src = clientData.r;
	auto *dst = uboInput.alignedData;
	for (unsigned int i = 0; i < numElements; ++i) {
		memcpy(dst, src, elementSizeUnaligned);
		src += elementSizeUnaligned;
		dst += elementSizeAligned;
	}
}

void UBO::update(bool forceUpdate) {
	bool needUpdate = forceUpdate || needsUpdate();
	if (!needUpdate) { return; }
	std::unique_lock<std::mutex> lock(mutex_);

	glBindBuffer(GL_UNIFORM_BUFFER, id());

	if (requiresResize_) {
		computePaddedSize();
		glBufferData(GL_UNIFORM_BUFFER,
					 requiredSize_,
					 nullptr,
					 GL_STATIC_DRAW);
		allocatedSize_ = requiredSize_;
		forceUpdate = GL_TRUE;
		requiresResize_ = GL_FALSE;
	}

	void *bufferData = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	if (bufferData) {
		for (auto &uboInput: uboInputs_) {
			if (forceUpdate || uboInput.input->stamp() != uboInput.lastStamp) {
				if (!uboInput.input->hasClientData()) {
					continue;
				}
				// copy the data to the buffer.
				updateAlignedData(uboInput);
				if (uboInput.alignedData) {
					memcpy(static_cast<char *>(bufferData) + uboInput.offset,
						   uboInput.alignedData, uboInput.alignedSize);
				} else {
					auto mapped = uboInput.input->mapClientDataRaw(ShaderData::READ);
					memcpy(static_cast<char *>(bufferData) + uboInput.offset,
						   mapped.r,
						   uboInput.input->inputSize());
				}
				uboInput.lastStamp = uboInput.input->stamp();
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
	RenderState::get()->bindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, id());
}

ref_ptr<UBO> UBO::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto ubo = ref_ptr<UBO>::alloc();
	auto dummyState = ref_ptr<State>::alloc();

	for (auto &n: input.getChildren()) {
		if (n->getCategory() == "uniform" || n->getCategory() == "input") {
			auto uniform = scene::ShaderInputProcessor::createShaderInput(
					ctx.scene(), *n.get(), dummyState);
			if (uniform->isVertexAttribute()) {
				REGEN_WARN("UBO cannot contain vertex attributes. In node '" << n->getDescription() << "'.");
				continue;
			}
			auto name = n->getValue("name");
			ubo->addUniform(uniform, name);
		} else {
			REGEN_WARN("Unknown UBO child category '" << n->getCategory() << "'.");
		}
	}

	GL_ERROR_LOG();

	return ubo;
}
