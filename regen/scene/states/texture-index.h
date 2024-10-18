/*
 * texture-index.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_TEXTURE_INDEX_H_
#define REGEN_SCENE_TEXTURE_INDEX_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/states/texture.h>

#define REGEN_TEXTURE_INDEX_CATEGORY "texture-index"

#include <regen/states/fbo-state.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates states modifying texture indides.
		 */
		class TextureIndexProvider : public StateProcessor {
		public:
			TextureIndexProvider()
					: StateProcessor(REGEN_TEXTURE_INDEX_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) override {
				const string texName = input.getValue("name");

				ref_ptr<Texture> tex = TextureStateProvider::getTexture(parser, input);
				if (tex.get() == nullptr) {
					REGEN_WARN("Skipping unidentified texture node for " << input.getDescription() << ".");
					return;
				}

				if (input.hasAttribute("value")) {
					auto index = input.getValue<GLuint>("index", 0u);
					state->joinStates(ref_ptr<TextureSetIndex>::alloc(tex, index));
				} else if (input.getValue<bool>("set-next-index", true)) {
					state->joinStates(ref_ptr<TextureNextIndex>::alloc(tex));
				} else {
					REGEN_WARN("Skipping " << input.getDescription() << " because no index set.");
				}
			}
		};
	}
}

#endif /* REGEN_SCENE_TEXTURE_INDEX_H_ */
