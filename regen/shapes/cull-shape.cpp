#include <regen/utility/logging.h>

#include "cull-shape.h"

using namespace regen;

CullShape::CullShape(
		const ref_ptr<SpatialIndex> &spatialIndex,
		std::string_view shapeName)
		: State(),
		  shapeName_(shapeName),
		  spatialIndex_(spatialIndex) {
	auto indexedShape = spatialIndex_->getShape(shapeName);
	initCullShape(indexedShape, true);
}

CullShape::CullShape(
		const ref_ptr<BoundingShape> &boundingShape,
		std::string_view shapeName)
		: State(),
		  shapeName_(shapeName) {
	initCullShape(boundingShape, false);
}

void CullShape::initCullShape(
		const ref_ptr<BoundingShape> &boundingShape,
		bool isIndexShape) {
	auto mesh = boundingShape->mesh();
	if (mesh.get() != nullptr) {
		// add mesh as part if not already added
		parts_.push_back(mesh);
	}
	for (const auto &part : boundingShape->parts()) {
		if (part.get() != nullptr && part.get() != mesh.get()) {
			parts_.push_back(part);
		}
	}
	boundingShape_ = boundingShape;
	numInstances_ = boundingShape->numInstances();
}
