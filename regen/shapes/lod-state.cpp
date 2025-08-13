#include <regen/states/state-node.h>
#include "lod-state.h"
#include "regen/meshes/mesh-vector.h"
#include "regen/gl-types/gl-param.h"
#include "regen/utility/conversion.h"
#include "regen/camera/light-camera.h"
#include "regen/gl-types/queries/elapsed-time.h"
#include "regen/buffer/dibo.h"

#define RADIX_BITS_PER_PASS 4u
#define RADIX_GROUP_SIZE 256
#define RADIX_OFFSET_GROUP_SIZE 512
//#define LOD_DEBUG_GROUPS
//#define LOD_DEBUG_TIME
//#define LOD_DEBUG_GPU_TIME
//#define LOD_DEBUG_SHAPE "fish-shape"
// TODO: Consider using indirect draw buffers for single-LOD shapes as well.
//      - evades synchronization issues with the staging system which uploads to main with delay.
//      - might be needed in the long run anyway, when grouping meshes with the same shader
//      - trivial to implement.
//#define LOD_USE_DIBO_FOR_SINGLE_LOD

using namespace regen;

static inline void reverse_copy_u32(uint32_t *__restrict dst, const uint32_t *__restrict src, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		dst[i] = src[count - 1 - i];
	}
}

namespace regen {
	class InstanceUpdater : public Animation {
	public:
		explicit InstanceUpdater(LODState *lodState)
				: Animation(false, true),
				  lodState_(lodState) {}

		void animate(double dt) override {
			lodState_->resetVisibility();
			lodState_->traverseCPU();
		}

	protected:
		LODState *lodState_;
	};
}

LODState::LODState(
		const ref_ptr<Camera> &camera,
		const ref_ptr<CullShape> &cullShape)
		: State(),
		  camera_(camera),
		  cullShape_(cullShape) {
	hasShadowTarget_ = dynamic_cast<LightCamera *>(camera_.get()) != nullptr;
	if (!cullShape_->parts().empty()) {
		mesh_ = cullShape_->parts().front();
	} else {
		REGEN_WARN("No mesh set for shape '" << cullShape_->shapeName() << "'.");
	}
	if (!cullShape_->boundingShape()->transform().get()) {
		REGEN_WARN("No TF for cull shape '"
			<< cullShape_->shapeName()
			<< "'. LOD and culling will not work properly!");
	}
	if (camera_->hasFixedLOD()) {
		if (camera_->fixedLODQuality() == LODQuality::HIGH) {
			fixedLOD_ = 0u; // always use highest quality LOD
		} else if (camera_->fixedLODQuality() == LODQuality::LOW) {
			fixedLOD_ = mesh_->numLODs() - 1u; // always use lowest quality LOD
		} else {
			fixedLOD_ = 0u;
			if (mesh_->numLODs() == 2) {
				fixedLOD_ = 1u;
			} else if (mesh_->numLODs() == 3) {
				fixedLOD_ = 1u;
			} else if (mesh_->numLODs() > 3) {
				fixedLOD_ = 2u;
			}
		}
	}
	initLODState();
}

