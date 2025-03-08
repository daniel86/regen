#ifndef REGEN_SCENE_DEBUGGER_H_
#define REGEN_SCENE_DEBUGGER_H_

#include <regen/scene/scene-loader.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/scene-processors.h>

#define REGEN_DEBUGGER_STATE_CATEGORY "debugger"

namespace regen::scene {
	/**
	 * Processes SceneInput and creates animations.
	 */
	class DebugNodeProcessor : public NodeProcessor {
	public:
		DebugNodeProcessor() : NodeProcessor(REGEN_DEBUGGER_STATE_CATEGORY) {}

		// Override
		void processInput(
				scene::SceneLoader *scene,
				SceneInputNode &input,
				const ref_ptr<StateNode> &parent) override;
	};
}


#endif /* REGEN_SCENE_DEBUGGER_H_ */
