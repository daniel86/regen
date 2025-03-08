#ifndef REGEN_SCENE_ANIMATIONS_H_
#define REGEN_SCENE_ANIMATIONS_H_

#include <regen/scene/scene-loader.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/scene-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>

#define REGEN_DEFORMATION_NODE_CATEGORY "animation"

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates animations.
		 */
		class AnimationProcessor : public StateProcessor {
		public:
			AnimationProcessor()
					: StateProcessor(REGEN_DEFORMATION_NODE_CATEGORY) {}

			// Override
			void processInput(
					scene::SceneLoader *scene,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parentNode,
					const ref_ptr<State> &parentState) override;

		protected:
			std::vector<ref_ptr<Animation>> createDeformation(scene::SceneLoader *scene, SceneInputNode &input);
		};
	}
}

#endif /* REGEN_SCENE_ANIMATIONS_H_ */
