#ifndef REGEN_SCENE_PHYSICS_H_
#define REGEN_SCENE_PHYSICS_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/nodes/shader.h>

#define REGEN_PHYSICS_STATE_CATEGORY "physics"

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates animations.
		 */
		class PhysicsStateProvider : public StateProcessor {
		public:
			PhysicsStateProvider()
					: StateProcessor(REGEN_PHYSICS_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &parent) override;
		};
	}
}

#endif /* REGEN_SCENE_PHYSICS_H_ */
