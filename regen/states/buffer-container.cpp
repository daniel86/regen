#include "buffer-container.h"
#include "regen/textures/texture-state.h"

using namespace regen;

BufferContainer::BufferContainer(
	const std::string &bufferName,
	const std::vector<NamedShaderInput> &namedInputs,
	BufferUsage bufferUsage)
		: State(),
		  HasInput(ARRAY_BUFFER, USAGE_DYNAMIC),
		  namedInputs_(namedInputs),
		  bufferUsage_(bufferUsage),
		  bufferName_(bufferName) {
	updateBuffer();
}

BufferContainer::BufferContainer(const std::string &bufferName, BufferUsage bufferUsage)
	: State(),
	  HasInput(ARRAY_BUFFER, USAGE_DYNAMIC),
	  bufferUsage_(bufferUsage),
	  bufferName_(bufferName) {
}

void BufferContainer::addInput(const ref_ptr<ShaderInput> &input, const std::string &name) {
	if (input->isBufferBlock()) {
		auto block = ref_ptr<BufferBlock>::dynamicCast(input);
		for (auto &blockUniform: block->blockInputs()) {
			namedInputs_.emplace_back(blockUniform.in_, blockUniform.name_);
		}
	} else {
		namedInputs_.emplace_back(input, name);
	}
	isAllocated_ = false;
}

std::string BufferContainer::getNextBufferName() {
	return REGEN_STRING(bufferName_ << "_" << (ubos_.size()+tbos_.size()));
}

void BufferContainer::createUBO(const std::vector<NamedShaderInput> &namedInputs) {
	auto ubo = ref_ptr<UBO>::alloc(getNextBufferName(), BufferUsage::USAGE_DYNAMIC);
	for (auto &namedInput: namedInputs) {
		ubo->addBlockInput(namedInput.in_, namedInput.name_);
		bufferObjectOfInput_[namedInput.in_.get()] = ubo;
	}
	ubo->update();
	joinShaderInput(ubo);
	ubos_.push_back(ubo);
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
	auto tex = ref_ptr<TextureBuffer>::alloc(namedInput.in_->dataType());
	tex->begin(rs);
	tex->attach(ref);
	// initially upload the data to the TBO
	tbo->setBufferData(namedInput.in_);
	tex->end(rs);
	rs->textureBuffer().pop();
	textureBuffers_.push_back(tex);
	// and make the TBO available as a texture to the shader
	auto texState = ref_ptr<TextureState>::alloc(tex,
		REGEN_STRING("tbo_" << namedInput.name_));
	texState->set_mapping(TextureState::MAPPING_CUSTOM);
	texState->set_mapTo(TextureState::MAP_TO_CUSTOM);
	joinStates(texState);
	// add shader defines for accessing the buffer
	auto shaderType = glenum::glslDataType(
			namedInput.in_->baseType(),
			namedInput.in_->valsPerElement());
	shaderInclude(REGEN_STRING("regen.buffer.tbo." << shaderType));
	shaderDefine(
	 		REGEN_STRING("in_" << namedInput.name_),
	 		REGEN_STRING("tboRead_" << shaderType << "(tbo_" << namedInput.name_ << ", regen_InstanceID)"));
	bufferObjectOfInput_[namedInput.in_.get()] = tbo;
}

void BufferContainer::updateBuffer() {
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
		setInput(namedInput.in_);
	}
	if (!nextUBOInputs.empty()) {
		createUBO(nextUBOInputs);
	}
}

ref_ptr<BufferObject> BufferContainer::getBufferObject(const ref_ptr<ShaderInput> &input) {
	return bufferObjectOfInput_[input.get()];
}

void BufferContainer::enable(RenderState *rs) {
	updateBuffer();
	State::enable(rs);
}
