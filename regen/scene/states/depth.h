/*
 * depth.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_DEPTH_H_
#define REGEN_SCENE_DEPTH_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#define REGEN_DEPTH_STATE_CATEGORY "depth"

#include <regen/states/depth-state.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates DepthState's.
		 */
		class DepthStateProvider : public StateProcessor {
		public:
			DepthStateProvider()
					: StateProcessor(REGEN_DEPTH_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) override {
				ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();

				depth->set_useDepthTest(input.getValue<bool>("test", true));
				depth->set_useDepthWrite(input.getValue<bool>("write", true));

				if (input.hasAttribute("range")) {
					auto range = input.getValue<Vec2f>("range", Vec2f(0.0f));
					depth->set_depthRange(range.x, range.y);
				}

				if (input.hasAttribute("function")) {
					depth->set_depthFunc(glenum::compareFunction(input.getValue("function")));
				}

				state->joinStates(depth);
			}
		};
	}
}

#endif /* REGEN_SCENE_DEPTH_H_ */
