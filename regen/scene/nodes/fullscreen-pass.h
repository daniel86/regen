/*
 * fullscreen-pass.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_FULLSCREEN_PASS_H_
#define REGEN_SCENE_FULLSCREEN_PASS_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>

#define REGEN_FULLSCREEN_PASS_NODE_CATEGORY "fullscreen-pass"

#include <regen/states/fullscreen-pass.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates FullscreenPass nodes.
		 */
		class FullscreenPassNodeProvider : public NodeProcessor {
		public:
			FullscreenPassNodeProvider()
					: NodeProcessor(REGEN_FULLSCREEN_PASS_NODE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) override {
				if (!input.hasAttribute("shader")) {
					REGEN_WARN("Missing shader attribute for " << input.getDescription() << ".");
					return;
				}
				const string shaderKey = input.getValue("shader");

				ref_ptr<FullscreenPass> fs = ref_ptr<FullscreenPass>::alloc(shaderKey);
				ref_ptr<StateNode> node = ref_ptr<StateNode>::alloc(fs);
				parent->addChild(node);

				StateConfigurer shaderConfigurer;
				shaderConfigurer.addNode(node.get());
				fs->createShader(shaderConfigurer.cfg());
			}
		};
	}
}

#endif /* REGEN_SCENE_FULLSCREEN_PASS_H_ */
