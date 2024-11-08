#ifndef REGEN_SCENE_MOTION_BLUR_H_
#define REGEN_SCENE_MOTION_BLUR_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/nodes/shader.h>
#include "regen/effects/motion-blur.h"

#define REGEN_MOTION_BLUR_STATE_CATEGORY "motion-blur"

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates animations.
		 */
		class MotionBlurProvider : public NodeProcessor {
		public:
			MotionBlurProvider()
					: NodeProcessor(REGEN_MOTION_BLUR_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) override {
				// get the camera from input
				auto camera = parser->getResources()->getCamera(parser, input.getValue("camera"));
				if (camera.get() == nullptr) {
					REGEN_WARN("No Camera found for " << input.getDescription() << ".");
					return;
				}

				auto fs = ref_ptr<MotionBlur>::alloc(camera);
				auto node = ref_ptr<StateNode>::alloc(fs);
				parent->addChild(node);

				StateConfigurer shaderConfigurer;
				shaderConfigurer.addNode(node.get());
				fs->createShader(shaderConfigurer.cfg());
			}
		};
	}
}

#endif /* REGEN_SCENE_MOTION_BLUR_H_ */