void LODState::initLODState() {
	lodNumInstances_.resize(4);
	lodBoundaries_.resize(5);
	lodBoundaries_[0] = 0;
	// initially all instances are added to first LOD group
	for (uint32_t i = 0u; i < 4; ++i) {
		lodNumInstances_[i] = 0;
	}
	if (camera_->hasFixedLOD()) {
		lodNumInstances_[fixedLOD_] = cullShape_->numInstances();
	} else {
		lodNumInstances_[0] = cullShape_->numInstances();
	}

	if (cullShape_->hasInstanceBuffer()) {
		// the cull shape may provide a shared instance buffer which is used for all
		// LOD states that use this cull shape, e.g. for shadow mapping, reflection passes, etc.
		// In this case, we cannot update the instance IDs per frame, but only per draw call
		// as the content might change between different passes.
		// NOTE: If memory allows, it is better to use a per-frame instance buffer to avoid stalling.
		instanceData_ = cullShape_->instanceData();
		instanceBuffer_ = cullShape_->instanceBuffer();
	} else if (cullShape_->numInstances() > 1) {
		// create instance buffer for per-frame updates.
		createInstanceBuffer();
		// Create indirect draw buffers for each mesh part and LOD.
		createIndirectDrawBuffers();
	} else if (camera_->numLayer()>1) {
		// we also need indirect draw buffers for multi-layer rendering
		REGEN_INFO("Switching to indirect draw for shape '" << cullShape_->shapeName()
					   << "' with " << camera_->numLayer() << " layers.");
		createIndirectDrawBuffers();
	}

	if (cullShape_->isIndexShape()) {
		auto index = cullShape_->spatialIndex();
		shapeIndex_ = index->getIndexedShape(camera_, cullShape_->shapeName());
		if (!shapeIndex_.get()) {
			REGEN_WARN("No indexed shape found for cull shape '" << cullShape_->shapeName() << "'.");
		} else {
			// FIXME: Below we assign the IBO and DIBO to the shape index, which will only be useful
			//        for the CPU path! the GPU path currently won't work with multiple parts entirely
			//        due to this. this is required in some cases in the scene loading.
			//        We would need to attach these buffers to some other common state which is accessible
			//        from both paths, and where we have individual buffers for each part+camera combination.
			if (!cullShape_->hasInstanceBuffer() && instanceBuffer_.get()) {
				// Make sure that all meshes that share the index shape also have access to the instance buffer.
				shapeIndex_->setInstanceBuffer(instanceBuffer_);
				shapeIndex_->setSortMode(instanceSortMode_);
				// Note: we do not update in state enable, but rather use a separate animation for this.
				//   This is done as the animations are dedicated place to write to client buffers,
				//   and we avoid computations in between draw calls, however would still be ok
				//   to update in state traverse.
				lodAnim_ = ref_ptr<InstanceUpdater>::alloc(this);
				lodAnim_->startAnimation();
			}
			if (!indirectDrawBuffers_.empty()) {
				shapeIndex_->setIndirectDrawBuffers(indirectDrawBuffers_);
			}
		}
	} else {
		createComputeShader();
	}
	REGEN_INFO("Created LOD state for cull shape '"
					   << cullShape_->shapeName()
					   << "' with " << cullShape_->numInstances() << " instances, "
					   << mesh_->numLODs() << " LODs, "
					   << (cullShape_->hasInstanceBuffer() ? "per-draw" : "per-frame") << " "
					   << (cullShape_->isIndexShape() ? "CPU" : "GPU") << " mode.");
}

void LODState::createInstanceBuffer() {
	int32_t numIndices = cullShape_->numInstances();
	std::vector<uint32_t> clearData(numIndices);
	for (int32_t i = 0; i < numIndices; ++i) { clearData[i] = i; }

	instanceData_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numIndices);
	instanceBuffer_ = ref_ptr<SSBO>::alloc("InstanceIDs", BufferUpdateFlags::FULL_PER_FRAME);
	if (cullShape_->isIndexShape()) {
		instanceData_->setInstanceData(1, 1, (byte*)clearData.data());
	}
	instanceBuffer_->addStagedInput(instanceData_);
	instanceBuffer_->update();
	if (!cullShape_->isIndexShape()) {
		// clear segment to [0, 1, 2, ..., numInstances_-1]
		instanceBuffer_->setBufferSubData(0, numIndices, clearData.data());
	}
	// Set the instance buffer as input of the LOD state.
	// This should make it available for the mesh state.
	setInput(instanceBuffer_);
}

ref_ptr<SSBO> LODState::createIndirectDrawBuffer(uint32_t partIdx) {
	const uint32_t numLayers = camera_->numLayer();
	auto buffer = ref_ptr<DrawIndirectBuffer>::alloc(
			"IndirectDrawBuffer",
			BufferUpdateFlags::FULL_PER_FRAME);
	auto input = ref_ptr<ShaderInputStruct<DrawCommand>>::alloc(
			"DrawCommand", "drawParams", 4 * numLayers);
	if (cullShape_->isIndexShape()) {
		input->setInstanceData(1, 1,
			(byte*)indirectDrawData_[partIdx].current.data());
	}
	buffer->addStagedInput(input);
	buffer->update();
	if (!cullShape_->isIndexShape()) {
		buffer->setBufferData((byte*)indirectDrawData_[partIdx].current.data());
	}
	return buffer;
}

