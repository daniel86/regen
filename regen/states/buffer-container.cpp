#include "buffer-container.h"
#include "regen/textures/texture-state.h"

using namespace regen;

BufferContainer::BufferContainer(
	const std::string &bufferName,
	const std::vector<NamedShaderInput> &namedInputs,
	BufferUsage bufferUsage)
		: State(),
		  namedInputs_(namedInputs),
		  bufferUsage_(bufferUsage),
		  bufferName_(bufferName) {
	allocateBuffers();
}

BufferContainer::BufferContainer(const std::string &bufferName, BufferUsage bufferUsage)
	: State(),
	  bufferUsage_(bufferUsage),
	  bufferName_(bufferName) {
}

void BufferContainer::addInput(const NamedShaderInput &namedInput) {
	if (namedInput.in_->isBufferBlock()) {
		auto block = ref_ptr<BufferBlock>::dynamicCast(namedInput.in_);
		for (auto &blockUniform: block->blockInputs()) {
			namedInputs_.emplace_back(blockUniform.in_, blockUniform.name_);
		}
	} else {
		namedInputs_.emplace_back(namedInput.in_, namedInput.name_);
	}
	isAllocated_ = false;
}

void BufferContainer::createUBO(const std::vector<NamedShaderInput> &namedInputs) {
	auto ubo = ref_ptr<UBO>::alloc("BufferContainer", BufferUsage::USAGE_DYNAMIC);
	for (auto &namedInput: namedInputs) {
		ubo->addBlockInput(namedInput.in_, namedInput.name_);
	}
	joinShaderInput(ubo);
	ubos_.push_back(ubo);
}

std::string BufferContainer::getNextBufferName() {
	return REGEN_STRING(bufferName_ << "_" << (ubos_.size()+tbos_.size()));
}

static GLenum getTBOFormat(GLenum dataType) {
	switch (dataType) {
		case GL_FLOAT:
			return GL_RGBA32F;
		case GL_HALF_FLOAT:
			return GL_RGBA16F;
		case GL_UNSIGNED_INT:
			return GL_RGBA32UI;
		case GL_INT:
			return GL_RGBA32I;
		default:
			return GL_RGBA8;
	}
}

void BufferContainer::createTBO(const NamedShaderInput &namedInput) {
	auto inputSize = namedInput.in_->dataTypeBytes() * namedInput.in_->valsPerElement();
	auto rs = RenderState::get();
	// create a TBO for the input
	auto tbo = ref_ptr<TBO>::alloc(BufferUsage::USAGE_DYNAMIC);
	auto ref = tbo->allocBytes(inputSize);
	if (!ref.get()) {
		REGEN_WARN("Unable to allocate TBO for input '" << namedInput.in_->name() << "'.");
		return;
	}
	tbos_.push_back(tbo);
	// attach buffer to texture
	rs->textureBuffer().push(ref->bufferID());
	auto tex = ref_ptr<TextureBuffer>::alloc(getTBOFormat(namedInput.in_->dataType()));
	tex->begin(rs);
	tex->attach(ref);
	tex->end(rs);
	rs->textureBuffer().pop();
	textureBuffers_.push_back(tex);
	// and make the TBO available as a texture to the shader
	auto texState = ref_ptr<TextureState>::alloc(tex, namedInput.name_);
	texState->set_mapping(TextureState::MAPPING_CUSTOM);
	texState->set_mapTo(TextureState::MAP_TO_CUSTOM);
	joinStates(texState);
	// TODO: add shader defines for accessing the buffer, or handle this in IO processor
	REGEN_WARN("TBO not fully implemented yet.");
	//shaderDefine(
	// 		REGEN_STRING(namedInput.name_),
	// 		REGEN_STRING("readFromTBO(tbo_" << namedInput.name_ << ")"));
}

void BufferContainer::allocateBuffers() {
	if (isAllocated_) return;
	isAllocated_ = true;

	auto maxUBOSize = getGLInteger(GL_MAX_UNIFORM_BLOCK_SIZE);
	auto maxTBOSize = getGLInteger(GL_MAX_TEXTURE_BUFFER_SIZE) * 16;
	unsigned int uboSize = 0u;
	std::vector<NamedShaderInput> nextUBOInputs;

	for (auto &namedInput: namedInputs_) {
		auto inputSize = namedInput.in_->dataTypeBytes() * namedInput.in_->valsPerElement();
		if (inputSize > maxTBOSize) {
			REGEN_WARN("Input '" << namedInput.in_->name() <<
				"' is too large for TBO. Size: " << inputSize/1024.0 << " KB.");
		}
		else if (inputSize > maxUBOSize) {
			createTBO(namedInput);
		}
		else {
			if (uboSize + inputSize > maxUBOSize) {
				createUBO(nextUBOInputs);
				nextUBOInputs.clear();
				uboSize = 0;
			}
			nextUBOInputs.push_back(namedInput);
			uboSize += inputSize;
		}
	}
	if (!nextUBOInputs.empty()) {
		createUBO(nextUBOInputs);
	}
}
