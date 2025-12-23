#include <regen/shading/light-state.h>
#include <regen/objects/mesh.h>
#include <regen/utility/strings.h>

#include "state-configurer.h"

#include "regen/gl/states/feedback-state.h"
#include "regen/textures/fbo-state.h"

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
	static const bool has_ARB_shader_viewport_layer_array =
		glewIsSupported("GL_ARB_shader_viewport_layer_array");
	// default is using separate attributes.
	cfg_.feedbackMode_ = GL_SEPARATE_ATTRIBS;
	cfg_.feedbackStage_ = GL_VERTEX_SHADER;
	// sets the minimum version
	cfg_.setVersion(460);
	// initially no lights added
	define("NUM_LIGHTS", "0");
	if (has_ARB_shader_viewport_layer_array) {
		// enable layer rendering in vertex shader
		define("ARB_shader_viewport_layer_array", "TRUE");
	}
}

StateConfig &StateConfigurer::cfg() { return cfg_; }

void StateConfigurer::setVersion(uint32_t version) { cfg_.setVersion(version); }

void StateConfigurer::addNode(const StateNode *node) {
	bool hadFBOBefore = hasFBO_;
	preAddState(node->state().get());
	bool hasFBO = hasFBO_;
	if (node->hasParent()) {
		addNode(node->parent());
	}
	hasFBO_ = hadFBOBefore;
	addState(node->state().get());
	hasFBO_ = hasFBO;
}

void StateConfigurer::addInput(const std::string &name,
		const ref_ptr<ShaderInput> &in,
		const std::string &type,
		const std::string &memberSuffix) {
	auto needle = inputNames_.find(name);
	if (needle == inputNames_.end()) {
		cfg_.inputs_.emplace_back(in, name, type, memberSuffix);
		auto it = cfg_.inputs_.end();
		--it;
		inputNames_[name] = it;
	} else {
		*needle->second = NamedShaderInput(in, name, type, memberSuffix);
	}
	if (in->isStagedBuffer()) {
		auto bo = dynamic_cast<StagedBuffer*>(in.get());
		for (auto& blockUniform : bo->stagedInputs()) {
			std::string memberName = (blockUniform.name_.empty() ? blockUniform.in_->name() : blockUniform.name_);
			if (!memberSuffix.empty()) {
				memberName += memberSuffix;
			}
			define(REGEN_STRING("HAS_" << memberName), "TRUE");
		}
	}
}

void StateConfigurer::preAddState(const State *s) {
	const auto *fboState = dynamic_cast<const FBOState *>(s);
	if (fboState) {
		// set FBO flag to true, to avoid that parent FBOs are added
		hasFBO_ = true;
	}
	for (const auto & it : *s->joined().get()) {
		preAddState(it.get());
	}
}

void StateConfigurer::addState(const State *s) {
	const auto *x1 = dynamic_cast<const FeedbackSpecification *>(s);
	const auto *x2 = dynamic_cast<const TextureState *>(s);
	const auto *x3 = dynamic_cast<const StateSequence *>(s);
	const auto *fboState = dynamic_cast<const FBOState *>(s);

	if (fboState) {
		// skip this state if it is a FBO and a child node already has a FBO.
		if (hasFBO_) { return; }
	}

	{
		// remember inputs, they will be enabled automatically
		// when the shader is enabled.
		for (const auto & it : s->inputs()) {
			addInput(it.name_, it.in_, it.type_, it.memberSuffix_);

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
				if (in->numArrayElements()>1 || in->forceArray()) {
					define(REGEN_STRING("IS_ARRAY_" << inName), "TRUE");
				}
				if (in->numInstances() > 1) {
					define("HAS_INSTANCES", "TRUE");
					cfg_.numInstances_ = in->numInstances();
				}

				if (in->isStagedBuffer()) {
					auto bo = dynamic_cast<StagedBuffer*>(in);
					for (auto& blockUniform : bo->stagedInputs()) {
						if (!it.memberSuffix_.empty()) {
							if (blockUniform.name_.empty()) {
								queue.emplace(
									REGEN_STRING(blockUniform.in_->name() << it.memberSuffix_),
									blockUniform.in_.get());
							} else {
								queue.emplace(
									REGEN_STRING(blockUniform.name_ << it.memberSuffix_),
									blockUniform.in_.get());
							}
						} else {
							queue.emplace(blockUniform.name_, blockUniform.in_.get());
						}
					}
				}
			}
		}
		if (s->numInstances()>1) {
			define("HAS_INSTANCES", "TRUE");
			cfg_.numInstances_ = std::max(cfg_.numInstances_, static_cast<uint32_t>(s->numInstances()));
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
		auto needle = cfg_.textures_.find(x2->name());
		if (needle == cfg_.textures_.end()) {
			// add texture to the list
			auto texIdx = cfg_.textures_.size();
			auto it = cfg_.textures_.insert({ x2->name(), { x2->texture(), texIdx } });
			needle = it.first;
		}
		auto texIdx = needle->second.second;
		define(REGEN_STRING("TEX_ID" << texIdx), REGEN_STRING(x2->stateID()));
		define(REGEN_STRING("TEX_ID_" << x2->name()), REGEN_STRING(x2->stateID()));
		needle->second.first = x2->texture();
		addInput(x2->name(), x2->texture(), x2->samplerType());
		// note: NUM_TEXTURES is rather "num texture states", i.e. one texture can be used
		//         in multiple states with different mapping etc.
		textureStates_.insert(x2);
		define("NUM_TEXTURES", REGEN_STRING(textureStates_.size()));
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
	for (const auto & it : *s->joined().get()) {
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
