#include <regen/utility/logging.h>

#include "cull-shape.h"

using namespace regen;

CullShape::CullShape(
		const ref_ptr<SpatialIndex> &spatialIndex,
		std::string_view shapeName,
		bool useSharedInstanceBuffer)
		: State(),
		  shapeName_(shapeName),
		  spatialIndex_(spatialIndex) {
	auto indexedShape = spatialIndex_->getShape(shapeName);
	initCullShape(indexedShape, true, useSharedInstanceBuffer);
}

CullShape::CullShape(
		const ref_ptr<BoundingShape> &boundingShape,
		std::string_view shapeName,
		bool useSharedInstanceBuffer)
		: State(),
		  shapeName_(shapeName) {
	initCullShape(boundingShape, false, useSharedInstanceBuffer);
}

void CullShape::initCullShape(
		const ref_ptr<BoundingShape> &boundingShape,
		bool isIndexShape,
		bool useSharedInstanceBuffer) {
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
	if (useSharedInstanceBuffer) {
		// create instanceIDMap_ and instanceIDBuffer_, these are used to store the instance IDs
		createBuffers();
	}
}

void CullShape::createBuffers() {
	auto numIndices = boundingShape_->numInstances();
	if (numIndices <= 1) { return; }

	std::vector<uint32_t> clearData(numInstances_);
	for (uint32_t i = 0; i < numInstances_; ++i) { clearData[i] = i; }

	// NOTE: cull shape is potentially used in multiple passes, and the InstanceIDs are usually
	//       updated for each draw call, so we use FULL_PER_DRAW to ensure the data is updated.
	//       if per-frame is desired, the ssbo could be moved into the cull state.
	instanceData_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numIndices);
	instanceBuffer_ = ref_ptr<SSBO>::alloc("InstanceIDs", BufferUpdateFlags::FULL_PER_DRAW);
	if (isIndexShape()) {
		// Note: do not set CPU-side data in case of GPU shape (we rather use setBufferData below).
		instanceData_->setInstanceData(1, 1, (byte*)clearData.data());
	}
	instanceBuffer_->addStagedInput(instanceData_);
	instanceBuffer_->update();
	if (!isIndexShape()) {
		// clear segment to [0, 1, 2, ..., numInstances_-1]
		instanceBuffer_->setBufferSubData(0, numInstances_, clearData.data());
	}

	setInput(instanceBuffer_);
}
