#ifndef REGEN_SCENE_BLOOM_H_
#define REGEN_SCENE_BLOOM_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/nodes/shader.h>
#include "regen/effects/bloom-texture.h"
#include "regen/effects/bloom-pass.h"

#define REGEN_BLOOM_STATE_CATEGORY "bloom"

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates animations.
		 */
		class BloomProvider : public NodeProcessor {
		public:
			BloomProvider()
					: NodeProcessor(REGEN_BLOOM_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) override {
				if (!input.hasAttribute("texture") && !input.hasAttribute("fbo")) {
					REGEN_WARN("Ignoring " << input.getDescription() << " without input texture.");
					return;
				}
				if (!input.hasAttribute("bloom-texture")) {
					REGEN_WARN("Ignoring " << input.getDescription() << " without bloom texture.");
					return;
				}
				// get the bloom texture
				auto bloomTexture1 = parser->getResources()->getTexture(parser, input.getValue("bloom-texture"));
				if (bloomTexture1.get() == nullptr) {
					REGEN_WARN("No bloom-texture found for " << input.getDescription() << ".");
					return;
				}
				auto bloomTexture = ref_ptr<BloomTexture>::dynamicCast(bloomTexture1);
				if (bloomTexture.get() == nullptr) {
					REGEN_WARN("Bloom texture has wrong type for " << input.getDescription() << ".");
					return;
				}
				// get the input texture
				ref_ptr<Texture> inputTexture;
				if (input.hasAttribute("texture")) {
					inputTexture = parser->getResources()->getTexture(parser, input.getValue("texture"));
					if (inputTexture.get() == nullptr) {
						REGEN_WARN("Unable to find Texture for " << input.getDescription() << ".");
						return;
					}
				} else {
					ref_ptr<FBO> fbo = parser->getResources()->getFBO(parser, input.getValue("fbo"));
					if (fbo.get() == nullptr) {
						REGEN_WARN("Unable to find FBO for " << input.getDescription() << ".");
						return;
					}
					auto attachment = input.getValue<GLuint>("attachment", 0);
					std::vector<ref_ptr<Texture> > textures = fbo->colorTextures();
					if (attachment >= textures.size()) {
						REGEN_WARN("FBO " << input.getValue("fbo") <<
										  " has less then " << (attachment + 1) << " attachments.");
						return;
					}
					inputTexture = textures[attachment];
				}

				auto bloomPass = ref_ptr<BloomPass>::alloc(inputTexture, bloomTexture);
				parent->addChild(bloomPass);
				StateConfigurer shaderConfigurer;
				shaderConfigurer.addNode(bloomPass.get());
				bloomPass->createShader(shaderConfigurer.cfg());
			}
		};
	}
}

#endif /* REGEN_SCENE_BLOOM_H_ */
