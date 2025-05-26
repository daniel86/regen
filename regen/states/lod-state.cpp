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
	lodGroups_.resize(4);
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
	}
	else if (numPartLevels == 1) {
		return 0;
	}
	else if (numPartLevels < numBaseLevels || lodLevel >= numPartLevels) {
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
}

///////////////////////
//////////// CPU-based LOD update
///////////////////////

void LODState::traverseCPU(RenderState *rs) {
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
		}
		else {
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

void LODState::computeLODGroups() {
	auto visible_ids = shapeIndex_->mapInstanceIDs(ShaderData::READ);
	auto numVisible = visible_ids.r[0];
	if (numVisible == 0) { return; }

	if (instanceSortMode_ == SortMode::BACK_TO_FRONT) {
		computeLODGroups_(
				visible_ids.r + 1,
				static_cast<int>(numVisible) - 1,
				-1,
				-1);
	} else {
		computeLODGroups_(
				visible_ids.r + 1,
				0,
				static_cast<int>(numVisible),
				1);
	}
}

void LODState::computeLODGroups_(
		const uint32_t *mappedData, int begin, int end, int increment) {
	auto &transform = cullShape_->tf();
	auto &modelOffset = cullShape_->modelOffset();
	auto camPos = camera_->position()->getVertex(0);
	// clear LOD groups of last frame
	for (auto &lodGroup: lodGroups_) {
		lodGroup.clear();
	}

	// TODO: make this faster:
	// - remove lodGroups_ entirely, it is not needed.
	// - look for fast function to cop in reverse order, else it should be fine to copy data as is.
	// - then we could try optimizing the counting of instances per LOD level.
	//   in case we have some instance, let's say >1000, we could use a multithreaded approach
	//   with e.g. group size of 256 and then use a parallel reduction to count the instances.
	if (mesh_->numLODs() == 1) {
		for (int i = begin; i != end; i += increment) {
			lodGroups_[0].push_back(mappedData[i]);
		}
	} else if (transform.get() && modelOffset.get()) {
		auto modelOffsetData = modelOffset->mapClientData<Vec3f>(ShaderData::READ);
		auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
		for (int i = begin; i != end; i += increment) {
			auto lodLevel = mesh_->getLODLevel((
													   tfData.r[mappedData[i]].position() +
													   modelOffsetData.r[mappedData[i]] - camPos.r).length());
			lodGroups_[lodLevel].push_back(mappedData[i]);
		}
	} else if (modelOffset.get()) {
		auto modelOffsetData = modelOffset->mapClientData<Vec3f>(ShaderData::READ);
		for (int i = begin; i != end; i += increment) {
			auto lodLevel = mesh_->getLODLevel((
													   modelOffsetData.r[mappedData[i]] - camPos.r).length());
			lodGroups_[lodLevel].push_back(mappedData[i]);
		}
	} else if (transform.get()) {
		auto tfData = transform->get()->mapClientData<Mat4f>(ShaderData::READ);
		for (int i = begin; i != end; i += increment) {
			auto lodLevel = mesh_->getLODLevel((
													   tfData.r[mappedData[i]].position() - camPos.r).length());
			lodGroups_[lodLevel].push_back(mappedData[i]);
		}
	} else {
		for (int i = begin; i != end; i += increment) {
			lodGroups_[0].push_back(mappedData[i]);
		}
	}

	// write lodGroups_ data into instanceIDMap_
	auto &instanceIDMap = cullShape_->instanceIDMap();
	auto instance_ids = instanceIDMap->mapClientData<uint32_t>(ShaderData::WRITE);
	uint32_t numVisible = 0u;
	for (size_t i = 0; i < lodGroups_.size(); ++i) {
		auto &lodGroup = lodGroups_[i];
		lodNumInstances_[i] = static_cast<uint32_t>(lodGroup.size());
		if (!lodGroup.empty()) {
			std::memcpy(instance_ids.w + numVisible, lodGroup.data(), lodNumInstances_[i] * sizeof(uint32_t));
			numVisible += lodNumInstances_[i];
		}
	}
	instance_ids.unmap();
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
