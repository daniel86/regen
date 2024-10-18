/*
 * blend.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_BLEND_H_
#define REGEN_SCENE_BLEND_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#define REGEN_BLEND_STATE_CATEGORY "blend"

#include <regen/states/blend-state.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates BlendState's.
		 */
		class BlendStateProvider : public StateProcessor {
		public:
			BlendStateProvider()
					: StateProcessor(REGEN_BLEND_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) {
				ref_ptr<BlendState> blend = ref_ptr<BlendState>::alloc(
						input.getValue<BlendMode>("mode", BLEND_MODE_SRC));
				if (input.hasAttribute("color")) {
					blend->setBlendColor(input.getValue<Vec4f>("color", Vec4f(0.0f)));
				}
				if (input.hasAttribute("equation")) {
					blend->setBlendEquation(glenum::blendFunction(
							input.getValue<string>("equation", "ADD")));
				}
				state->joinStates(blend);
			}
		};
	}
}

#endif /* BLEND_H_ */
