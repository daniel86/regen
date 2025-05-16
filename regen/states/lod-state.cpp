#include <regen/states/state-node.h>
#include "lod-state.h"
#include "regen/meshes/mesh-vector.h"
#include "regen/gl-types/gl-param.h"
#include "regen/utility/conversion.h"
#include "regen/camera/light-camera.h"

#define RADIX_BITS_PER_PASS 4u
#define RADIX_GROUP_SIZE 256
#define RADIX_OFFSET_GROUP_SIZE 512

using namespace regen;

LODState::LODState(
		const ref_ptr<Camera> &camera,
		const ref_ptr<SpatialIndex> &spatialIndex,
		std::string_view shapeName)
		: StateNode(),
		  camera_(camera),
		  spatialIndex_(spatialIndex) {
	hasShadowTarget_ = dynamic_cast<LightCamera*>(camera_.get()) != nullptr;
	shapeIndex_ = spatialIndex_->getIndexedShape(camera, shapeName);
	if (shapeIndex_.get()) {
		numInstances_ = shapeIndex_->shape()->numInstances();
		mesh_ = shapeIndex_->shape()->mesh();
		meshVector_.push_back(mesh_);
		for (auto &part: shapeIndex_->shape()->parts()) {
			meshVector_.push_back(part);
		}
	}
	if (!mesh_.get()) {
		numInstances_ = 1;
	}
	initLODState();
}

LODState::LODState(
		const ref_ptr<Camera> &camera,
		const std::vector<ref_ptr<Mesh>> &meshVector,
		const ref_ptr<ModelTransformation> &tf)
		: StateNode(),
		  camera_(camera),
		  meshVector_(meshVector),
		  tf_(tf) {
	hasShadowTarget_ = dynamic_cast<LightCamera*>(camera_.get()) != nullptr;
	mesh_ = meshVector.front();
	numInstances_ = tf->get()->numInstances();
	initLODState();
}

void LODState::initLODState() {
	lodNumInstances_.resize(4);
	lodGroups_.resize(4);
	// initially all instances are added to first LOD group
	lodNumInstances_[0] = numInstances_;
	for (uint32_t i = 1u; i < 4; ++i) {
		lodNumInstances_[i] = 0;
	}
}

void LODState::createBuffers() {
	if (numInstances_ <= 1) return;
	// Create array with numInstances_ elements.
	// The instance ids will be added each frame 1. in LOD-groups and 2. in view-dependent order
	instanceIDMap_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDMap", numInstances_);
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
	state()->joinShaderInput(instanceIDBuffer_);
	// In addition, we use an offset uniform, such that each LOD level can access the right
	// section of the instanceIDMap_.
	instanceIDOffset_ = createUniform<ShaderInput1i, int32_t>("instanceIDOffset", 0);
	state()->joinShaderInput(instanceIDOffset_);

	if (!spatialIndex_.get() && numInstances_ > 1) {
		createComputeShader();
	}
}

void LODState::updateMeshLOD() {
	if (!mesh_.get() || mesh_->numLODs() <= 1) {
		return;
	}
	auto &shape = shapeIndex_->shape();
	// set LOD level based on distance
	auto camPos = camera_->position()->getVertex(0);
	auto distance = (shape->getCenterPosition() - camPos.r).length();
	camPos.unmap();

	auto lodLevel = mesh_->getLODLevel(distance);
	// increase LOD level by one if we have a shadow target
	if (hasShadowTarget_ && lodLevel < mesh_->numLODs() - 1) {
		lodLevel++;
	}
	mesh_->activateLOD(lodLevel);
	for (auto &part: shape->parts()) {
		if (part->numLODs() > 1) {
			part->activateLOD(lodLevel);
		}
	}
}

static inline uint32_t getPartLOD(uint32_t lodLevel, uint32_t numPartLevels) {
	if (numPartLevels == 1) {
		return 0;
	}
	else if (numPartLevels == 2) {
		if (lodLevel < 2) {
			return 0;
		} else {
			return 1;
		}
	}
	else if (numPartLevels == 3) {
		if (lodLevel == 0) {
			return 0;
		} else if (lodLevel == 3) {
			return 2;
		} else {
			return 1;
		}
	}
	return lodLevel;
}

void LODState::activateLOD(uint32_t lodLevel) {
	// increase LOD level by one if we have a shadow target
	if (hasShadowTarget_ && lodLevel < mesh_->numLODs() - 1) {
		lodLevel++;
	}
	// set the LOD level
	for (auto &part: meshVector_) {
		// could be part has different number of LODs, need to compute an adjusted
		// LOD level for each part
		part->activateLOD(getPartLOD(lodLevel, part->numLODs()));
	}
}

