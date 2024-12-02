/*
 * tesselation.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_TESS_H_
#define REGEN_SCENE_TESS_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#define REGEN_TESS_STATE_CATEGORY "tesselation"

#include <regen/states/tesselation-state.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates TesselationState.
		 */
		class TesselationStateProvider : public StateProcessor {
		public:
			TesselationStateProvider()
					: StateProcessor(REGEN_TESS_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) {
				ref_ptr<TesselationState> tess = ref_ptr<TesselationState>::alloc(
						input.getValue<GLuint>("num-patch-vertices", 3u));

				tess->innerLevel()->setVertex(0,
											  input.getValue<Vec4f>("inner-level", Vec4f(8.0f)));
				tess->outerLevel()->setVertex(0,
											  input.getValue<Vec4f>("outer-level", Vec4f(8.0f)));
				tess->lodFactor()->setVertex(0,
											 input.getValue<GLfloat>("lod-factor", 4.0f));
				tess->set_lodMetric(input.getValue<TesselationState::LoDMetric>(
						"lod-metric", TesselationState::CAMERA_DISTANCE));

				state->joinStates(tess);
			}
		};
	}
}

#endif /* REGEN_SCENE_TOGGLE_H_ */
