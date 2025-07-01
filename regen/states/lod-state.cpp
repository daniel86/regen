#include <regen/states/state-node.h>
#include "lod-state.h"
#include "regen/meshes/mesh-vector.h"
#include "regen/gl-types/gl-param.h"
#include "regen/utility/conversion.h"
#include "regen/camera/light-camera.h"
#include "regen/gl-types/draw-command.h"

#define RADIX_BITS_PER_PASS 4u
#define RADIX_GROUP_SIZE 256
#define RADIX_OFFSET_GROUP_SIZE 512
//#define LOD_DEBUG_GROUPS
#undef LOD_DEBUG_CPU_TIME

using namespace regen;

static inline void reverse_copy_u32(uint32_t *__restrict dst, const uint32_t *__restrict src, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		dst[i] = src[count - 1 - i];
	}
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
	frustumPlanes_.resize(6 * camera_->frustum().size());
	if (cullShape_->isIndexShape()) {
		auto index = cullShape_->spatialIndex();
		shapeIndex_ = index->getIndexedShape(camera_, cullShape_->shapeName());
		if (!shapeIndex_.get()) {
			REGEN_WARN("No indexed shape found for cull shape '" << cullShape_->shapeName() << "'.");
		}
	} else {
		createComputeShader();
	}
	REGEN_INFO("Created LOD state for cull shape '"
					   << cullShape_->shapeName()
					   << "' with " << cullShape_->numInstances() << " instances, "
					   << mesh_->numLODs() << " LODs, "
					   << (cullShape_->isIndexShape() ? "CPU" : "GPU") << " mode.");
}

void LODState::updateMeshLOD() {
	if (!mesh_.get()) { return; }
	// set LOD level based on distance
	auto camPos = camera_->position()->getVertex(0);
	auto distanceSquared = (shapeIndex_->shape()->getShapeOrigin() - camPos.r.xyz_()).lengthSquared();
	camPos.unmap();
	updateVisibility(
			mesh_->getLODLevel(distanceSquared),
			1, 0);
}

static inline uint32_t getPartLOD(uint32_t lodLevel, uint32_t numPartLevels, uint32_t numBaseLevels) {
	if (numPartLevels == numBaseLevels && lodLevel < numBaseLevels) {
		// part has same number of LODs as mesh, return the LOD level
		return lodLevel;
	} else if (numPartLevels == 1) {
		return 0;
	} else if (numPartLevels < numBaseLevels || lodLevel >= numPartLevels) {
		// adjust the LOD level for parts
		if (numPartLevels == 2) {
			if (lodLevel < 2) {
				return 0;
			} else {
				return 1;
			}
		} else if (numPartLevels == 3) {
			if (lodLevel == 0) {
				return 0;
			} else if (lodLevel == 3) {
				return 2;
			} else {
				return 1;
			}
		}
	}
	return lodLevel;
}

void LODState::updateVisibility(uint32_t lodLevel, uint32_t numInstances, uint32_t instanceOffset) {
	// increase LOD level by one if we have a shadow target
	if (!camera_->hasFixedLOD()) {
		//if (hasShadowTarget_ && lodLevel < mesh_->numLODs() - 1) {
		//	lodLevel++;
		//}
	}
	// set the LOD level
	for (auto &part: cullShape_->parts()) {
		// could be part has different number of LODs, need to compute an adjusted
		// LOD level for each part
		auto partLODLevel = getPartLOD(lodLevel, part->numLODs(), mesh_->numLODs());
		auto &partLOD = part->meshLODs()[partLODLevel];
		if (partLOD.d->numVisibleInstances > 0u) {
			part->updateVisibility(partLODLevel,
								   partLOD.d->numVisibleInstances + numInstances,
								   partLOD.d->instanceOffset);
		} else {
			part->updateVisibility(partLODLevel, numInstances, instanceOffset);
		}
	}
}

void LODState::resetVisibility() {
	for (auto &part: cullShape_->parts()) {
		for (uint32_t lodLevel = 0; lodLevel < part->numLODs(); ++lodLevel) {
			part->updateVisibility(lodLevel, 0, 0);
		}
	}
}