void LODState::createIndirectDrawBuffers() {
	const uint32_t numParts = cullShape_->parts().size();
	const uint32_t numLayers = camera_->numLayer();

	indirectDrawBuffers_.resize(numParts);
	indirectDrawData_.resize(numParts);

	for (uint32_t partIdx=0; partIdx < numParts; ++partIdx) {
		auto &part = cullShape_->parts()[partIdx];
		auto &partLODs = part->meshLODs();
		auto &drawData = indirectDrawData_[partIdx];

		drawData.current.resize(4 * numLayers);
		drawData.clear.resize(4 * numLayers);

		// Create the indirect draw data for this part and the first layer.
		// DrawID order: LOD0_layer0, LOD0_layer1, LOD0_layer2
		// 				 LOD1_layer0, LOD1_layer1, LOD1_layer2, ...
		for (uint32_t lodIdx = 0; lodIdx < 4; ++lodIdx) {
			const uint32_t lodStartIdx = lodIdx * numLayers;
			DrawCommand &drawParams = drawData.current[lodStartIdx];
			if (lodIdx < part->numLODs()) {
				auto &lodData = partLODs[lodIdx];
				Mesh *m = lodData.impostorMesh.get() ? lodData.impostorMesh.get() : part.get();
				if (m->indices().get()) {
					drawParams.mode = 1u; // 1=elements, 2=arrays
					drawParams.setCount(lodData.d->numIndices);
					drawParams.setFirstElement(lodData.d->indexOffset / sizeof(uint32_t));
					drawParams.data[3] = 0; // base vertex
				} else {
					drawParams.mode = 2u; // 1=elements, 2=arrays
					drawParams.setCount(lodData.d->numVertices);
					drawParams.setFirstElement(lodData.d->vertexOffset);
				}
			} else {
				// no LOD data available, use the base mesh
				drawParams.mode = part->indices().get() ? 1u : 2u; // 1=elements, 2=arrays
				drawParams.setCount(0);
				drawParams.setFirstElement(0);
			}
			drawParams.setInstanceCount(lodIdx==0 ? part->numInstances() : 0);
			drawParams.setBaseInstance(0);

			// Set the clear draw command to zero instance count.
			std::memcpy(
				&drawData.clear[lodIdx * numLayers],
				&drawParams,
				sizeof(DrawCommand));
			drawData.clear[lodIdx].setInstanceCount(0);

			// Copy over the data for the remaining layers.
			for (uint32_t layerIdx = 1; layerIdx < numLayers; ++layerIdx) {
				const uint32_t lodLayerIdx = lodStartIdx + layerIdx;
				std::memcpy(
					&drawData.current[lodLayerIdx],
					&drawData.current[lodStartIdx],
					sizeof(DrawCommand));
				std::memcpy(
					&drawData.clear[lodLayerIdx],
					&drawData.clear[lodStartIdx],
					sizeof(DrawCommand));
			}
		}

		// finally create the indirect draw buffer for this part
		indirectDrawBuffers_[partIdx] = createIndirectDrawBuffer(partIdx);
	}
}

static inline uint32_t getPartLOD(uint32_t lodLevel, uint32_t numPartLevels) {
	static const uint32_t lodMappings[5][4] = {
		{0, 0, 0, 0}, // 0 LOD
		{0, 0, 0, 0}, // 1 LODs
		{0, 0, 1, 1}, // 2 LODs
		{0, 1, 2, 2}, // 3 LODs
		{0, 1, 2, 3}  // 4 LODs
	};
	return lodMappings[numPartLevels][lodLevel];
}

void LODState::updateVisibility(uint32_t lodLevel, uint32_t numInstances, uint32_t instanceOffset) {
	// increase LOD level by one if we have a shadow target
	if (!camera_->hasFixedLOD()) {
		if (hasShadowTarget_ && lodLevel < mesh_->numLODs() - 1) {
			lodLevel++;
		}
	}
	// set the LOD level
	for (uint32_t partIdx = 0; partIdx < cullShape_->parts().size(); ++partIdx) {
		auto &part = cullShape_->parts()[partIdx];
		// could be part has different number of LODs, need to compute an adjusted
		// LOD level for each part
		auto partLODLevel = getPartLOD(lodLevel, part->numLODs());
		auto &partLOD = part->meshLODs()[partLODLevel];
		const uint32_t numVisibleInstances = partLOD.d->numVisibleInstances + numInstances;
		const uint32_t baseInstance = (partLOD.d->numVisibleInstances > 0u ? partLOD.d->instanceOffset : instanceOffset);
		part->updateVisibility(partLODLevel, numVisibleInstances, baseInstance);

		if (!indirectDrawBuffers_.empty()) {
			// write into local storage buffer
			const uint32_t lodStartIdx = partLODLevel * camera_->numLayer();
			auto &current = indirectDrawData_[partIdx].current;
			auto &drawParams = current[lodStartIdx];
			drawParams.setInstanceCount(numVisibleInstances);
			drawParams.setBaseInstance(baseInstance);
			// Update draw commands for the other layers too.
			for (uint32_t layerIdx = 1; layerIdx < camera_->numLayer(); ++layerIdx) {
				auto &drawParamsLayer = current[lodStartIdx + layerIdx];
				drawParamsLayer.setInstanceCount(numVisibleInstances);
				drawParamsLayer.setBaseInstance(baseInstance);
			}
		}
	}
}

