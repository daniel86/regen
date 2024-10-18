/*
 * define.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_DEFINE_H_
#define REGEN_SCENE_DEFINE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#define REGEN_DEFINE_STATE_CATEGORY "define"

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates defines in last State.
		 */
		class DefineStateProvider : public StateProcessor {
		public:
			DefineStateProvider()
					: StateProcessor(REGEN_DEFINE_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
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
				while (!s->joined().empty()) {
					s = *s->joined().rbegin();
				}
				s->shaderDefine(
						input.getValue("key"),
						input.getValue("value"));
			}
		};
	}
}

#endif /* REGEN_SCENE_DEFINE_H_ */