void LODState::enable(RenderState *rs) {
	State::enable(rs);
#ifdef LOD_DEBUG_CPU_TIME
	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;
	using std::chrono::milliseconds;
	auto t1 = high_resolution_clock::now();
#endif
	resetVisibility();
	if (cullShape_->isIndexShape()) {
		traverseCPU(rs);
	} else {
		traverseGPU(rs);
	}
#ifdef LOD_DEBUG_GROUPS
	if (!indirectDrawBuffers_.empty()) {
		// map indirect buffer and print the number of instances per LOD
		auto indirectBuffer = indirectDrawBuffers_[0];
		auto *data = (DrawCommand *) indirectBuffer->map(
				indirectBuffer->blockReference(), GL_MAP_READ_BIT);
		if (data) {
			// print the number of instances per LOD
			REGEN_INFO("LOD ("
							   << std::setw(4) << std::setfill(' ') << data[0].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << data[1].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << data[2].instanceCount() << " "
							   << std::setw(4) << std::setfill(' ') << data[3].instanceCount() << ")"
							   << " numInstances: " <<
							   std::setw(5) << std::setfill(' ') << cullShape_->numInstances()
							   << " numLODs: " <<
							   std::setw(2) << std::setfill(' ') << mesh_->numLODs()
							   << " mode: " << (cullShape_->isIndexShape() ? "CPU" : "GPU")
							   << " shadow: " << (hasShadowTarget_ ? "1" : "0")
							   << " shape: " << cullShape_->shapeName());
			for (uint32_t i = 0; i < mesh_->numLODs(); ++i) {
				REGEN_INFO("   Indirect buffer " << i << " -- "
								<< "mode: " << data[i].mode << "; data: ["
								   << std::setw(8) << data[i].data[0] << ", "
								   << std::setw(8) << data[i].data[1] << ", "
								   << std::setw(8) << data[i].data[2] << ", "
								   << std::setw(8) << data[i].data[3] << ", "
								   << std::setw(4) << data[i].data[4] << ", "
								   << data[i]._pad[0] << ", "
								   << data[i]._pad[1] << "]");
			}
			indirectBuffer->unmap();
		}
	} else if (mesh_.get()) {
		if (mesh_->numLODs() > 1) {
			REGEN_INFO("LOD ("
						   << std::setw(4) << std::setfill(' ') << lodNumInstances_[0] << " "
						   << std::setw(4) << std::setfill(' ') << lodNumInstances_[1] << " "
						   << std::setw(4) << std::setfill(' ') << lodNumInstances_[2] << " "
						   << std::setw(4) << std::setfill(' ') << lodNumInstances_[3] << ")"
						   << " numInstances: " <<
						   std::setw(5) << std::setfill(' ') << cullShape_->numInstances()
						   << " numLODs: " <<
						   std::setw(2) << std::setfill(' ') << mesh_->numLODs()
						   << " mode: " << (cullShape_->isIndexShape() ? "CPU" : "GPU")
						   << " shadow: " << (hasShadowTarget_ ? "1" : "0")
						   << " shape: " << cullShape_->shapeName()
						   );
		} else {
			REGEN_INFO("LOD ("
						   << std::setw(4) << std::setfill(' ') << lodNumInstances_[0] << ")"
						   << " numInstances: " <<
						   std::setw(5) << std::setfill(' ') << cullShape_->numInstances()
						   << " numLODs: " <<
						   std::setw(2) << std::setfill(' ') << mesh_->numLODs()
						   << " mode: " << (cullShape_->isIndexShape() ? "CPU" : "GPU")
						   << " shape: " << cullShape_->shapeName());
		}
	}
#endif
#ifdef LOD_DEBUG_CPU_TIME
	auto t2 = high_resolution_clock::now();
	duration<double, std::milli> ms_double = t2 - t1;
	REGEN_INFO("LOD time: " << ms_double.count() << " ms"
							<< " shape: " << cullShape_->shapeName());
#endif
}

