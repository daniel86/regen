#ifndef REGEN_SCENE_INDEX_RESOURCE_H_
#define REGEN_SCENE_INDEX_RESOURCE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/shapes/spatial-index.h>

namespace regen {
	namespace scene {
		/**
		 * Provides Font instances from SceneInputNode data.
		 */
		class IndexResource : public ResourceProvider<SpatialIndex> {
		public:
			IndexResource();

			// Override
			ref_ptr<SpatialIndex> createResource(
					SceneParser *parser, SceneInputNode &input) override;
		};
	}
}

#endif /* REGEN_SCENE_INDEX_RESOURCE_H_ */
