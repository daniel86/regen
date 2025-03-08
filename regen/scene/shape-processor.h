#ifndef REGEN_SCENE_SHAPE_H_
#define REGEN_SCENE_SHAPE_H_

#include <regen/scene/scene-loader.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/scene-processors.h>

#define REGEN_SHAPE_STATE_CATEGORY "shape"

namespace regen::scene {
	class ShapeProcessor : public StateProcessor {
	public:
		ShapeProcessor() : StateProcessor(REGEN_SHAPE_STATE_CATEGORY) {}

		// Override
		void processInput(
				scene::SceneLoader *scene,
				SceneInputNode &input,
				const ref_ptr<StateNode> &parentNode,
				const ref_ptr<State> &parentState) override;
	};
}


#endif /* REGEN_SCENE_SHAPE_H_ */
