#include "buffer-container.h"
#include "regen/textures/texture-state.h"

using namespace regen;

// TODO: would be nice if updateBuffer would not be needed.
//       at least information about shader inputs is needed early when shaders are compiled,
//       but the actual buffers do not need to be allocated then. So we could dynamically create
//       ShaderInput etc. and then make a deferred data allocation.
//       However, one workflow would be that a shader input is added to the container, and then
//       the input is changed, e.g. to n instances. however, input does not really have a reference
//       to the container where it is stored.

BufferContainer::BufferContainer(
	const std::string &bufferName,
	const std::vector<NamedShaderInput> &namedInputs,
	BufferUsage bufferUsage)
		: State(),
		  HasInput(ARRAY_BUFFER, BUFFER_USAGE_DYNAMIC_DRAW),
		  namedInputs_(namedInputs),
		  bufferUsage_(bufferUsage),
		  bufferName_(bufferName) {
	updateBuffer();
}

BufferContainer::BufferContainer(const std::string &bufferName, BufferUsage bufferUsage)
	: State(),
	  HasInput(ARRAY_BUFFER, BUFFER_USAGE_DYNAMIC_DRAW),
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
	auto ubo = ref_ptr<UBO>::alloc(getNextBufferName(), BUFFER_USAGE_DYNAMIC_DRAW);
	for (auto &namedInput: namedInputs) {
		ubo->addBlockInput(namedInput.in_, namedInput.name_);
		bufferObjectOfInput_[namedInput.in_.get()] = ubo;
	}
	ubo->update();
	joinShaderInput(ubo);
	ubos_.push_back(ubo);
}

void BufferContainer::createSSBO(const std::vector<NamedShaderInput> &namedInputs) {
	// TODO: allow more fine grained usage configuration
	auto ssbo = ref_ptr<SSBO>::alloc(getNextBufferName(), BUFFER_USAGE_STREAM_COPY);
	for (auto &namedInput: namedInputs) {
		ssbo->addBlockInput(namedInput.in_, namedInput.name_);
		bufferObjectOfInput_[namedInput.in_.get()] = ssbo;
	}
	ssbo->update();
	joinShaderInput(ssbo);
	ssbos_.push_back(ssbo);
}

void BufferContainer::createTBO(const NamedShaderInput &namedInput) {
	// create a TBO for the input
	auto tbo = ref_ptr<TBO>::alloc(BUFFER_USAGE_DYNAMIC_DRAW);
	tbo->setBufferInput(namedInput.in_);
	tbos_.push_back(tbo);
	textureBuffers_.push_back(tbo->tboTexture());
	// initially upload the data to the TBO
	tbo->updateTBO();

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
	bufferObjectOfInput_[namedInput.in_.get()] = tbo;
}

void BufferContainer::updateBuffer() {
	if (isAllocated_) return;
	isAllocated_ = true;

	static auto maxUBOSize = static_cast<uint32_t>(getGLInteger(GL_MAX_UNIFORM_BLOCK_SIZE));
	static auto maxTBOSize = static_cast<uint32_t>(getGLInteger(GL_MAX_TEXTURE_BUFFER_SIZE)) * 16;
	unsigned int uboSize = 0u;
	std::vector<NamedShaderInput> nextUBOInputs;
	std::vector<NamedShaderInput> nextSSBOInputs;

	for (auto &namedInput: namedInputs_) {
		auto inputSize = namedInput.in_->inputSize();
		if (namedInput.in_->gpuUsage() == ShaderData::WRITE || inputSize > maxTBOSize) {
			// create SSBO for large inputs, or if usage is WRITE (i.e. the buffer is written to from a shader).
			nextSSBOInputs.push_back(namedInput);
		}
		else if (inputSize > maxUBOSize) {
			createTBO(namedInput);
			if (namedInput.in_->numInstances() > 1) {
				inputContainer()->set_numInstances(namedInput.in_->numInstances());
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
			setInput(namedInput.in_);
		}
	}
	if (!nextUBOInputs.empty()) {
		createUBO(nextUBOInputs);
	}
	if (!nextSSBOInputs.empty()) {
		createSSBO(nextSSBOInputs);
	}
	GL_ERROR_LOG();
}

ref_ptr<BufferObject> BufferContainer::getBufferObject(const ref_ptr<ShaderInput> &input) {
	return bufferObjectOfInput_[input.get()];
}

void BufferContainer::enable(RenderState *rs) {
	updateBuffer();
	for (auto &tbo: tbos_) {
		// update TBO in case client data changed
		// TODO: UBO uses ShaderInput interface for update, would be good to unify!
		//         one option would be to only do it here, as probably UBO/TBO won't be
		//         used much without this container.
		tbo->updateTBO();
	}
	State::enable(rs);
}

void BufferContainer::printLayout() {
	REGEN_INFO("buffer " << bufferName_);
	for (auto &ubo: ubos_) {
		std::stringstream stream;
		stream << "  [ubo] " << ubo->name() << ":";
		for (auto &input: ubo->blockInputs()) {
			stream << " " << input.name_ << ": " << input.in_->inputSize() / 1024.0 << "kB";
		}
		REGEN_INFO(stream.str());
	}
	for (auto &ssbo: ssbos_) {
		std::stringstream stream;
		stream << "  [ssbo] " << ssbo->name() << ":";
		for (auto &input: ssbo->blockInputs()) {
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
