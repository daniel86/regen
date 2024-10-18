/*
 * font.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_RESOURCE_FONT_H_
#define REGEN_SCENE_RESOURCE_FONT_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/utility/font.h>

namespace regen {
	namespace scene {
		/**
		 * Provides Font instances from SceneInputNode data.
		 */
		class FontResource : public ResourceProvider<Font> {
		public:
			FontResource();

			// Override
			ref_ptr<Font> createResource(
					SceneParser *parser, SceneInputNode &input);
		};
	}
}

#endif /* REGEN_SCENE_RESOURCE_FONT_H_ */
