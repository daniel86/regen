#include "buffer-container.h"
#include "regen/textures/texture-state.h"

using namespace regen;

BufferContainer::BufferContainer(
	std::string_view bufferName,
	const std::vector<NamedShaderInput> &namedInputs,
	const BufferUpdateFlags &hints)
		: State(),
		  namedInputs_(namedInputs),
		  bufferUpdateHints_(hints),
		  bufferName_(bufferName) {
	updateBuffer();
}

BufferContainer::BufferContainer(std::string_view bufferName, const BufferUpdateFlags &hints)
	: State(),
	  bufferUpdateHints_(hints),
	  bufferName_(bufferName) {
}

void BufferContainer::addInput(const ref_ptr<ShaderInput> &input, std::string_view name) {
	if (input->isBufferBlock()) {
		auto block = ref_ptr<BufferBlock>::dynamicCast(input);
		for (auto &blockUniform: block->stagedInputs()) {
			namedInputs_.emplace_back(blockUniform.in_, blockUniform.name_);
		}
	} else {
		namedInputs_.emplace_back(input, std::string(name));
	}
	isAllocated_ = false;
}

std::string BufferContainer::getNextBufferName() {
	return REGEN_STRING(bufferName_ << "_" << (ubos_.size()+tbos_.size()));
}

void BufferContainer::createUBO(const std::vector<NamedShaderInput> &namedInputs) {
	auto ubo = ref_ptr<UBO>::alloc(getNextBufferName(), bufferUpdateHints_);
	if (bufferingMode_.has_value()) {
		ubo->setBufferingMode(bufferingMode_.value());
	}
	for (auto &namedInput: namedInputs) {
		ubo->addStagedInput(namedInput.in_, namedInput.name_);
		bufferObjectOfInput_[namedInput.in_.get()] = ubo;
	}
	ubo->update();
	setInput(ubo);
	ubos_.push_back(ubo);
}

void BufferContainer::createSSBO(const std::vector<NamedShaderInput> &namedInputs) {
	auto ssbo = ref_ptr<SSBO>::alloc(getNextBufferName(), bufferUpdateHints_);
	if (bufferingMode_.has_value()) {
		ssbo->setBufferingMode(bufferingMode_.value());
	}
	for (auto &namedInput: namedInputs) {
		ssbo->addStagedInput(namedInput.in_, namedInput.name_);
		bufferObjectOfInput_[namedInput.in_.get()] = ssbo;
	}
	ssbo->update();
	setInput(ssbo);
	ssbos_.push_back(ssbo);
}

void BufferContainer::createTBO(const NamedShaderInput &namedInput) {
	// create a TBO for the input
	auto tbo = ref_ptr<TBO>::alloc(
			getNextBufferName(),
			namedInput.in_->dataType(),
			bufferUpdateHints_);
	tbos_.push_back(tbo);
	textureBuffers_.push_back(tbo->tboTexture());
	tbo->addStagedInput(namedInput.in_, namedInput.name_);
	tbo->update();

	// and make the TBO available as a texture to the shader
	auto texState = ref_ptr<TextureState>::alloc(tbo->tboTexture(), namedInput.name_);
	texState->set_mapping(TextureState::MAPPING_CUSTOM);
	texState->set_mapTo(TextureState::MAP_TO_CUSTOM);
	joinStates(texState);
	// add shader defines for accessing the buffer
	auto shaderType = glenum::glslDataType(
			namedInput.in_->baseType(),
			namedInput.in_->valsPerElement());
	texState->shaderInclude(REGEN_STRING("regen.buffer.tbo." << shaderType));
	texState->shaderDefine(
	 		REGEN_STRING("in_" << namedInput.name_),
	 		REGEN_STRING("tboRead_" << shaderType << "(tbo_" << namedInput.name_ << ", int(regen_InstanceID))"));
	texState->shaderDefine(
			 REGEN_STRING("fetch_" << namedInput.name_ << "(i)"),
			 REGEN_STRING("tboRead_" << shaderType << "(tbo_" << namedInput.name_ << ", int(i))"));
	if (namedInput.in_->numInstances() > 1) {
		texState->shaderDefine("HAS_INSTANCES", "TRUE");
	}
	bufferObjectOfInput_[namedInput.in_.get()] = tbo;
}

void BufferContainer::updateBuffer() {
	if (isAllocated_) return;
	isAllocated_ = true;

	static auto maxUBOSize = static_cast<uint32_t>(glGetInteger(GL_MAX_UNIFORM_BLOCK_SIZE));
	static auto maxTBOSize = static_cast<uint32_t>(glGetInteger(GL_MAX_TEXTURE_BUFFER_SIZE)) * 16;
	unsigned int uboSize = 0u;
	std::vector<NamedShaderInput> nextUBOInputs;
	std::vector<NamedShaderInput> nextSSBOInputs;

	for (auto &namedInput: namedInputs_) {
		auto inputSize = namedInput.in_->inputSize();
		if (namedInput.in_->serverAccessMode() == BUFFER_GPU_WRITE || inputSize > maxTBOSize) {
			// create SSBO for large inputs, or if usage is WRITE (i.e. the buffer is written to from a shader).
			nextSSBOInputs.push_back(namedInput);
		}
		else if (inputSize > maxUBOSize) {
			createTBO(namedInput);
			if (namedInput.in_->numInstances() > 1) {
				set_numInstances(namedInput.in_->numInstances());
			}
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
	if (!nextSSBOInputs.empty()) {
		createSSBO(nextSSBOInputs);
	}
}

ref_ptr<StagedBuffer> BufferContainer::getBufferObject(const ref_ptr<ShaderInput> &input) {
	return bufferObjectOfInput_[input.get()];
}

void BufferContainer::printLayout() {
	REGEN_INFO("buffer " << bufferName_);
	for (auto &ubo: ubos_) {
		std::stringstream stream;
		stream << "  [ubo] " << ubo->name() << ":";
		for (auto &input: ubo->stagedInputs()) {
			stream << " " << input.name_ << ": " << input.in_->inputSize() / 1024.0 << "kB";
		}
		REGEN_INFO(stream.str());
	}
	for (auto &ssbo: ssbos_) {
		std::stringstream stream;
		stream << "  [ssbo] " << ssbo->name() << ":";
		for (auto &input: ssbo->stagedInputs()) {
			stream << " " << input.name_ << ": " << input.in_->inputSize() / 1024.0 << "kB";
		}
		REGEN_INFO(stream.str());
	}
	for (auto &tbo: tbos_) {
		std::stringstream stream;
		stream << "  [tbo] " << tbo->tboTexture()->name() << ":";
		stream << tbo->allocatedSize() / 1024.0 << "kB";
		REGEN_INFO(stream.str());
	}
}
