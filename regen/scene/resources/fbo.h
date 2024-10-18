/*
 * fbo.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_RESOURCE_FBO_H_
#define REGEN_SCENE_RESOURCE_FBO_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/gl-types/fbo.h>

namespace regen {
	namespace scene {
		/**
		 * Provides FBO instances from SceneInputNode data.
		 */
		class FBOResource : public ResourceProvider<FBO> {
		public:
			FBOResource();

			// Override
			ref_ptr<FBO> createResource(
					SceneParser *parser, SceneInputNode &input) override;
		};
	}
}

#endif /* REGEN_SCENE_RESOURCE_FBO_H_ */
