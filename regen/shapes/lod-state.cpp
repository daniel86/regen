#include <regen/states/state-node.h>
#include "lod-state.h"

#include "aabb.h"
#include "obb.h"
#include "regen/objects/composite-mesh.h"
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
//#define LOD_DEBUG_SHAPE "silhouette-shape"
// TODO: Consider using indirect draw buffers for single-LOD shapes as well.
//      - evades synchronization issues with the staging system which uploads to main with delay.
//      - might be needed in the long run anyway, when grouping meshes with the same shader
//      - trivial to implement.
//#define LOD_USE_DIBO_FOR_SINGLE_LOD

using namespace regen;

namespace regen {
	// Note: I think for this to work we also need to write into indirect draw buffers directly
	//       in spatial index traversal.
	//       So for now we stick to having a duplication instance buffer.
	static constexpr bool LOD_SEPARATE_INSTANCE_BUFFERS = true;

	class InstanceUpdater : public Animation {
	public:
		explicit InstanceUpdater(LODState *lodState)
				: Animation(false, true),
				  lodState_(lodState) {}

		void cpuUpdate(double dt) override {
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
			fixedLOD_ = std::max(1u,mesh_->numLODs()) - 1u; // always use lowest quality LOD
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
	numLODs_ = 1;
	if (shapeIndex_.get()) {
		numLODs_ = std::max(shapeIndex_->numLODs(), numLODs_);
	}
	if (cullShape_.get()) {
		for (auto &part : cullShape_->parts()) {
			numLODs_ = std::max(part->numLODs(), numLODs_);
		}
	}
	if (mesh_.get()) {
		numLODs_ = std::max(mesh_->numLODs(), numLODs_);
	}
	if (useCPUPath()) {
		auto index = cullShape_->spatialIndex();
		shapeIndex_ = index->getIndexedShape(camera_, cullShape_->shapeName());
	}

	if (cullShape_->numInstances() > 1) {
		// create instance buffer for per-frame updates.
		createInstanceBuffer();
		// Create indirect draw buffers for each mesh part and LOD.
		createIndirectDrawBuffers();
	} else if (camera_->numLayer()>1) {
		// we also need indirect draw buffers for multi-layer rendering
		createIndirectDrawBuffers();
	}
	if (!indirectDrawBuffers_.empty()) {
		cullShape_->setIndirectDrawBuffers(indirectDrawBuffers_);
	}
	if (instanceBuffer_.get()) {
		// Make sure that all meshes that share the cull shape also have access to the instance buffer.
		cullShape_->setInstanceBuffer(instanceBuffer_);
		cullShape_->setInstanceSortMode(instanceSortMode_);
	}

	if (useCPUPath()) {
		if (!shapeIndex_.get()) {
			REGEN_WARN("No indexed shape found for cull shape '" << cullShape_->shapeName() << "'.");
		} else {
			shapeIndex_->setInstanceSortMode(instanceSortMode_);
			// Note: we do not update in state enable, but rather use a separate animation for this.
			//   This is done as the animations are dedicated place to write to client buffers,
			//   and we avoid computations in between draw calls, however would still be ok
			//   to update in state traverse.
			lodAnim_ = ref_ptr<InstanceUpdater>::alloc(this);
			lodAnim_->startAnimation();
		}
	} else {
		createComputeShader();
	}
	REGEN_INFO("Created LOD state for cull shape '"
					   << cullShape_->shapeName()
					   << "' with " << cullShape_->numInstances() << " instances, "
					   << numLODs_ << " LODs, "
					   << (cullShape_->isIndexShape() ? "CPU" : "GPU") << " mode.");
}

void LODState::createInstanceBuffer() {
	// Note: make enough space for case where all instances are visible in all layers.
	//       but usually we will write far less data.
	const int32_t numIndices = cullShape_->numInstances() * camera_->numLayer();
	std::vector<uint32_t> clearData(numIndices, 0);

	if constexpr (LOD_SEPARATE_INSTANCE_BUFFERS) {
		instanceData_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numIndices);
		if (cullShape_->isIndexShape()) {
			instanceData_->setInstanceData(1, 1, (byte*)clearData.data());
		}
	} else {
		if (shapeIndex_.get()) {
			instanceData_ = shapeIndex_->instanceIDs();
		} else {
			instanceData_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numIndices);
		}
	}
	instanceBuffer_ = ref_ptr<SSBO>::alloc("InstanceIDs", BufferUpdateFlags::FULL_PER_FRAME);
	instanceBuffer_->addStagedInput(instanceData_);
	instanceBuffer_->update();
	if (!cullShape_->isIndexShape()) {
		instanceBuffer_->setBufferSubData(0, numIndices, clearData.data());
	}
	// Set the instance buffer as input of the LOD state.
	// This should make it available for the mesh state.
	setInput(instanceBuffer_);
}

