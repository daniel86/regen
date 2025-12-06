#ifndef REGEN_STATE_DEFINE_H_
#define REGEN_STATE_DEFINE_H_

#include <regen/scene/scene-loader.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/scene-processors.h>

#define REGEN_DEFINE_STATE_CATEGORY "define"

namespace regen::scene {
	/**
	 * Processes SceneInput and creates defines in last State.
	 */
	class ShaderDefineProcessor : public StateProcessor {
	public:
		ShaderDefineProcessor() : StateProcessor(REGEN_DEFINE_STATE_CATEGORY) {}

		// Override
		void processInput(scene::SceneLoader *scene, SceneInputNode &input,
						  const ref_ptr<StateNode> &parent,
						  const ref_ptr<State> &state) override {
			if (!input.hasAttribute("key")) {
				REGEN_WARN("Ignoring " << input.getDescription() << " without key attribute.");
				return;
			}
			if (!input.hasAttribute("value")) {
				REGEN_WARN("Ignoring " << input.getDescription() << " without value attribute.");
				return;
			}
			ref_ptr<State> s = state;
			while (!s->joined()->empty()) {
				s = *s->joined()->rbegin();
			}
			s->shaderDefine(input.getValue("key"), input.getValue("value"));
		}
	};
}


#endif /* REGEN_STATE_DEFINE_H_ */
