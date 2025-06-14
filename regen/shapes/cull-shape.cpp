#include <regen/utility/logging.h>

#include "cull-shape.h"

using namespace regen;

CullShape::CullShape(const ref_ptr<SpatialIndex> &spatialIndex, std::string_view shapeName)
		: State(),
		  shapeName_(shapeName),
		  spatialIndex_(spatialIndex) {
	auto indexedShape = spatialIndex_->getShape(shapeName);
	initCullShape(indexedShape, true);
}

CullShape::CullShape(const ref_ptr<BoundingShape> &boundingShape, std::string_view shapeName)
		: State(),
		  shapeName_(shapeName) {
	initCullShape(boundingShape, false);
}

void CullShape::initCullShape(const ref_ptr<BoundingShape> &boundingShape, bool isIndexShape) {
	auto mesh = boundingShape->mesh();
	bool isMeshPart = false;
	for (const auto &part : boundingShape->parts()) {
		if (part.get() != nullptr) {
			parts_.push_back(part);
		}
		if (part.get() == mesh.get()) {
			isMeshPart = true;
		}
	}
	if (!isMeshPart && mesh.get() != nullptr) {
		// add mesh as part if not already added
		parts_.push_back(mesh);
	}
	tf_ = boundingShape->transform();
	numInstances_ = boundingShape->numInstances();
	// create instanceIDMap_ and instanceIDBuffer_, these are used to store the instance IDs
	createBuffers();
}

void CullShape::createBuffers() {
	numInstances_ = tf_->numInstances();

	// Create array with numInstances_ elements.
	// The instance ids will be added each frame 1. in LOD-groups and 2. in view-dependent order
	instanceIDMap_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numInstances_);
	//instanceIDMap_->set_forceArray(true);
	instanceIDMap_->setInstanceData(1, 1, nullptr);
	auto instanceData = instanceIDMap_->mapClientData<GLuint>(ShaderData::WRITE);
	for (GLuint i = 0; i < numInstances_; ++i) {
		instanceData.w[i] = i;
	}
	instanceData.unmap();
	// use SSBO for instanceIDMap_
	instanceIDBuffer_ = ref_ptr<SSBO>::alloc("InstanceIDs", BUFFER_USAGE_STREAM_DRAW);
	instanceIDBuffer_->addBlockInput(instanceIDMap_);
	instanceIDBuffer_->update();
	joinShaderInput(instanceIDBuffer_);
}