ref_ptr<SSBO> LODState::createIndirectDrawBuffer(const std::vector<DrawCommand> &initialData) {
	const uint32_t numLayer = camera_->numLayer();
	auto buffer = ref_ptr<DrawIndirectBuffer>::alloc(
			"IndirectDrawBuffer",
			BufferUpdateFlags::FULL_PER_FRAME);
	auto input = ref_ptr<ShaderInputStruct<DrawCommand>>::alloc(
			"DrawCommand", "drawParams", numLODs_ * numLayer);
	if (cullShape_->isIndexShape()) {
		input->setInstanceData(1, 1, (byte*)initialData.data());
	}
	buffer->addStagedInput(input);
	buffer->update();
	if (!cullShape_->isIndexShape()) {
		buffer->setBufferData((byte*)initialData.data());
	}
	return buffer;
}

static inline uint32_t getPartLOD(uint32_t lodLevel, uint32_t numPartLevels, uint32_t numBaseLevels) {
	static const uint32_t lodMappings[5][4] = {
		{0, 0, 0, 0}, // 0 LOD
		{0, 0, 0, 0}, // 1 LODs
		{0, 0, 1, 1}, // 2 LODs
		{0, 1, 2, 2}, // 3 LODs
		{0, 1, 2, 3}  // 4 LODs
	};
	return lodMappings[numPartLevels][lodLevel];
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

		drawData.current.resize(numLODs_ * numLayers);
		drawData.clear.resize(numLODs_ * numLayers);

		// Create the indirect draw data for this part and the first layer.
		// DrawID order: LOD0_layer0, LOD0_layer1, LOD0_layer2
		// 				 LOD1_layer0, LOD1_layer1, LOD1_layer2, ...
		for (uint32_t lodIdx = 0; lodIdx < numLODs_; ++lodIdx) {
			const uint32_t layer0_idx = lodIdx * numLayers;
			const uint32_t partLOD_idx = std::min(lodIdx, std::max(1u,part->numLODs())-1);
			const auto &lodData = partLODs[partLOD_idx];
			DrawCommand &drawParams = drawData.current[layer0_idx];

			Mesh *m = lodData.impostorMesh.get() ? lodData.impostorMesh.get() : part.get();
			if (m->indices().get()) {
				drawParams.mode = 1u; // 1=elements, 2=arrays
				drawParams.setCount(lodData.d->numIndices);
				drawParams.setFirstElement(lodData.d->indexOffset / m->indices()->dataTypeBytes());
				drawParams.data[3] = 0; // base vertex
			} else {
				drawParams.mode = 2u; // 1=elements, 2=arrays
				drawParams.setCount(lodData.d->numVertices);
				drawParams.setFirstElement(lodData.d->vertexOffset);
			}
			drawParams.setInstanceCount(lodIdx==0 ? part->numInstances() : 0);
			drawParams.setBaseInstance(0);

			// Set the clear draw command to zero instance count.
			std::memcpy(
				&drawData.clear[layer0_idx],
				&drawParams,
				sizeof(DrawCommand));
			drawData.clear[layer0_idx].setInstanceCount(0);

			// Copy over the data for the remaining layers.
			for (uint32_t layerIdx = 1; layerIdx < numLayers; ++layerIdx) {
				const uint32_t lodLayerIdx = layer0_idx + layerIdx;
				std::memcpy(
					&drawData.current[lodLayerIdx],
					&drawData.current[layer0_idx],
					sizeof(DrawCommand));
				std::memcpy(
					&drawData.clear[lodLayerIdx],
					&drawData.clear[layer0_idx],
					sizeof(DrawCommand));
			}
		}

		// finally create the indirect draw buffer for this part
		indirectDrawBuffers_[partIdx] = createIndirectDrawBuffer(drawData.current);
	}
}

