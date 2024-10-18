/*
 * direct-shading.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_DIRECT_SHADING_H_
#define REGEN_SCENE_DIRECT_SHADING_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/nodes/scene-node.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/states/texture.h>

#define REGEN_DIRECT_SHADING_NODE_CATEGORY "direct-shading"

#include <regen/states/direct-shading.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates DirectShading nodes.
		 */
		class DirectShadingNodeProvider : public NodeProcessor {
		public:
			DirectShadingNodeProvider()
					: NodeProcessor(REGEN_DIRECT_SHADING_NODE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) override {
				ref_ptr<SceneInputNode> lightNode = input.getFirstChild("direct-lights");
				ref_ptr<SceneInputNode> passNode = input.getFirstChild("direct-pass");
				if (lightNode.get() == nullptr) {
					REGEN_WARN("Missing direct-lights attribute for " << input.getDescription() << ".");
					return;
				}
				if (passNode.get() == nullptr) {
					REGEN_WARN("Missing direct-pass attribute for " << input.getDescription() << ".");
					return;
				}

				ref_ptr<DirectShading> shadingState = ref_ptr<DirectShading>::alloc();
				ref_ptr<StateNode> shadingNode = ref_ptr<StateNode>::alloc(shadingState);
				parent->addChild(shadingNode);

				if (input.hasAttribute("ambient")) {
					shadingState->ambientLight()->setVertex(0,
															input.getValue<Vec3f>("ambient", Vec3f(0.1f)));
				}

				// load lights
				const list<ref_ptr<SceneInputNode> > &childs = lightNode->getChildren();
				for (auto it = childs.begin(); it != childs.end(); ++it) {
					ref_ptr<SceneInputNode> n = *it;
					if (n->getCategory() != "light") {
						REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
						continue;
					}

					ref_ptr<Light> light = parser->getResources()->getLight(parser, n->getName());
					if (light.get() == nullptr) {
						REGEN_WARN("Unable to find Light for '" << n->getDescription() << "'.");
						continue;
					}

					auto shadowFiltering =
							n->getValue<ShadowFilterMode>("shadow-filter", SHADOW_FILTERING_NONE);
					ref_ptr<Texture> shadowMap;
					ref_ptr<Texture> shadowColorMap;
					ref_ptr<LightCamera> shadowCamera;
					if (n->hasAttribute("shadow-camera")) {
						shadowCamera = ref_ptr<LightCamera>::dynamicCast(
								parser->getResources()->getCamera(parser, n->getValue("shadow-camera")));
						if (shadowCamera.get() == nullptr) {
							REGEN_WARN("Unable to find LightCamera for '" << n->getDescription() << "'.");
						}
					}
					if (n->hasAttribute("shadow-buffer") || n->hasAttribute("shadow-texture")) {
						shadowMap = TextureStateProvider::getTexture(parser, *n.get(),
																	 "shadow-texture", "shadow-buffer",
																	 "shadow-attachment");
						if (shadowMap.get() == nullptr) {
							REGEN_WARN("Unable to find ShadowMap for '" << n->getDescription() << "'.");
						}
					}
					if (n->hasAttribute("shadow-buffer") || n->hasAttribute("shadow-color-texture")) {
						shadowColorMap = TextureStateProvider::getTexture(parser, *n.get(),
																		  "shadow-color-texture", "shadow-buffer",
																		  "shadow-color-attachment");
					}
					shadingState->addLight(light, shadowCamera, shadowMap, shadowColorMap, shadowFiltering);
				}

				// parse passNode
				SceneNodeProcessor().processInput(parser, *passNode.get(), shadingNode);
			}
		};
	}
}

#endif /* REGEN_SCENE_DIRECT_SHADING_H_ */
