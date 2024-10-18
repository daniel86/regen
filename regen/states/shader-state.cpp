/*
 * shader-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <regen/utility/string-util.h>
#include <regen/states/light-state.h>
#include <regen/states/material-state.h>
#include <regen/states/texture-state.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/glsl/directive-processor.h>
#include <regen/gl-types/glsl/includer.h>

#include "shader-state.h"

using namespace regen;

ShaderState::ShaderState(const ref_ptr<Shader> &shader)
		: State(), shader_(shader) { isHidden_ = (shader.get() == nullptr); }

ShaderState::ShaderState()
		: State() { isHidden_ = GL_TRUE; }

void ShaderState::loadStage(
		const std::map<std::string, std::string> &shaderConfig,
		const std::string &effectName,
		std::map<GLenum, std::string> &code,
		GLenum stage) {
	std::string stageName = glenum::glslStageName(stage);
	std::string effectKey = REGEN_STRING(effectName << "." << glenum::glslStagePrefix(stage));
	std::string ignoreKey = REGEN_STRING("IGNORE_" << stageName);

	auto it = shaderConfig.find(ignoreKey);
	if (it != shaderConfig.end() && it->second == "TRUE") { return; }

	code[stage] = Includer::get().include(effectKey);
	// failed to include ?
	if (code[stage].empty()) { code.erase(stage); }
}

GLboolean ShaderState::createShader(const StateConfig &cfg, const std::string &shaderKey) {
	const std::list<NamedShaderInput> specifiedInput = cfg.inputs_;
	const std::map<std::string, ref_ptr<Texture> > &textures = cfg.textures_;
	const std::map<std::string, std::string> &shaderConfig = cfg.defines_;
	const std::map<std::string, std::string> &shaderFunctions = cfg.functions_;
	std::map<GLenum, std::string> unprocessedCode;
	std::map<GLenum, std::string> processedCode;

	for (GLint i = 0; i < glenum::glslStageCount(); ++i) {
		loadStage(shaderConfig, shaderKey, unprocessedCode, glenum::glslStages()[i]);
	}
	if (unprocessedCode.empty()) {
		REGEN_ERROR("Failed to load shader with key '" << shaderKey << "'");
		return GL_FALSE;
	}

	PreProcessorConfig preProcessCfg(cfg.version(),
									 unprocessedCode, shaderConfig,
									 shaderFunctions, specifiedInput);
	Shader::preProcess(processedCode, preProcessCfg);
	shader_ = ref_ptr<Shader>::alloc(processedCode);
	// setup transform feedback attributes
	shader_->setTransformFeedback(cfg.feedbackAttributes_, cfg.feedbackMode_, cfg.feedbackStage_);

	if (!shader_->compile()) {
		REGEN_ERROR("Shader '" << shaderKey << "' failed to compiled.");
		return GL_FALSE;
	}
	if (!shader_->link()) {
		REGEN_ERROR("Shader '" << shaderKey << "' failed to link.");
	}

	shader_->setInputs(specifiedInput);
	for (auto it = textures.begin(); it != textures.end(); ++it) {
		shader_->setTexture(it->second, it->first);
	}

	isHidden_ = GL_FALSE;

	return GL_TRUE;
}

const ref_ptr<Shader> &ShaderState::shader() const { return shader_; }

void ShaderState::set_shader(const ref_ptr<Shader>& shader) { shader_ = shader; }

void ShaderState::enable(RenderState *rs) {
	rs->shader().push(shader_->id());
	if (!rs->shader().isLocked()) {
		shader_->enable(rs);
	}
	State::enable(rs);
}

void ShaderState::disable(RenderState *rs) {
	State::disable(rs);
	rs->shader().pop();
}
