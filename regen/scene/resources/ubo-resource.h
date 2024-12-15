#ifndef REGEN_SCENE_RESOURCE_UBO_H_
#define REGEN_SCENE_RESOURCE_UBO_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>
#include <regen/gl-types/ubo.h>

namespace regen {
	namespace scene {
		/**
		 * Provides FBO instances from SceneInputNode data.
		 */
		class UBOResource : public ResourceProvider<UBO> {
		public:
			UBOResource();

			// Override
			ref_ptr<UBO> createResource(
					SceneParser *parser, SceneInputNode &input) override;
		};
	}
}

#endif /* REGEN_SCENE_RESOURCE_UBO_H_ */
