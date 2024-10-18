/*
 * light.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_RESOURCE_LIGHT_H_
#define REGEN_SCENE_RESOURCE_LIGHT_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/states/light-state.h>

namespace regen {
	namespace scene {
		/**
		 * Provides Light instances from SceneInputNode data.
		 */
		class LightResource : public ResourceProvider<Light> {
		public:
			LightResource();

			// Override
			ref_ptr<Light> createResource(
					SceneParser *parser, SceneInputNode &input) override;
		};
	}
}

#endif /* REGEN_SCENE_RESOURCE_LIGHT_H_ */
