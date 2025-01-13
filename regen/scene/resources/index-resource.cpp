#include "index-resource.h"
#include "regen/shapes/quad-tree.h"
#include "regen/scene/resource-manager.h"

using namespace regen::scene;
using namespace regen;
using namespace std;

#define REGEN_INDEX_CATEGORY "index"

IndexResource::IndexResource()
		: ResourceProvider(REGEN_INDEX_CATEGORY) {}

ref_ptr<SpatialIndex> IndexResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	auto indexType = input.getValue<std::string>("type", "quadtree");
	ref_ptr<SpatialIndex> index;

	if (indexType == "quadtree") {
		auto quadTree = ref_ptr<QuadTree>::alloc();
		//quadTree->setMaxObjectsPerNode(input.getValue<GLuint>("max-objects-per-node", 4u));
		index = quadTree;
	}

	if (index.get()) {
		parser->getResources()->putIndex(input.getName(), index);
		return index;
	} else {
		REGEN_WARN("Ignoring Index '" << input.getDescription() << "' without type.");
		return {};
	}
}


