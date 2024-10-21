#ifndef REGEN_SCENE_POLYGON_H_
#define REGEN_SCENE_POLYGON_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/gl-types/gl-enum.h>

#define REGEN_POLYGON_STATE_CATEGORY "polygon"

#include <regen/states/atomic-states.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates CullState.
		 */
		class PolygonStateProvider : public StateProcessor {
		public:
			PolygonStateProvider()
					: StateProcessor(REGEN_POLYGON_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) override {
				if (input.hasAttribute("offset-fill")) {
					auto offset = input.getValue<Vec2f>("offset-fill", Vec2f(1.1, 4.0));
					state->joinStates(ref_ptr<PolygonOffsetState>::alloc(offset.x, offset.y));
				}
				if (input.hasAttribute("mode")) {
					auto mode = input.getValue("mode");
					if (mode == "fill") {
						state->joinStates(ref_ptr<FillModeState>::alloc(GL_FILL));
					} else if (mode == "line") {
						state->joinStates(ref_ptr<FillModeState>::alloc(GL_LINE));
					} else if (mode == "point") {
						state->joinStates(ref_ptr<FillModeState>::alloc(GL_POINT));
					}
				}
			}
		};
	}
}

#endif /* REGEN_SCENE_POLYGON_H_ */