void LODState::resetVisibility() {
	auto &parts = cullShape_->parts();

	for (uint32_t partIdx = 0; partIdx < parts.size(); ++partIdx) {
		auto &part = parts[partIdx];

		for (uint32_t lodLevel = 0; lodLevel < part->numLODs(); ++lodLevel) {
			part->updateVisibility(lodLevel, 0, 0);
		}
		if (!indirectDrawBuffers_.empty()) {
			// reset the indirect draw buffer for this part
			std::memcpy(
				indirectDrawData_[partIdx].current.data(),
				indirectDrawData_[partIdx].clear.data(),
				indirectDrawBuffers_[partIdx]->inputSize());
		}
	}
}

void LODState::enable(RenderState *rs) {
	State::enable(rs);

	if (!cullShape_->isIndexShape()) {
#ifdef LOD_DEBUG_TIME
		static ElapsedTimeDebugger elapsedTime("GPU LOD", 300);
		elapsedTime.beginFrame();
		elapsedTime.push("frame begin");
#endif
		resetVisibility();
#ifdef LOD_DEBUG_TIME
		elapsedTime.push("visibility reset");
#endif
		traverseGPU(rs);
#ifdef LOD_DEBUG_TIME
		elapsedTime.push("GPU traverse");
		elapsedTime.endFrame();
#endif
	}
	else if(cullShape_->hasInstanceBuffer()) {
#ifdef LOD_DEBUG_TIME
		static ElapsedTimeDebugger elapsedTime("CPU LOD", 300);
		elapsedTime.beginFrame();
#endif
		resetVisibility();
#ifdef LOD_DEBUG_TIME
		elapsedTime.push("visibility reset");
#endif
		traverseCPU();
#ifdef LOD_DEBUG_TIME
		elapsedTime.push("CPU traverse");
		elapsedTime.endFrame();
#endif
	}
	else if (indirectDrawBuffers_.empty()) {
		// Set the mesh state for the net draw call.
		// Note: we compute the LOD groups only once per frame in an animation
		//     loop, but we cannot set the mesh state there, because the mesh may be used
		//     in multiple passes, e.g. shadow mapping, reflection, etc.
		// Note: in case of indirect draw buffers, we skip this step as at the moment the
		//     LOD state is only used in case of direct draw calls.
		for (auto &part : cullShape_->parts()) {
			// reset the visibility for each part
			for (uint32_t lodIdx = 0; lodIdx < part->numLODs(); ++lodIdx) {
				part->updateVisibility(lodIdx, 0, 0);
			}
			// update the visibility for each part
			uint32_t instanceOffset = 0;
			for (uint32_t lodIdx = 0; lodIdx < 4; ++lodIdx) {
				auto partLODLevel = getPartLOD(lodIdx, part->numLODs());
				auto &partLOD = part->meshLODs()[partLODLevel];
				const uint32_t numVisibleInstances = partLOD.d->numVisibleInstances + lodNumInstances_[lodIdx];
				const uint32_t baseInstance = (partLOD.d->numVisibleInstances > 0u ? partLOD.d->instanceOffset : instanceOffset);
				part->updateVisibility(partLODLevel, numVisibleInstances, baseInstance);
				instanceOffset += lodNumInstances_[lodIdx];
			}
		}
	}
#ifdef LOD_DEBUG_GROUPS
	REGEN_INFO("LOD for shape '" << cullShape_->shapeName() << "'"
		<< " with " << cullShape_->numInstances() << " instances, "
		<< (cullShape_->hasInstanceBuffer() ? "per-draw" : "per-frame") << " "
		<< (cullShape_->isIndexShape() ? "CPU" : "GPU") << " mode "
		<< cullShape_->parts().size() << " parts, "
		<< " and shadow target: " << (hasShadowTarget_ ? "1" : "0")
		);
	if (cullShape_->isIndexShape()) {
		REGEN_INFO("  - CPU ("
			<< std::setw(4) << std::setfill(' ') << lodNumInstances_[0] << " "
			<< std::setw(4) << std::setfill(' ') << lodNumInstances_[1] << " "
			<< std::setw(4) << std::setfill(' ') << lodNumInstances_[2] << " "
			<< std::setw(4) << std::setfill(' ') << lodNumInstances_[3] << ")");
	}
	if (!indirectDrawBuffers_.empty()) {
		// map indirect buffer and print the number of instances per LOD
		static std::vector<DrawCommand> readVec(4);
		for (uint32_t partIdx= 0; partIdx < indirectDrawBuffers_.size(); ++partIdx) {
			auto indirectBuffer = indirectDrawBuffers_[partIdx];
			auto &part = cullShape_->parts()[partIdx];
			indirectBuffer->readBufferSubData(
					0, 4 * sizeof(DrawCommand), (byte *)readVec.data());
			// print the number of instances per LOD
			REGEN_INFO("  - GPU ("
							   << std::setw(4) << std::setfill(' ') << readVec[0].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << readVec[1].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << readVec[2].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << readVec[3].instanceCount() << ")"
							   << " numLODs: " <<
							   std::setw(2) << std::setfill(' ') << part->numLODs()
							   << " part: " << partIdx);
			for (uint32_t i = 0; i < part->numLODs(); ++i) {
				REGEN_INFO("   - DIBO " << i << " -- "
								<< "mode: " << readVec[i].mode << "; data: ["
								   << std::setw(8) << readVec[i].data[0] << ", "
								   << std::setw(8) << readVec[i].data[1] << ", "
								   << std::setw(8) << readVec[i].data[2] << ", "
								   << std::setw(8) << readVec[i].data[3] << ", "
								   << std::setw(4) << readVec[i].data[4] << ", "
								   << readVec[i]._pad[0] << ", "
								   << readVec[i]._pad[1] << "]");
			}
		}
	} else if (!cullShape_->parts().empty()) {
		for (auto &part : cullShape_->parts()) {
			if (mesh_->numLODs() <= 1) continue;
			auto &meshLODs = part->meshLODs();
			uint32_t lod1_count = meshLODs[0].numVisibleInstances();
			uint32_t lod2_count = meshLODs.size() > 1 ? meshLODs[1].numVisibleInstances() : 0;
			uint32_t lod3_count = meshLODs.size() > 2 ? meshLODs[2].numVisibleInstances() : 0;
			uint32_t lod4_count = meshLODs.size() > 3 ? meshLODs[3].numVisibleInstances() : 0;
			REGEN_INFO("    - PART ("
				<< std::setw(4) << std::setfill(' ') << lod1_count << " "
				<< std::setw(4) << std::setfill(' ') << lod2_count << " "
				<< std::setw(4) << std::setfill(' ') << lod3_count << " "
				<< std::setw(4) << std::setfill(' ') << lod4_count << ")");
		}
	}
#endif
}

