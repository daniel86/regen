/*
 * shader-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/light-state.h>
#include <regen/states/material-state.h>
#include <regen/gl-types/glsl/directive-processor.h>
#include <regen/gl-types/glsl/includer.h>

#include "shader-state.h"
#include "state-configurer.h"

using namespace regen;

ShaderState::ShaderState(const ref_ptr<Shader> &shader)
		: State(), shader_(shader) {
	set_isHidden((shader.get() == nullptr));
}

ShaderState::ShaderState()
		: State() {
	set_isHidden(true);
}

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
	std::map<GLenum, std::string> unprocessedCode;
	for (GLint i = 0; i < glenum::glslStageCount(); ++i) {
		loadStage(cfg.defines_, shaderKey, unprocessedCode, glenum::glslStages()[i]);
	}
	if (unprocessedCode.empty()) {
		REGEN_ERROR("Failed to load shader with key '" << shaderKey << "'");
		return GL_FALSE;
	}
	return createShader(cfg, unprocessedCode);
}

GLboolean ShaderState::createShader(const StateConfig &cfg, const std::vector<std::string> &shaderKeys) {
	std::map<GLenum, std::string> unprocessedCode;
	for (GLuint i = 0u; i < shaderKeys.size(); ++i) {
		loadStage(cfg.defines_, shaderKeys[i], unprocessedCode, glenum::glslStages()[i]);
	}
	if (unprocessedCode.empty()) {
		REGEN_ERROR("Failed to load shader with key '" << shaderKeys[0] << "'");
		return GL_FALSE;
	}
	return createShader(cfg, unprocessedCode);
}

GLboolean ShaderState::createShader(const StateConfig &cfg, const std::map<GLenum, std::string> &unprocessedCode) {
	std::map<GLenum, std::string> processedCode;

	PreProcessorConfig preProcessCfg(
			cfg.version(),
			unprocessedCode,
			cfg.defines_,
			cfg.functions_,
			cfg.inputs_,
			cfg.includes_);
	Shader::preProcess(processedCode, preProcessCfg);
	shader_ = ref_ptr<Shader>::alloc(processedCode);
	// setup transform feedback attributes
	shader_->setTransformFeedback(cfg.feedbackAttributes_, cfg.feedbackMode_, cfg.feedbackStage_);

	if (!shader_->compile()) {
		REGEN_ERROR("Shader failed to compiled.");
		for (auto it = processedCode.begin(); it != processedCode.end(); ++it) {
			REGEN_DEBUG("Shader code failed to compile:\n" << it->second);
		}
		return GL_FALSE;
	}
	if (!shader_->link()) {
		REGEN_ERROR("Shader failed to link.");
		for (auto it = processedCode.begin(); it != processedCode.end(); ++it) {
			REGEN_DEBUG("Shader code failed to link:\n" << it->second);
		}
	} else {
		set_isHidden(false);
	}

	shader_->setInputs(cfg.inputs_);
	for (const auto & texture : cfg.textures_) {
		shader_->setTexture(texture.second, texture.first);
	}

	return GL_TRUE;
}

const ref_ptr<Shader> &ShaderState::shader() const { return shader_; }

void ShaderState::set_shader(const ref_ptr<Shader> &shader) { shader_ = shader; }

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

ref_ptr<Shader> ShaderState::findShader(State *s) {
	for (auto it = s->joined().rbegin(); it != s->joined().rend(); ++it) {
		ref_ptr<Shader> out = findShader((*it).get());
		if (out.get() != nullptr) return out;
	}

	auto *shaderState = dynamic_cast<ShaderState *>(s);
	if (shaderState != nullptr) return shaderState->shader();

	auto *hasShader = dynamic_cast<HasShader *>(s);
	if (hasShader != nullptr) return hasShader->shaderState()->shader();

	return {};
}

ref_ptr<Shader> ShaderState::findShader(StateNode *n) {
	ref_ptr<Shader> out = findShader(n->state().get());
	if (out.get() == nullptr && n->hasParent()) {
		return findShader(n->parent());
	} else {
		return out;
	}
}

ref_ptr<ShaderState> ShaderState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	if (!input.hasAttribute("key") && !input.hasAttribute("code")) {
		REGEN_WARN("Ignoring " << input.getDescription() << " without shader input.");
		return {};
	}
	ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();

	const std::string shaderKey = input.hasAttribute("key") ?
								  input.getValue("key") : input.getValue("code");
	StateConfigurer stateConfigurer;
	stateConfigurer.addNode(ctx.parent().get());
	stateConfigurer.addState(shaderState.get());
	shaderState->createShader(stateConfigurer.cfg(), shaderKey);

	return shaderState;
}
