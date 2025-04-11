/*
 * shader-configurer.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <regen/gl-types/input-container.h>
#include <regen/states/light-state.h>
#include <regen/meshes/mesh-state.h>
#include <regen/utility/string-util.h>

#include "state-configurer.h"

using namespace regen;

StateConfig StateConfigurer::configure(const StateNode *node) {
	StateConfigurer configurer;
	configurer.addNode(node);
	return configurer.cfg_;
}

StateConfig StateConfigurer::configure(const State *state) {
	StateConfigurer configurer;
	configurer.addState(state);
	return configurer.cfg_;
}

/////////////
/////////////

StateConfigurer::StateConfigurer(const StateConfig &cfg)
		: cfg_(cfg),
		  numLights_(0) {}

StateConfigurer::StateConfigurer()
		: numLights_(0) {
	// default is using separate attributes.
	cfg_.feedbackMode_ = GL_SEPARATE_ATTRIBS;
	cfg_.feedbackStage_ = GL_VERTEX_SHADER;
	// sets the minimum version
	cfg_.setVersion(330);
	// initially no lights added
	define("NUM_LIGHTS", "0");
}

StateConfig &StateConfigurer::cfg() { return cfg_; }

void StateConfigurer::setVersion(GLuint version) { cfg_.setVersion(version); }

void StateConfigurer::addNode(const StateNode *node) {
	if (node->hasParent()) {
		addNode(node->parent());
	}
	addState(node->state().get());
}

void StateConfigurer::addInput(const std::string &name, const ref_ptr<ShaderInput> &in, const std::string &type) {
	auto needle = inputNames_.find(name);
	if (needle == inputNames_.end()) {
		cfg_.inputs_.emplace_back(in, name, type);
		auto it = cfg_.inputs_.end();
		--it;
		inputNames_[name] = it;
	} else {
		*needle->second = NamedShaderInput(in, name, type);
	}
}

void StateConfigurer::addState(const State *s) {
	const auto *x0 = dynamic_cast<const HasInput *>(s);
	const auto *x1 = dynamic_cast<const FeedbackSpecification *>(s);
	const auto *x2 = dynamic_cast<const TextureState *>(s);
	const auto *x3 = dynamic_cast<const StateSequence *>(s);

	if (x0 != nullptr) {
		const ref_ptr<InputContainer> &container = x0->inputContainer();

		// remember inputs, they will be enabled automatically
		// when the shader is enabled.
		for (const auto & it : container->inputs()) {
			addInput(it.name_, it.in_);

			std::queue<std::pair<const std::string&,ShaderInput*>> queue;
			queue.emplace(it.in_->name(), it.in_.get());

			while (!queue.empty()) {
				auto [name, in] = queue.front();
				const auto &inName = name.empty() ? in->name() : name;
				queue.pop();

				define(REGEN_STRING("HAS_" << inName), "TRUE");
				if (in->isVertexAttribute()) {
					define(REGEN_STRING("HAS_VERTEX_" << inName), "TRUE");
				}
				if (in->numInstances() > 1) {
					define("HAS_INSTANCES", "TRUE");
					cfg_.numInstances_ = in->numInstances();
				}

				if (in->isBufferBlock()) {
					auto block = dynamic_cast<BufferBlock*>(in);
					for (auto& blockUniform : block->blockInputs()) {
						queue.emplace(blockUniform.name_, blockUniform.in_.get());
					}
				}
			}
		}
		if (container->numInstances()>1) {
			define("HAS_INSTANCES", "TRUE");
			cfg_.numInstances_ = container->numInstances();
		}
	}
	if (x1) {
		cfg_.feedbackMode_ = x1->feedbackMode();
		cfg_.feedbackStage_ = x1->feedbackStage();
		for (const auto & it : x1->feedbackAttributes()) {
			cfg_.feedbackAttributes_.push_back(it->name());
		}
	}
	if (x2) {
		// map for loop index to texture id
		define(
				REGEN_STRING("TEX_ID" << cfg_.textures_.size()),
				REGEN_STRING(x2->stateID()));
		define(
				REGEN_STRING("TEX_ID_" << x2->name()),
				REGEN_STRING(x2->stateID()));
		// remember the number of textures used
		define("NUM_TEXTURES", REGEN_STRING(cfg_.textures_.size() + 1));
		cfg_.textures_[x2->name()] = x2->texture();
		addInput(x2->name(), x2->texture(), x2->samplerType());
	}

	setVersion(s->shaderVersion());
	addDefines(s->shaderDefines());
	addIncludes(s->shaderIncludes());
	addFunctions(s->shaderFunctions());

	if (x3) {
		// add global sequence state
		addState(x3->globalState().get());
		// do not add joined states of sequences
		return;
	}
	for (const auto & it : s->joined()) {
		addState(it.get());
	}
}

void StateConfigurer::addDefines(const std::map<std::string, std::string> &defines) {
	for (auto it = defines.begin(); it != defines.end(); ++it) {
		define(it->first, it->second);
	}
}

void StateConfigurer::addIncludes(const std::vector<std::string> &includes) {
	for (const auto & include : includes) {
		cfg_.includes_.push_back(include);
	}
}

void StateConfigurer::addFunctions(const std::map<std::string, std::string> &functions) {
	for (const auto & function : functions) {
		defineFunction(function.first, function.second);
	}
}

void StateConfigurer::define(const std::string &name, const std::string &value) { cfg_.defines_[name] = value; }

void
StateConfigurer::defineFunction(const std::string &name, const std::string &value) { cfg_.functions_[name] = value; }