///////////////////////
//////////// CPU-based LOD update
///////////////////////

void LODState::traverseCPU() {
	if (!shapeIndex_->isVisible()) {
		// No instance is visible, early exit.
		// Note: the LOD state has been reset before, so nothing to do here.
		return;
	}
	if (cullShape_->numInstances() == 1) {
		if (shapeIndex_->isVisible() && mesh_.get()) {
			// set LOD level based on distance
			const Vec3f &camPos = camera_->position(0);
			const float distanceSquared = (shapeIndex_->shape()->getShapeOrigin() - camPos).lengthSquared();
			const uint32_t activeLOD = mesh_->getLODLevel(distanceSquared);
			updateVisibility(activeLOD, 1, 0);
		}
	} else if (camera_->hasFixedLOD()) {
		updateVisibility(fixedLOD_, shapeIndex_->numVisibleInstances(), 0);
	} else {
		if (tfStamp_ != cullShape_->boundingShape()->transformStamp() || cameraStamp_ != camera_->stamp()) {
			// recompute LOD groups if the transform or camera has changed
			computeLODGroups();
			tfStamp_ = cullShape_->boundingShape()->transformStamp();
			cameraStamp_ = camera_->stamp();
		}
		//computeLODGroups();
		if (mesh_->numLODs() <= 1) {
			updateVisibility(0, shapeIndex_->numVisibleInstances(), 0);
		} else {
			int32_t instanceIDOffset = 0;
			for (uint32_t lodLevel = 0; lodLevel < lodNumInstances_.size(); ++lodLevel) {
				auto lodGroupSize = lodNumInstances_[lodLevel];
				if (lodGroupSize > 0) {
					updateVisibility(lodLevel, lodGroupSize, instanceIDOffset);
					instanceIDOffset += static_cast<int32_t>(lodGroupSize);
				}
			}
		}
	}

	if (!indirectDrawBuffers_.empty()) {
		for (uint32_t partIdx = 0; partIdx < cullShape_->parts().size(); ++partIdx) {
			auto &indirectBuffer = indirectDrawBuffers_[partIdx];
			auto &indirectData = indirectDrawData_[partIdx];

			if (indirectBuffer->hasClientData()) {
				// also reset the client data buffer.
				// This is done in case not all draw buffers are updated this frame using updateVisibility.
				auto mapped = indirectBuffer->mapClientData<DrawCommand>(
						BUFFER_GPU_WRITE, 0, indirectBuffer->inputSize());
				std::memcpy(
					mapped.w.data(),
					indirectData.current.data(),
					indirectBuffer->inputSize());
			}
		}
	}
}

