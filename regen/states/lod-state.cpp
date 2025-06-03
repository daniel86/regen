#include <regen/states/state-node.h>
#include "lod-state.h"
#include "regen/meshes/mesh-vector.h"
#include "regen/gl-types/gl-param.h"
#include "regen/utility/conversion.h"
#include "regen/camera/light-camera.h"

#define RADIX_BITS_PER_PASS 4u
#define RADIX_GROUP_SIZE 256
#define RADIX_OFFSET_GROUP_SIZE 512
#undef LOD_DEBUG_GROUPS
#undef LOD_DEBUG_CPU_TIME

using namespace regen;

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
		REGEN_WARN("No mesh set for shape");
	}
	initLODState();
}

void LODState::initLODState() {
	lodNumInstances_.resize(4);
	lodBoundaries_.resize(5);
	lodBoundaries_[0] = 0;
	// initially all instances are added to first LOD group
	lodNumInstances_[0] = cullShape_->numInstances();
	for (uint32_t i = 1u; i < 4; ++i) {
		lodNumInstances_[i] = 0;
	}
	if (cullShape_->isIndexShape()) {
		auto index = cullShape_->spatialIndex();
		shapeIndex_ = index->getIndexedShape(camera_, cullShape_->shapeName());
		if (!shapeIndex_.get()) {
			REGEN_WARN("No indexed shape found for cull shape '" << cullShape_->shapeName() << "'.");
		}
	} else {
		createComputeShader();
	}
}

