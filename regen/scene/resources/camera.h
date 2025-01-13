/*
 * camera.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_RESOURCE_CAMERA_H_
#define REGEN_SCENE_RESOURCE_CAMERA_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/camera/camera.h>

namespace regen {
	namespace scene {
		/**
		 * Provides Camera instances from SceneInputNode data.
		 */
		class CameraResource : public ResourceProvider<Camera> {
		public:
			CameraResource();

			// Override
			ref_ptr<Camera> createResource(
					SceneParser *parser, SceneInputNode &input) override;

			ref_ptr<Camera> createCamera(
					SceneParser *parser, SceneInputNode &input);
		};
	}
}

#endif /* REGEN_SCENE_RESOURCE_CAMERA_H_ */