struct LODSelector_Full {
	const Mat4f *tfData;
	const Vec4f *modelOffsetData;
	const uint32_t *mappedData;
	const Mesh *mesh;
	const uint32_t tfIdxMultiplier;
	const uint32_t offsetIdxMultiplier;

	inline uint32_t operator()(uint32_t i, const Vec3f &camPos) const {
		auto idx = mappedData[i];
		return mesh->getLODLevel((
										 tfData[tfIdxMultiplier * idx].position() +
										 modelOffsetData[offsetIdxMultiplier * idx].xyz_() - camPos).lengthSquared());
	}
};

struct LODSelector_ModelOffset {
	const Vec4f *modelOffsetData;
	const uint32_t *mappedData;
	const Mesh *mesh;

	inline uint32_t operator()(uint32_t i, const Vec3f &camPos) const {
		return mesh->getLODLevel((
										 modelOffsetData[mappedData[i]].xyz_() - camPos).lengthSquared());
	}
};

struct LODSelector_Transform {
	const Mat4f *tfData;
	const uint32_t *mappedData;
	const Mesh *mesh;

	inline uint32_t operator()(uint32_t i, const Vec3f &camPos) const {
		return mesh->getLODLevel((
										 tfData[mappedData[i]].position() - camPos).lengthSquared());
	}
};

template<typename Selector>
static inline void countGroupSize_CPU(
		uint32_t numInstances,
		std::vector<uint32_t> &lodNumInstances,
		std::vector<uint32_t> &lodBoundaries,
		const Vec3f &camPos,
		const Selector &getLODLevel) {
	const auto numLODs = static_cast<uint32_t>(lodNumInstances.size());
	// Adjust boundaries incrementally
	for (uint32_t i = 1u; i < numLODs; ++i) {
		auto &boundary = lodBoundaries[i];
		boundary = lodBoundaries[i - 1] + lodNumInstances[i - 1];
		// Shrink boundary to num instances
		if (boundary >= numInstances) {
			boundary = numInstances;
		}
		// Move boundary backward if previous level leaks into current
		while (boundary > 0) {
			if (getLODLevel(boundary - 1, camPos) < i) break;
			--boundary;
		}
		// Move boundary forward if current level leaks into next
		while (boundary < numInstances) {
			if (getLODLevel(boundary, camPos) >= i) break;
			++boundary;
		}
		lodNumInstances[i - 1] = boundary - lodBoundaries[i - 1];

		// Fill in remaining LODs with zero instances
		if (boundary == numInstances) {
			for (uint32_t j = i; j < numLODs - 1; ++j) {
				lodNumInstances[j] = 0;
				lodBoundaries[j + 1] = numInstances;
			}
			lodNumInstances[numLODs - 1] = 0;
			return;
		}
	}
	lodBoundaries[numLODs] = numInstances;
	lodNumInstances[numLODs - 1] = numInstances - lodBoundaries[numLODs - 1];
}