void LODState::traverseInstanced_(RenderState *rs, uint32_t numVisible) {
	// set number of visible instances
	for (auto &m: meshVector_) {
		m->activeInputContainer()->set_numVisibleInstances(numVisible);
	}
	StateNode::traverse(rs);
	// reset number of visible instances
	for (auto &m: meshVector_) {
		m->activeInputContainer()->set_numVisibleInstances(numInstances_);
	}
}

void LODState::traverse(RenderState *rs) {
	if (spatialIndex_.get()) {
		traverseCPU(rs);
	} else {
		traverseGPU(rs);
	}
}

///////////////////////
//////////// CPU-based LOD update
///////////////////////

void LODState::traverseCPU(RenderState *rs) {
	if (!spatialIndex_->hasCamera(*camera_.get()) || !shapeIndex_.get()) {
		updateMeshLOD();
		StateNode::traverse(rs);
	} else if (numInstances_ <= 1) {
		if (shapeIndex_->isVisible()) {
			updateMeshLOD();
			StateNode::traverse(rs);
		}
	} else if (!mesh_.get()) {
		REGEN_WARN("No mesh set for shape " << shapeIndex_->shape()->name());
		numInstances_ = 0;
	} else {
		if (!shapeIndex_->hasVisibleInstances()) {
			// no visible instances
			return;
		}
		if (mesh_->numLODs() <= 1) {
			traverseInstanced_(rs, numInstances_);
		} else {
			// build LOD groups, then traverse each group
			computeLODGroups();

			int32_t instanceIDOffset = 0;
			for (uint32_t lodLevel = 0; lodLevel < lodNumInstances_.size(); ++lodLevel) {
				auto lodGroupSize = lodNumInstances_[lodLevel];
				if (lodGroupSize == 0) { continue; }
				// set the LOD level
				activateLOD(lodLevel);
				// set instanceIDOffset
				instanceIDOffset_->setVertex(0, instanceIDOffset);
				traverseInstanced_(rs, lodGroupSize);
				instanceIDOffset += static_cast<int32_t>(lodGroupSize);
			}
			// reset LOD level
			activateLOD(0);
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
	auto &transform = shapeIndex_->shape()->transform();
	auto &modelOffset = shapeIndex_->shape()->modelOffset();
	auto camPos = camera_->position()->getVertex(0);
	// clear LOD groups of last frame
	for (auto &lodGroup: lodGroups_) {
		lodGroup.clear();
	}

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
	auto instance_ids = instanceIDMap_->mapClientData<uint32_t>(ShaderData::WRITE);
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
		radixSort_ = ref_ptr<RadixSort>::alloc(numInstances_);
		radixSort_->setOutputBuffer(instanceIDBuffer_);
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
		cullPass_->computeState()->shaderDefine("LOD_NUM_INSTANCES", REGEN_STRING(numInstances_));
		cullPass_->computeState()->setNumWorkUnits(static_cast<int>(numInstances_), 1, 1);
		cullPass_->computeState()->setGroupSize(RADIX_GROUP_SIZE, 1, 1);
		cullPass_->joinShaderInput(cullUBO_);
		cullPass_->joinShaderInput(frustumUBO_);
		cullPass_->joinShaderInput(lodGroupSizeBuffer_);
		cullPass_->joinShaderInput(radixSort_->keyBuffer());
		cullPass_->joinShaderInput(instanceIDBuffer_);
		cullPass_->joinShaderInput(mesh_->getShapeBuffer());
		cullPass_->joinStates(tf_);
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
	//REGEN_INFO("LOD group sizes: (" << lodNumInstances_[0] << " " << lodNumInstances_[1] << " "
	//		<< lodNumInstances_[2] << " " << lodNumInstances_[3] << ")");

	// loop over all LOD levels
	int32_t instanceIDOffset = 0;
	for (uint32_t lodLevel = 0; lodLevel < 4; ++lodLevel) {
		auto lodGroupSize = lodNumInstances_[lodLevel];
		if (lodGroupSize == 0) {
			continue;
		}
		// set the LOD level
		activateLOD(lodLevel);
		// set instanceIDOffset
		instanceIDOffset_->setVertex(0, instanceIDOffset);
		traverseInstanced_(rs, lodGroupSize);
		instanceIDOffset += static_cast<int32_t>(lodGroupSize);
	}
	// reset LOD level
	activateLOD(0);
}