void LODState::updateMeshLOD() {
	if (!mesh_.get()) { return; }
	// set LOD level based on distance
	auto camPos = camera_->position()->getVertex(0);
	auto distance = (shapeIndex_->shape()->getCenterPosition() - camPos.r).length();
	camPos.unmap();
	updateVisibility(
			mesh_->getLODLevel(distance),
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
	if (hasShadowTarget_ && lodLevel < mesh_->numLODs() - 1) {
		lodLevel++;
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
	if (mesh_.get() && mesh_->numLODs() > 1) {
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
						   << " shape: " << cullShape_->shapeName());
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
	} else {
		computeLODGroups();

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

static inline void reverse_copy_u32(uint32_t *__restrict dst, const uint32_t *__restrict src, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		dst[i] = src[count - 1 - i];
	}
}

struct LODSelector_Full {
	const Mat4f *tfData;
	const Vec3f *modelOffsetData;
	const uint32_t *mappedData;
	const Mesh *mesh;

	inline uint32_t operator()(uint32_t i, const Vec3f &camPos) const {
		return mesh->getLODLevel((
										 tfData[mappedData[i]].position() +
										 modelOffsetData[mappedData[i]] - camPos).lengthSquared());
	}
};

struct LODSelector_ModelOffset {
	const Vec3f *modelOffsetData;
	const uint32_t *mappedData;
	const Mesh *mesh;

	inline uint32_t operator()(uint32_t i, const Vec3f &camPos) const {
		return mesh->getLODLevel((
										 modelOffsetData[mappedData[i]] - camPos).lengthSquared());
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
	auto &transform = cullShape_->tf();
	auto &modelOffset = cullShape_->modelOffset();
	auto camPos = camera_->position()->getVertex(0);
	bool hasTF = transform.get() || modelOffset.get();

	if (hasTF) {
		if (transform.get() && modelOffset.get()) {
			auto modelOffsetData = modelOffset->mapClientData<Vec3f>(ShaderData::READ);
			auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
			LODSelector_Full selector{
					.tfData = tfData.r,
					.modelOffsetData = modelOffsetData.r,
					.mappedData = mappedData,
					.mesh = mesh_.get()
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos.r,
							   selector);
		} else if (modelOffset.get()) {
			auto modelOffsetData = modelOffset->mapClientData<Vec3f>(ShaderData::READ);
			LODSelector_ModelOffset selector{
					.modelOffsetData = modelOffsetData.r,
					.mappedData = mappedData,
					.mesh = mesh_.get()
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos.r,
							   selector);
		} else {
			auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
			LODSelector_Transform selector{
					.tfData = tfData.r,
					.mappedData = mappedData,
					.mesh = mesh_.get()
			};
			countGroupSize_CPU(numVisible,
							   lodNumInstances_, lodBoundaries_,
							   camPos.r,
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
	auto instance_ids = (uint32_t*)instanceIDMap->clientData();
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
	// Output: lodGroupSize
	lodGroupSizeBuffer_ = ref_ptr<SSBO>::alloc("LODGroupBuffer", BUFFER_USAGE_STREAM_COPY, SSBO::RESTRICT);
	lodGroupSize_ = ref_ptr<ShaderInput1ui>::alloc("lodGroupSize", 4);
	lodGroupSizeBuffer_->addBlockInput(lodGroupSize_);
	lodGroupSizeBuffer_->update();
	// +PBO for reading back the lodGroupSizeBuffer_
	lodGroupSizeMapping_ = ref_ptr<BufferStructMapping<Vec4ui>>::alloc(
			BufferMapping::READ | BufferMapping::PERSISTENT | BufferMapping::COHERENT,
			BufferMapping::DOUBLE_BUFFER);

	{ // radix sort
		radixSort_ = ref_ptr<RadixSort>::alloc(cullShape_->numInstances());
		radixSort_->setOutputBuffer(cullShape_->instanceIDBuffer());
		radixSort_->setRadixBits(RADIX_BITS_PER_PASS);
		radixSort_->setSortGroupSize(RADIX_GROUP_SIZE);
		radixSort_->setScanGroupSize(RADIX_OFFSET_GROUP_SIZE);
		radixSort_->createResources();
	}

	{ // radix cull
		cullUBO_ = ref_ptr<UBO>::alloc("CullUBO");
		// TODO: Allow meshes to have different thresholds depending on render target/camera.
		//       e.g. for shadow mapping we never need to use the highest LOD.
		cullUBO_->addBlockInput(mesh_->lodThresholds());
		cullUBO_->update();
		// we store the 6 frustum planes in a UBO
		frustumUBO_ = ref_ptr<UBO>::alloc("FrustumBuffer");
		frustumUBO_->addBlockInput(ref_ptr<ShaderInput4f>::alloc("frustumPlanes", 6));
		frustumUBO_->update();

		StateConfigurer shaderCfg;
		if (instanceSortMode_ == SortMode::BACK_TO_FRONT) {
			shaderCfg.define("USE_REVERSE_SORT", "TRUE");
		}
		cullPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.cull");
		cullPass_->computeState()->shaderDefine("LOD_NUM_INSTANCES", REGEN_STRING(cullShape_->numInstances()));
		cullPass_->computeState()->setNumWorkUnits(static_cast<int>(cullShape_->numInstances()), 1, 1);
		cullPass_->computeState()->setGroupSize(RADIX_GROUP_SIZE, 1, 1);
		cullPass_->joinShaderInput(cullUBO_);
		cullPass_->joinShaderInput(frustumUBO_);
		cullPass_->joinShaderInput(lodGroupSizeBuffer_);
		cullPass_->joinShaderInput(radixSort_->keyBuffer());
		cullPass_->joinShaderInput(cullShape_->instanceIDBuffer());
		cullPass_->joinShaderInput(mesh_->getShapeBuffer());
		cullPass_->joinStates(cullShape_->tf());
		cullPass_->joinStates(camera_);
		auto boundingShape = mesh_->boundingShape();
		if (boundingShape->shapeType() == BoundingShapeType::SPHERE) {
			shaderCfg.define("SHAPE_TYPE", "SPHERE");
		} else if (boundingShape->shapeType() == BoundingShapeType::BOX) {
			auto box = (BoundingBox *) (boundingShape.get());
			if (box->isAABB()) {
				shaderCfg.define("SHAPE_TYPE", "AABB");
			} else {
				shaderCfg.define("SHAPE_TYPE", "OBB");
			}
		}
		shaderCfg.define("USE_CULLING", "TRUE");
		shaderCfg.addState(cullPass_.get());
		cullPass_->createShader(shaderCfg.cfg());
	}
}

void LODState::traverseGPU(RenderState *rs) {
	// clear the lodGroupSizeBuffer_ to zero's
	static uint32_t zero = 0;
	rs->shaderStorageBuffer().apply(lodGroupSizeBuffer_->blockReference()->bufferID());
	glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI,
						 lodGroupSizeBuffer_->blockReference()->address(),
						 lodGroupSizeBuffer_->blockReference()->allocatedSize(),
						 GL_RED_INTEGER,
						 GL_UNSIGNED_INT,
						 &zero);

	// Update the frustum planes in the UBO
	auto &frustumPlanes = camera_->frustum()[0].planes;
	for (int i = 0; i < 6; ++i) {
		frustumPlanes_[i] = frustumPlanes[i].equation();
	}
	rs->uniformBuffer().apply(frustumUBO_->blockReference()->bufferID());
	glBufferSubData(
			GL_UNIFORM_BUFFER,
			frustumUBO_->blockReference()->address(),
			frustumUBO_->blockReference()->allocatedSize(),
			&frustumPlanes_[0].x);

	// compute lod, write keys, and initialize values_[0] (instanceIDMap_)
	cullPass_->enable(rs);
	cullPass_->disable(rs);

	radixSort_->enable(rs);
	radixSort_->disable(rs);

	// Update and read lodGroupSize and update lodNumInstances
	lodGroupSizeMapping_->updateMapping(
			lodGroupSizeBuffer_->blockReference(),
			GL_SHADER_STORAGE_BUFFER);
	if (lodGroupSizeMapping_->hasData()) {
		auto &latestData = lodGroupSizeMapping_->storageValue();
		lodNumInstances_[0] = latestData.x;
		lodNumInstances_[1] = latestData.y;
		lodNumInstances_[2] = latestData.z;
		lodNumInstances_[3] = latestData.w;
	} else {
		return;
	}

	// loop over all LOD levels
	int32_t instanceIDOffset = 0;
	for (uint32_t lodLevel = 0; lodLevel < 4; ++lodLevel) {
		auto lodGroupSize = lodNumInstances_[lodLevel];
		if (lodGroupSize > 0) {
			updateVisibility(lodLevel, lodGroupSize, instanceIDOffset);
			instanceIDOffset += static_cast<int32_t>(lodGroupSize);
		}
	}
}