void LODState::computeLODGroups() {
	auto visible_ids = shapeIndex_->mapInstanceIDs(BUFFER_GPU_READ);
	const uint32_t numVisible = visible_ids.r[0];
	if (numVisible == 0) { return; }

	const uint32_t *mappedData = visible_ids.r.data() + 1;
	auto &tf = cullShape_->boundingShape()->transform();
	const Vec3f &camPos = camera_->position(0);

	if (tf.get() && (tf->hasModelOffset() || tf->hasModelMat())) {
		auto &modelOffset = tf->modelOffset();
		auto &modelMat = tf->modelMat();
		if (tf->hasModelOffset() && tf->hasModelMat()) {
			auto modelOffsetData = modelOffset->mapClientData<Vec4f>(BUFFER_GPU_READ);
			auto tfData = modelMat->mapClientData<Mat4f>(BUFFER_GPU_READ);
			LODSelector_Full selector{
					.tfData = tfData.r.data(),
					.modelOffsetData = modelOffsetData.r.data(),
					.mappedData = mappedData,
					.mesh = mesh_.get(),
					.tfIdxMultiplier = (modelMat->numInstances() > 1u ? 1u : 0u),
					.offsetIdxMultiplier = (modelOffset->numInstances() > 1u ? 1u : 0u)
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos,
							   selector);
		} else if (tf->hasModelOffset()) {
			auto modelOffsetData = modelOffset->mapClientData<Vec4f>(BUFFER_GPU_READ);
			LODSelector_ModelOffset selector{
					.modelOffsetData = modelOffsetData.r.data(),
					.mappedData = mappedData,
					.mesh = mesh_.get()
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos,
							   selector);
		} else {
			auto tfData = modelMat->mapClientData<Mat4f>(BUFFER_GPU_READ);
			LODSelector_Transform selector{
					.tfData = tfData.r.data(),
					.mappedData = mappedData,
					.mesh = mesh_.get()
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos,
							   selector);
		}
	} else {
		for (size_t i = 1; i < lodNumInstances_.size(); ++i) {
			lodNumInstances_[i] = 0;
		}
		lodNumInstances_[0] = numVisible;
	}

	// write lodGroups_ data into instanceData_
	auto mappedClientData = instanceData_->mapClientData<uint32_t>(
			BUFFER_GPU_WRITE, 0, numVisible * sizeof(uint32_t));
	if (instanceSortMode_ == SortMode::BACK_TO_FRONT) {
		reverse_copy_u32(mappedClientData.w.data(), mappedData, numVisible);
	} else {
		std::memcpy(mappedClientData.w.data(), mappedData, numVisible * sizeof(uint32_t));
	}
}

///////////////////////
//////////// GPU-based LOD update
///////////////////////