void LODState::updateVisibility(uint32_t layerIdx, uint32_t lodLevel, uint32_t numInstances, uint32_t baseInstance) {
	const uint32_t numLayer = camera_->numLayer();

	// set the LOD level
	for (uint32_t partIdx = 0; partIdx < cullShape_->parts().size(); ++partIdx) {
		if (indirectDrawBuffers_.empty()) {
			auto &part = cullShape_->parts()[partIdx];
			part->updateVisibility(
					getPartLOD(lodLevel, part->numLODs(), numLODs_),
					numInstances, baseInstance);
		} else {
			auto &current = indirectDrawData_[partIdx].current;
			auto &drawParams = current[numLayer * lodLevel + layerIdx];
			drawParams.setInstanceCount(numInstances);
			drawParams.setBaseInstance(baseInstance);
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
			std::memcpy(
				indirectDrawData_[partIdx].current.data(),
				indirectDrawData_[partIdx].clear.data(),
				indirectDrawBuffers_[partIdx]->inputSize());
		}
	}
}

ref_ptr<SSBO> LODState::getIndirectDrawBuffer(const ref_ptr<Mesh> &mesh) const {
	return cullShape_->getIndirectDrawBuffer(mesh);
}

void LODState::enable(RenderState *rs) {
	State::enable(rs);

	if (useGPUPath()) {
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
	else if (indirectDrawBuffers_.empty()) {
		// This means that we have only a single layer and a single instance.
		// Because in all other cases, an indirect draw buffer is created.
		// -> here we can safely adjust the LOD state of the mesh for its next traversal.

		auto counts = shapeIndex_->mapInstanceCounts(BUFFER_GPU_READ);
		int32_t lodLevelWithInstance = -1;
		for (uint32_t lodIdx = 0; lodIdx < numLODs_; ++lodIdx) {
			const uint32_t binIdx = lodIdx * camera_->numLayer(); // layerIdx=0
			if (counts.r[binIdx] != 0) {
				lodLevelWithInstance = lodIdx;
				break;
			}
		}
		if (lodLevelWithInstance != -1) {
			// set the LOD level for the mesh
			for (auto &part : cullShape_->parts()) {
				auto partLODLevel = getPartLOD(lodLevelWithInstance, part->numLODs(), numLODs_);
				part->updateVisibility(partLODLevel, part->numInstances(), 0);
			}
		}
	}
#ifdef LOD_DEBUG_GROUPS
	REGEN_INFO("LOD for shape '" << cullShape_->shapeName() << "'"
		<< " with " << cullShape_->numInstances() << " instances, "
		<< (cullShape_->isIndexShape() ? "CPU" : "GPU") << " mode "
		<< cullShape_->parts().size() << " parts, "
		<< " and shadow target: " << (hasShadowTarget_ ? "1" : "0")
		);
	if (!indirectDrawBuffers_.empty()) {
		// map indirect buffer and print the number of instances per LOD
		static std::vector<DrawCommand> readVec(4 * 8); // max 4 LODs, max 8 layers
		for (uint32_t partIdx= 0; partIdx < indirectDrawBuffers_.size(); ++partIdx) {
			auto indirectBuffer = indirectDrawBuffers_[partIdx];
			auto &part = cullShape_->parts()[partIdx];
			indirectBuffer->readBufferSubData(
					0, numLODs_ * camera_->numLayer() * sizeof(DrawCommand), (byte *)readVec.data());
			// print the number of instances per LOD
			REGEN_INFO("  - GPU ("
							   << std::setw(4) << std::setfill(' ') << readVec[0].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << readVec[1].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << readVec[2].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << readVec[3].instanceCount() << ")"
							   << " numLODs: " <<
							   std::setw(2) << std::setfill(' ') << part->numLODs()
							   << " part: " << partIdx);
			for (uint32_t i = 0; i < numLODs_; ++i) {
			for (uint32_t j = 0; j < camera_->numLayer(); ++j) {
				auto binIdx = i * camera_->numLayer() + j;
				REGEN_INFO("   - DIBO " << i << "." << j << " -- "
								<< "mode: " << readVec[i].mode << "; data: ["
								   << std::setw(8) << readVec[binIdx].data[0] << ", "
								   << std::setw(8) << readVec[binIdx].data[1] << ", "
								   << std::setw(8) << readVec[binIdx].data[2] << ", "
								   << std::setw(8) << readVec[binIdx].data[3] << ", "
								   << std::setw(4) << readVec[binIdx].data[4] << ", "
								   << readVec[binIdx]._pad[0] << ", "
								   << readVec[binIdx]._pad[1] << "]");
			}}
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
	const uint32_t numLayer = camera_->numLayer();
	bool hasVisibleInstance = shapeIndex_->isVisibleInAnyLayer();
	if (!hasVisibleInstance_ && !hasVisibleInstance) {
		// No instance is visible, early exit.
		// Note: the LOD state has been reset before, so nothing to do here.
		return;
	}
	hasVisibleInstance_ = hasVisibleInstance;

	if (hasVisibleInstance_) {
		if (cullShape_->numInstances() == 1) {
			auto count = shapeIndex_->mapInstanceCounts(BUFFER_CPU_READ);
			auto base = shapeIndex_->mapBaseInstances(BUFFER_CPU_READ);

			for (uint32_t lodLevel=0; lodLevel<numLODs_; ++lodLevel) {
				for (uint32_t layerIdx=0; layerIdx<numLayer; ++layerIdx) {
					const uint32_t binIdx =  CullShape::binIdx(lodLevel, layerIdx, numLayer);
					if (count.r[binIdx] != 0) {
						updateVisibility(layerIdx, lodLevel, 1, 0);
					}
				}
			}
		} else if (camera_->hasFixedLOD()) {
			auto count = shapeIndex_->mapInstanceCounts(BUFFER_CPU_READ);
			auto base = shapeIndex_->mapBaseInstances(BUFFER_CPU_READ);

			for (uint32_t lodLevel=0; lodLevel<numLODs_; ++lodLevel) {
			for (uint32_t layerIdx=0; layerIdx<numLayer; ++layerIdx) {
				const uint32_t binIdx =  CullShape::binIdx(lodLevel, layerIdx, numLayer);
				if (count.r[binIdx] != 0) {
					updateVisibility(layerIdx, fixedLOD_, count.r[binIdx], base.r[binIdx]);
				}
			}}
		} else {
			auto count = shapeIndex_->mapInstanceCounts(BUFFER_CPU_READ);
			auto base = shapeIndex_->mapBaseInstances(BUFFER_CPU_READ);

			// update local data of indirect draw buffers
			for (uint32_t lodLevel=0; lodLevel<numLODs_; ++lodLevel) {
				for (uint32_t layer=0; layer<numLayer; ++layer) {
					const uint32_t binIdx = CullShape::binIdx(lodLevel, layer, numLayer);
					if (count.r[binIdx] != 0) {
						updateVisibility(layer, lodLevel, count.r[binIdx], base.r[binIdx]);
					}
				}
			}

			if constexpr (LOD_SEPARATE_INSTANCE_BUFFERS) {
				auto ids = shapeIndex_->mapInstanceIDs(BUFFER_CPU_READ);
				if (tfStamp_ != cullShape_->boundingShape()->tfStamp() || cameraStamp_ != camera_->stamp()) {
					tfStamp_ = cullShape_->boundingShape()->tfStamp();
					cameraStamp_ = camera_->stamp();

					const uint32_t lastBinIdx = (numLODs_ * numLayer) - 1;
					const uint32_t numVisible = base.r[lastBinIdx] + count.r[lastBinIdx];
					const uint32_t dataSize = numVisible * sizeof(uint32_t);

					auto mappedClientData = instanceData_->mapClientData<uint32_t>(
							BUFFER_GPU_WRITE, 0, dataSize);
					std::memcpy(mappedClientData.w.data(), ids.r.data(), dataSize);
				}
			}
		}
	}

	// update client buffer of indirect draw buffers
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

///////////////////////
//////////// GPU-based LOD update
///////////////////////

void LODState::createComputeShader() {
	{	// Create a static indirect draw buffer, which is used for clearing the
		// first indirect draw buffer each frame.
		auto clearData = ref_ptr<ShaderInputStruct<DrawCommand>>::alloc(
					"DrawCommand", "drawParams", numLODs_ * camera_->numLayer());
		clearData->setUniformUntyped((byte*)indirectDrawData_[0].clear.data());
		clearIndirectBuffer_ = ref_ptr<SSBO>::alloc(
			"Clear_IndirectDrawBuffer",
			BufferUpdateFlags::NEVER,
			SSBO::RESTRICT);
		clearIndirectBuffer_->addStagedInput(clearData);
		clearIndirectBuffer_->update();
	}
	{	// Create a static buffer with zeroes for resetting the number of visible instances.
		std::vector<uint32_t> clearData(camera_->numLayer(), 0);
		clearNumVisibleBuffer_ = ref_ptr<SSBO>::alloc(
			"Clear_NumVisibleBuffer",
			BufferUpdateFlags::NEVER,
			SSBO::RESTRICT);
		auto clearInput = ref_ptr<ShaderInput1ui>::alloc(
			"numVisibleKeys", camera_->numLayer());
		clearInput->setUniformUntyped((byte*)clearData.data());
		clearNumVisibleBuffer_->addStagedInput(clearInput);
		clearNumVisibleBuffer_->update();
	}

	{ // radix sort
		radixSort_ = ref_ptr<RadixSort_GPU>::alloc(cullShape_->numInstances(), camera_->numLayer());
		radixSort_->setUseCompaction(useCompaction_);
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
		if (useCompaction_) {
			shaderCfg.define("USE_COMPACTION", "TRUE");
		}
		cullPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.cull");
		cullPass_->computeState()->shaderDefine("LOD_NUM_INSTANCES", REGEN_STRING(cullShape_->numInstances()));
		cullPass_->computeState()->shaderDefine("NUM_LAYERS", REGEN_STRING(camera_->frustum().size()));
		// X-direction: one work unit per instance
		// Y-direction: one work unit per layer
		cullPass_->computeState()->setNumWorkUnits(
			static_cast<int>(cullShape_->numInstances()),
			static_cast<int>(camera_->frustum().size()), 1);
		cullPass_->computeState()->setGroupSize(RADIX_GROUP_SIZE, 1, 1);
		cullPass_->setInput(mesh_->u_lodThresholds());
		cullPass_->setInput(camera_->getFrustumBuffer());
		// Note: LOD pass only writes into first buffer, we need to copy into the other buffers
		//       in a separate pass.
		cullPass_->setInput(indirectDrawBuffers_[0]);
		cullPass_->setInput(radixSort_->keyBuffer());
		cullPass_->setInput(instanceBuffer_);
		auto boundingShape = mesh_->boundingShape();
		if (boundingShape->shapeType() == BoundingShapeType::SPHERE) {
			auto *sphere = static_cast<BoundingSphere*>(boundingShape.get());
			cullPass_->setInput(createUniform<ShaderInput1f, float>(
					"shapeRadius", sphere->radius()));
			shaderCfg.define("SHAPE_TYPE", "SPHERE");
		}
		else if (boundingShape->shapeType() == BoundingShapeType::AABB) {
			auto *box = static_cast<AABB*>(boundingShape.get());
			cullPass_->setInput(createUniform<ShaderInput4f, Vec4f>(
					"shapeAABBMin", Vec4f::create(box->baseBounds().min,0.0f)));
			cullPass_->setInput(createUniform<ShaderInput4f, Vec4f>(
					"shapeAABBMax", Vec4f::create(box->baseBounds().max,0.0f)));
			shaderCfg.define("SHAPE_TYPE", "AABB");
		}
		else if (boundingShape->shapeType() == BoundingShapeType::OBB) {
			auto *box = static_cast<OBB*>(boundingShape.get());
			cullPass_->setInput(createUniform<ShaderInput4f, Vec4f>(
					"shapeAABBMin", Vec4f::create(box->baseBounds().min,0.0f)));
			cullPass_->setInput(createUniform<ShaderInput4f, Vec4f>(
					"shapeAABBMax", Vec4f::create(box->baseBounds().max,0.0f)));
			shaderCfg.define("SHAPE_TYPE", "OBB");
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
		copyIndirect_->setInput(indirectDrawBuffers_[0],
					"IndirectDrawBuffer0", "Base");
		for (uint32_t indirectIdx = 1; indirectIdx < indirectDrawBuffers_.size(); ++indirectIdx) {
			copyIndirect_->setInput(indirectDrawBuffers_[indirectIdx],
					REGEN_STRING("IndirectDrawBuffer" << indirectIdx),
					REGEN_STRING(indirectIdx-1));
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
	if (useCompaction_) {
		// clear the visibility count to zero
		auto &keys = radixSort_->keyBuffer()->allocations()[0];
		auto &clear = clearNumVisibleBuffer_->allocations()[0];
		glCopyNamedBufferSubData(
			clear->bufferID(),
			keys->bufferID(),
			clear->address(),
			keys->address(),
			clear->allocatedSize());
	}
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
