/*
 * camera.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_CAMERA_H_
#define REGEN_SCENE_CAMERA_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resource-manager.h>

#define REGEN_CAMERA_STATE_CATEGORY "camera"

#include <regen/camera/camera.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates Camera's.
		 */
		class CameraStateProvider : public StateProcessor {
		public:
			CameraStateProvider()
					: StateProcessor(REGEN_CAMERA_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) override {
				ref_ptr<Camera> cam = parser->getResources()->getCamera(parser, input.getName());
				if (cam.get() == nullptr) {
					REGEN_WARN("Unable to load Camera for '" << input.getDescription() << "'.");
					return;
				}
				state->joinStates(cam);
			}
		};
	}
}

#endif /* REGEN_SCENE_CAMERA_H_ */