void LODState::createComputeShader() {
	{	// Create a static indirect draw buffer, which is used for clearing the
		// first indirect draw buffer each frame.
		auto clearData = ref_ptr<ShaderInputStruct<DrawCommand>>::alloc(
					"DrawCommand", "drawParams", 4);
		clearData->setUniformUntyped((byte*)indirectDrawData_[0].clear.data()); // FIXME: data idx
		clearIndirectBuffer_ = ref_ptr<SSBO>::alloc(
			"Clear_IndirectDrawBuffer",
			BufferUpdateFlags::NEVER,
			SSBO::RESTRICT);
		clearIndirectBuffer_->addStagedInput(clearData);
		clearIndirectBuffer_->update();
	}

	{ // radix sort
		radixSort_ = ref_ptr<RadixSort>::alloc(cullShape_->numInstances());
		radixSort_->setOutputBuffer(instanceBuffer_, false);
		radixSort_->setRadixBits(RADIX_BITS_PER_PASS);
		radixSort_->setSortGroupSize(RADIX_GROUP_SIZE);
		radixSort_->setScanGroupSize(RADIX_OFFSET_GROUP_SIZE);
		radixSort_->createResources();
	}

	{ // cull
		setInput(camera_->getFrustumBuffer());
		StateConfigurer shaderCfg;
		if (instanceSortMode_ == SortMode::BACK_TO_FRONT) {
			shaderCfg.define("USE_REVERSE_SORT", "TRUE");
		}
		cullPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.cull");
		cullPass_->computeState()->shaderDefine("LOD_NUM_INSTANCES", REGEN_STRING(cullShape_->numInstances()));
		cullPass_->computeState()->shaderDefine("NUM_CAMERA_LAYERS", REGEN_STRING(camera_->frustum().size()));
		cullPass_->computeState()->setNumWorkUnits(static_cast<int>(cullShape_->numInstances()), 1, 1);
		cullPass_->computeState()->setGroupSize(RADIX_GROUP_SIZE, 1, 1);
		cullPass_->setInput(mesh_->lodThresholds());
		cullPass_->setInput(camera_->getFrustumBuffer());
		// Note: LOD pass only writes into first buffer, we need to copy into the other buffers
		//       in a separate pass.
		cullPass_->setInput(indirectDrawBuffers_[0]);
		cullPass_->setInput(radixSort_->keyBuffer());
		cullPass_->setInput(instanceBuffer_);
		auto boundingShape = mesh_->boundingShape();
		if (boundingShape->shapeType() == BoundingShapeType::SPHERE) {
			auto *sphere = dynamic_cast<BoundingSphere*>(boundingShape.get());
			cullPass_->setInput(createUniform<ShaderInput1f, float>(
					"shapeRadius", sphere->radius()));
			shaderCfg.define("SHAPE_TYPE", "SPHERE");
		}
		else if (boundingShape->shapeType() == BoundingShapeType::BOX) {
			auto *box = dynamic_cast<BoundingBox*>(boundingShape.get());
			cullPass_->setInput(createUniform<ShaderInput4f, Vec4f>(
					"shapeAABBMin", Vec4f(box->bounds().min,0.0f)));
			cullPass_->setInput(createUniform<ShaderInput4f, Vec4f>(
					"shapeAABBMax", Vec4f(box->bounds().max,0.0f)));
			if (box->isAABB()) {
				shaderCfg.define("SHAPE_TYPE", "AABB");
			} else {
				shaderCfg.define("SHAPE_TYPE", "OBB");
			}
		}
		auto &tf = cullShape_->boundingShape()->transform();
		if (tf.get()) {
			cullPass_->joinStates(tf);
		}
		cullPass_->joinStates(camera_);
		shaderCfg.define("USE_CULLING", "TRUE");
		shaderCfg.addState(cullPass_.get());
		cullPass_->createShader(shaderCfg.cfg());
	}

	if (cullShape_->parts().size()>1) {
		// copy indirect draw buffers
		copyIndirect_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.copy-indirect");
		copyIndirect_->computeState()->setNumWorkUnits(1, 1, 1);
		copyIndirect_->computeState()->setGroupSize(1, 1, 1);
		for (uint32_t indirectIdx = 0; indirectIdx < indirectDrawBuffers_.size(); ++indirectIdx) {
			copyIndirect_->setInput(indirectDrawBuffers_[indirectIdx],
					REGEN_STRING("IndirectDrawBuffer" << indirectIdx),
					REGEN_STRING(indirectIdx));
		}
		StateConfigurer shaderCfg;
		shaderCfg.addState(copyIndirect_.get());
		shaderCfg.define("NUM_ATTACHED_PARTS", REGEN_STRING(cullShape_->parts().size() - 1));
		shaderCfg.define("NUM_BASE_LOD", REGEN_STRING(mesh_->numLODs()));
		for (uint32_t i = 0; i < cullShape_->parts().size() - 1; ++i) {
			shaderCfg.define(REGEN_STRING("NUM_PART_LOD_" << i),
							 REGEN_STRING(cullShape_->parts()[i + 1]->numLODs()));
		}
		copyIndirect_->createShader(shaderCfg.cfg());
	}
}

void LODState::traverseGPU(RenderState *rs) {
#ifdef LOD_DEBUG_GPU_TIME
	static ElapsedTimeDebugger elapsedTime("LOD GPU Travers", 300);
	#ifdef LOD_DEBUG_SHAPE
	if (cullShape_->shapeName() == LOD_DEBUG_SHAPE) {
		elapsedTime.beginFrame();
	}
	#else
	elapsedTime.beginFrame();
	#endif
#endif
	// copy the clear buffer to the indirect draw buffer
	indirectDrawBuffers_[0]->setBufferData(*clearIndirectBuffer_.get());
#ifdef LOD_DEBUG_GPU_TIME
	elapsedTime.push("clear indirect draw buffer");
#endif

	// compute lod, write keys, and initialize values_[0] (instanceData_)
	cullPass_->enable(rs);
	cullPass_->disable(rs);
#ifdef LOD_DEBUG_GPU_TIME
	elapsedTime.push("cull pass");
#endif

	radixSort_->enable(rs);
	radixSort_->disable(rs);
#ifdef LOD_DEBUG_GPU_TIME
	elapsedTime.push("radix sort");
#endif

	if (copyIndirect_.get()) {
		// update the indirect draw buffers for the other parts
		copyIndirect_->enable(rs);
		copyIndirect_->disable(rs);
	}
#ifdef LOD_DEBUG_GPU_TIME
	elapsedTime.push("copy indirect draw buffers");
	elapsedTime.endFrame();
#endif
}
