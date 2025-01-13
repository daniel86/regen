#ifndef REGEN_SCENE_SHAPE_H_
#define REGEN_SCENE_SHAPE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/nodes/shader.h>

#define REGEN_SHAPE_STATE_CATEGORY "shape"

namespace regen {
	namespace scene {
		class ShapeStateProvider : public StateProcessor {
		public:
			ShapeStateProvider()
					: StateProcessor(REGEN_SHAPE_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &parent) override;
		};
	}
}

#endif /* REGEN_SCENE_SHAPE_H_ */
