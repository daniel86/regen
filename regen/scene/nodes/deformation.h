#ifndef REGEN_SCENE_ANIMATIONS_H_
#define REGEN_SCENE_ANIMATIONS_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/nodes/shader.h>

#define REGEN_DEFORMATION_NODE_CATEGORY "deformation"

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates animations.
		 */
		class DeformationNodeProvider : public NodeProcessor {
		public:
			DeformationNodeProvider()
					: NodeProcessor(REGEN_DEFORMATION_NODE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) override;
		};
	}
}

#endif /* REGEN_SCENE_ANIMATIONS_H_ */
