/*
 * shader.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_SHADER_NODE_H_
#define REGEN_SCENE_SHADER_NODE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/states/shader-state.h>
#include <regen/scene/resource-manager.h>

#define REGEN_SHADER_NODE_CATEGORY "shader"

#include <regen/states/filter.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates FilterSequence nodes.
		 */
		class ShaderNodeProvider : public NodeProcessor {
		public:
			/**
			 * @param s A State instance.
			 * @return Shader associated to State or joined States.
			 */
			static ref_ptr<Shader> findShader(State *s) {
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

			/**
			 * @param n A StateNode instance.
			 * @return Shader associated to StateNode or on of the parent StateNode's.
			 */
			static ref_ptr<Shader> findShader(StateNode *n) {
				ref_ptr<Shader> out = findShader(n->state().get());
				if (out.get() == nullptr && n->hasParent()) {
					return findShader(n->parent());
				} else {
					return out;
				}
			}

			ShaderNodeProvider()
					: NodeProcessor(REGEN_SHADER_NODE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) override {
				if (!input.hasAttribute("key") && !input.hasAttribute("code")) {
					REGEN_WARN("Ignoring " << input.getDescription() << " without shader input.");
					return;
				}
				ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
				parent->state()->joinStates(shaderState);

				const std::string shaderKey = input.hasAttribute("key") ?
										 input.getValue("key") : input.getValue("code");
				StateConfigurer stateConfigurer;
				stateConfigurer.addNode(parent.get());
				shaderState->createShader(stateConfigurer.cfg(), shaderKey);
			}
		};
	}
}

#endif /* REGEN_SCENE_SHADER_NODE_H_ */