///////////////////////
//////////// CPU-based LOD update
///////////////////////

void LODState::traverseCPU(RenderState *) {
	if (!shapeIndex_.get() || !shapeIndex_->isVisible()) {
		return;
	}

	if (cullShape_->numInstances() == 1) {
		if (shapeIndex_->isVisible()) {
			updateMeshLOD();
		}
	} else if (camera_->hasFixedLOD()) {
		updateVisibility(fixedLOD_, shapeIndex_->numVisibleInstances(), 0);
	} else {
		if (tfStamp_ != cullShape_->tf()->stamp() || cameraStamp_ != camera_->stamp()) {
			// recompute LOD groups if the transform or camera has changed
			computeLODGroups();
			tfStamp_ = cullShape_->tf()->stamp();
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
	auto visible_ids = shapeIndex_->mapInstanceIDs(ShaderData::READ);
	auto numVisible = visible_ids.r[0];
	if (numVisible == 0) { return; }

	const uint32_t *mappedData = visible_ids.r + 1;
	auto &tf = cullShape_->tf();
	auto camPos = camera_->position()->getVertex(0);
	bool hasTF = tf.get() && (tf->hasModelOffset() || tf->hasModelMat());

	if (hasTF) {
		auto &modelOffset = tf->modelOffset();
		auto &modelMat = tf->modelMat();
		if (tf->hasModelOffset() && tf->hasModelMat()) {
			auto modelOffsetData = modelOffset->mapClientData<Vec4f>(ShaderData::READ);
			auto tfData = modelMat->mapClientData<Mat4f>(ShaderData::READ);
			LODSelector_Full selector{
					.tfData = tfData.r,
					.modelOffsetData = modelOffsetData.r,
					.mappedData = mappedData,
					.mesh = mesh_.get(),
					.tfIdxMultiplier = (modelMat->numInstances() > 1u ? 1u : 0u),
					.offsetIdxMultiplier = (modelOffset->numInstances() > 1u ? 1u : 0u)
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos.r.xyz_(),
							   selector);
		} else if (tf->hasModelOffset()) {
			auto modelOffsetData = modelOffset->mapClientData<Vec4f>(ShaderData::READ);
			LODSelector_ModelOffset selector{
					.modelOffsetData = modelOffsetData.r,
					.mappedData = mappedData,
					.mesh = mesh_.get()
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos.r.xyz_(),
							   selector);
		} else {
			auto tfData = modelMat->mapClientData<Mat4f>(ShaderData::READ);
			LODSelector_Transform selector{
					.tfData = tfData.r,
					.mappedData = mappedData,
					.mesh = mesh_.get()
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos.r.xyz_(),
							   selector);
		}
	} else {
		for (size_t i = 1; i < lodNumInstances_.size(); ++i) {
			lodNumInstances_[i] = 0;
		}
		lodNumInstances_[0] = numVisible;
	}

	// write lodGroups_ data into instanceIDMap_
	auto &instanceIDMap = cullShape_->instanceIDMap();
	auto instance_ids = (uint32_t *) instanceIDMap->clientData();
	if (instanceSortMode_ == SortMode::BACK_TO_FRONT) {
		reverse_copy_u32(instance_ids, mappedData, numVisible);
	} else {
		std::memcpy(instance_ids, mappedData, numVisible * sizeof(uint32_t));
	}
	instanceIDMap->nextStamp();
}

///////////////////////
//////////// GPU-based LOD update
///////////////////////

void LODState::createComputeShader() {
	DrawCommand drawParams[4];
	indirectDrawBuffers_.resize(cullShape_->parts().size());

	for (uint32_t partIdx=0; partIdx < cullShape_->parts().size(); ++partIdx) {
    	std::string suffix = (partIdx==0 ? "Base" : REGEN_STRING(partIdx-1));
		auto &part = cullShape_->parts()[partIdx];
		// we need to create a drawParams for each part, since they can have different
		// number of indices and different index buffers.
		auto &meshLODs = part->meshLODs();
		for (uint32_t i = 0; i < 4; ++i) {
			ref_ptr<Mesh> m;
			if (i < meshLODs.size()) {
				m = meshLODs[i].impostorMesh.get() ? meshLODs[i].impostorMesh : part;
			} else {
				m = part;
			}
			auto &indices = m->inputContainer()->indices();
			if (indices.get()) {
				drawParams[i].mode = 1u; // 1=elements, 2=arrays
				drawParams[i].setCount(m->inputContainer()->numIndices());
				drawParams[i].setFirstElement(indices->offset() / sizeof(uint32_t));
			} else {
				drawParams[i].mode = 2u; // 1=elements, 2=arrays
				drawParams[i].setCount(m->inputContainer()->numVertices());
				drawParams[i].setFirstElement(m->inputContainer()->vertexOffset());
			}
			drawParams[i].setInstanceCount(i==0 ? m->inputContainer()->numInstances() : 0);
		}
		auto idb = ref_ptr<ShaderInputStruct<DrawCommand>>::alloc(
				"DrawCommand",
				REGEN_STRING("drawParams"<<suffix),
				4);
		idb->setUniformUntyped((byte*)(&drawParams[0]));
		// create an indirect draw buffer, which is computed each frame
		indirectDrawBuffers_[partIdx] = ref_ptr<SSBO>::alloc(
				REGEN_STRING("IndirectDrawBuffer"<<suffix),
				BUFFER_USAGE_STREAM_DRAW, SSBO::RESTRICT);
		indirectDrawBuffers_[partIdx]->addBlockInput(idb);
		indirectDrawBuffers_[partIdx]->update();
		// TODO use a single buffer with offsets
		uint32_t partDrawIdx = 0;
		part->setIndirectDrawBuffer(
				indirectDrawBuffers_[partIdx],
				partDrawIdx);

		if(partIdx==0) {
			// Create a static indirect draw buffer, which is used for clearing the
			// indirect draw buffer each frame.
			for (uint32_t i = 0; i < 4; ++i) {
				drawParams[i].setInstanceCount(0u);
			}
			auto clearData = ref_ptr<ShaderInputStruct<DrawCommand>>::alloc(
					"DrawCommand",
					REGEN_STRING("drawParams"<<suffix),
					4);
			clearData->setUniformUntyped((byte*)(&drawParams[0]));
			clearIndirectBuffer_ = ref_ptr<SSBO>::alloc(
				REGEN_STRING("IndirectDrawBuffer"<<suffix),
				BUFFER_USAGE_STATIC_COPY, SSBO::RESTRICT);
			clearIndirectBuffer_->addBlockInput(clearData);
			clearIndirectBuffer_->update();
		}
	}

	{ // radix sort
		radixSort_ = ref_ptr<RadixSort>::alloc(cullShape_->numInstances());
		radixSort_->setOutputBuffer(cullShape_->instanceIDBuffer());
		radixSort_->setRadixBits(RADIX_BITS_PER_PASS);
		radixSort_->setSortGroupSize(RADIX_GROUP_SIZE);
		radixSort_->setScanGroupSize(RADIX_OFFSET_GROUP_SIZE);
		radixSort_->createResources();
	}

	{ // cull
		// we store the 6 frustum planes in a UBO
		frustumUBO_ = ref_ptr<UBO>::alloc("FrustumBuffer");
		frustumUBO_->addBlockInput(ref_ptr<ShaderInput4f>::alloc("frustumPlanes", frustumPlanes_.size()));
		frustumUBO_->update();

		StateConfigurer shaderCfg;
		if (instanceSortMode_ == SortMode::BACK_TO_FRONT) {
			shaderCfg.define("USE_REVERSE_SORT", "TRUE");
		}
		cullPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.cull");
		cullPass_->computeState()->shaderDefine("LOD_NUM_INSTANCES", REGEN_STRING(cullShape_->numInstances()));
		cullPass_->computeState()->shaderDefine("NUM_CAMERA_LAYERS", REGEN_STRING(camera_->frustum().size()));
		cullPass_->computeState()->setNumWorkUnits(static_cast<int>(cullShape_->numInstances()), 1, 1);
		cullPass_->computeState()->setGroupSize(RADIX_GROUP_SIZE, 1, 1);
		cullPass_->joinShaderInput(mesh_->lodThresholds());
		cullPass_->joinShaderInput(frustumUBO_);
		// Note: LOD pass only writes into first buffer, we need to copy into the other buffers
		//       in a separate pass.
		cullPass_->joinShaderInput(indirectDrawBuffers_[0]);
		cullPass_->joinShaderInput(radixSort_->keyBuffer());
		cullPass_->joinShaderInput(cullShape_->instanceIDBuffer());
		auto boundingShape = mesh_->boundingShape();
		if (boundingShape->shapeType() == BoundingShapeType::SPHERE) {
			auto *sphere = dynamic_cast<BoundingSphere*>(boundingShape.get());
			cullPass_->joinShaderInput(createUniform<ShaderInput1f, float>(
					"shapeRadius", sphere->radius()));
			shaderCfg.define("SHAPE_TYPE", "SPHERE");
		}
		else if (boundingShape->shapeType() == BoundingShapeType::BOX) {
			auto *box = dynamic_cast<BoundingBox*>(boundingShape.get());
			cullPass_->joinShaderInput(createUniform<ShaderInput4f, Vec4f>(
					"shapeAABBMin", Vec4f(box->bounds().min,0.0f)));
			cullPass_->joinShaderInput(createUniform<ShaderInput4f, Vec4f>(
					"shapeAABBMax", Vec4f(box->bounds().max,0.0f)));
			if (box->isAABB()) {
				shaderCfg.define("SHAPE_TYPE", "AABB");
			} else {
				shaderCfg.define("SHAPE_TYPE", "OBB");
			}
		}
		cullPass_->joinStates(cullShape_->tf());
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
		for (const auto & indirectDrawBuffer : indirectDrawBuffers_) {
			copyIndirect_->joinShaderInput(indirectDrawBuffer);
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
	// copy the clear buffer to the indirect draw buffer
	rs->copyReadBuffer().push(clearIndirectBuffer_->blockReference()->bufferID());
	rs->shaderStorageBuffer().apply(indirectDrawBuffers_[0]->blockReference()->bufferID());
	glCopyBufferSubData(
			GL_COPY_READ_BUFFER,
			GL_SHADER_STORAGE_BUFFER,
			clearIndirectBuffer_->blockReference()->address(),
			indirectDrawBuffers_[0]->blockReference()->address(),
			clearIndirectBuffer_->blockReference()->allocatedSize());
	rs->copyReadBuffer().pop();

	if (cameraStamp_ != camera_->stamp()) {
		// Update the frustum planes in the UBO
		cameraStamp_ = camera_->stamp();
		auto &frustum = camera_->frustum();
		for (size_t i = 0; i < frustum.size(); ++i) {
			auto &frustumPlanes = frustum[i].planes;
			for (int j = 0; j < 6; ++j) {
				frustumPlanes_[i*6 + j] = frustumPlanes[j].equation();
			}
		}
		rs->uniformBuffer().apply(frustumUBO_->blockReference()->bufferID());
		glBufferSubData(
				GL_UNIFORM_BUFFER,
				frustumUBO_->blockReference()->address(),
				frustumUBO_->blockReference()->allocatedSize(),
				&frustumPlanes_[0].x);
	}
	if (tfStamp_ != cullShape_->tf()->stamp()) {
		// Update the transform in the cull pass
		tfStamp_ = cullShape_->tf()->stamp();
	}

	// compute lod, write keys, and initialize values_[0] (instanceIDMap_)
	cullPass_->enable(rs);
	cullPass_->disable(rs);

	radixSort_->enable(rs);
	radixSort_->disable(rs);

	if (copyIndirect_.get()) {
		// update the indirect draw buffers for the other parts
		copyIndirect_->enable(rs);
		copyIndirect_->disable(rs);
	}
}
