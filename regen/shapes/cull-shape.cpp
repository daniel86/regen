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
	auto numIndices = numInstances_;
	// if it is a GPU shape, then we need double the size in the GPU buffer for
	// doing GPU-side sorting (needs a second buffer for the sorted indices).
	if (!isIndexShape()) { numIndices *= 2; }

	std::vector<uint32_t> clearData(numIndices);
	for (uint32_t i = 0; i < numIndices; ++i) { clearData[i] = i; }

	instanceIDMap_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numIndices);
	instanceIDBuffer_ = ref_ptr<SSBO>::alloc("InstanceIDs", BufferUpdateFlags::FULL_PER_FRAME);
	if (isIndexShape()) {
		// Note: do not set CPU-side data in case of GPU shape (we rather use setBufferData below).
		instanceIDMap_->setInstanceData(1, 1, (byte*)clearData.data());
	}
	instanceIDBuffer_->addBlockInput(instanceIDMap_);
	instanceIDBuffer_->update();
	if (!isIndexShape()) {
		// clear segment to [0, 1, 2, ..., numInstances_-1]
		instanceIDBuffer_->setBufferData(clearData.data());
	}

	setInput(instanceIDBuffer_);
}
