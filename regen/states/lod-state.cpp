#include <regen/states/state-node.h>
#include "lod-state.h"
#include "regen/meshes/mesh-vector.h"

using namespace regen;

LODState::LODState(
		const ref_ptr<Camera> &camera,
		const ref_ptr<SpatialIndex> &spatialIndex,
		std::string_view shapeName)
		: StateNode(),
		  camera_(camera),
		  spatialIndex_(spatialIndex) {
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
	createInstanceBuffer();
}

LODState::LODState(
		const ref_ptr<Camera> &camera,
		const std::vector<ref_ptr<Mesh>> &meshVector,
		const ref_ptr<ModelTransformation> &tf)
		: StateNode(),
		  camera_(camera),
		  meshVector_(meshVector),
		  tf_(tf) {
	mesh_ = meshVector.front();
	numInstances_ = tf->get()->numInstances();
	createInstanceBuffer();
}

void LODState::createInstanceBuffer() {
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
	instanceIDBuffer_ = ref_ptr<SSBO>::alloc("InstanceIDs", BufferUsage::USAGE_DYNAMIC);
	instanceIDBuffer_->addBlockInput(instanceIDMap_);
	instanceIDBuffer_->update();
	state()->joinShaderInput(instanceIDBuffer_);
	// In addition, we use an offset uniform, such that each LOD level can access the right
	// section of the instanceIDMap_.
	instanceIDOffset_ = createUniform<ShaderInput1i, int32_t>("instanceIDOffset", 0);
	state()->joinShaderInput(instanceIDOffset_);
	lodNumInstances_.resize(mesh_->numLODs());
	lodGroups_.resize(mesh_->numLODs());
	// initially all instances are added to first LOD group
	lodNumInstances_[0] = numInstances_;
	for (uint32_t i = 1u; i < mesh_->numLODs(); ++i) {
		lodNumInstances_[i] = 0;
	}

	// create LOD thresholds
	auto far = camera_->far()->getVertex(0).r;
	lodThresholds_ = ref_ptr<ShaderInput3f>::alloc("lodThresholds");
	lodThresholds_->setUniformData(Vec3f::zero());
	setThresholds(Vec3f(0.2f*far, 0.6f*far, 0.8f*far));

	if (!spatialIndex_.get()) {
		createComputeShader();
	}
}

void LODState::setThresholds(const Vec3f &thresholds) {
	if(mesh_->numLODs()==4) {
		lodThresholds_->setVertex(0, thresholds);
	}
	else if(mesh_->numLODs()==3) {
		lodThresholds_->setVertex(0, Vec3f(0.0f, thresholds.x, thresholds.y));
	}
	else if(mesh_->numLODs()==2) {
		lodThresholds_->setVertex(0, Vec3f(0.0f, 0.0f, thresholds.x));
	}
	else {
		lodThresholds_->setVertex(0, Vec3f(0.0f, 0.0f, 0.0f));
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
	mesh_->updateLOD(distance);
	for (auto &part: shape->parts()) {
		if (part->numLODs() > 1) {
			part->updateLOD(distance);
		}
	}
}

void LODState::activateLOD(uint32_t lodLevel) {
	// set the LOD level
	for (auto &part: meshVector_) {
		if (mesh_->numLODs() == part->numLODs() && part->numLODs() > 1) {
			part->activateLOD(lodLevel);
		} else if (part->numLODs() > 1) {
			// could be part has different number of LODs, need to compute an adjusted
			// LOD level for each part
			part->activateLOD(static_cast<uint32_t>(std::round(static_cast<float>(lodLevel) *
															   static_cast<float>(part->numLODs()) /
															   static_cast<float>(mesh_->numLODs()))));
		}
	}
}

void LODState::traverseInstanced_(RenderState *rs, uint32_t numVisible) {
	// set number of visible instances
	for (auto &m : meshVector_) {
		m->inputContainer()->set_numVisibleInstances(numVisible);
	}
	StateNode::traverse(rs);
	// reset number of visible instances
	for (auto &m : meshVector_) {
		m->inputContainer()->set_numVisibleInstances(numInstances_);
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
		if (lodNumInstances_.size() < 2) {
			traverseInstanced_(rs, numInstances_);
		} else {
			// build LOD groups, then traverse each group
			computeLODGroups();

			int32_t instanceIDOffset = 0;
			for (uint32_t lodLevel = 0; lodLevel < mesh_->numLODs(); ++lodLevel) {
				auto lodGroupSize = lodNumInstances_[lodLevel];
				if (lodGroupSize == 0) { continue; }
				// set the LOD level
				activateLOD(lodLevel);
				// set instanceIDOffset
				instanceIDOffset_->setVertex(0, instanceIDOffset);
				traverseInstanced_(rs, lodGroupSize);
				instanceIDOffset += lodGroupSize;
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

	if (lodNumInstances_.size() == 1) {
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
	computeLODPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.compute");
	computeLODPass_->computeState()->setNumWorkUnits(static_cast<int>(numInstances_), 1, 1);
	computeLODPass_->computeState()->setGroupSize(256, 1, 1);

	// Output (1): instanceIDMap
	computeLODPass_->joinShaderInput(instanceIDBuffer_);
	// Output (2): lodGroupSize
	lodGroupSizeBuffer_ = ref_ptr<SSBO>::alloc("LODGroupBuffer", USAGE_DYNAMIC);
	lodGroupSize_ = ref_ptr<ShaderInput1ui>::alloc("lodGroupSize", 4);
	lodGroupSizeBuffer_->addBlockInput(lodGroupSize_);
	lodGroupSizeBuffer_->update();
	computeLODPass_->joinShaderInput(lodGroupSizeBuffer_);
	// +PBO for reading back the lodGroupSizeBuffer_
	lodGroupSizePBO_ = ref_ptr<PBO>::alloc(USAGE_STREAM);
	lodGroupSizePBO_->bindPackBuffer();
	glBufferStorage(GL_PIXEL_PACK_BUFFER,
		sizeof(uint32_t)*4, nullptr,
		GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	m_lodGroupSize_ = (Vec4ui*)glMapBufferRange(
		GL_PIXEL_PACK_BUFFER,
		0,
		sizeof(uint32_t)*4,
		GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	// Temporary Buffers for sorting.
	auto numWorkGroups = computeLODPass_->computeState()->numWorkGroups().x;
	sortBuffer1_ = ref_ptr<SSBO>::alloc("SortBuffer1", USAGE_DYNAMIC);
	sortBuffer1_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("sortKeys", numInstances_));
	sortBuffer1_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("sortedIDsTemp", numInstances_));
	sortBuffer1_->update();
	computeLODPass_->joinShaderInput(sortBuffer1_);
	sortBuffer2_ = ref_ptr<SSBO>::alloc("SortBuffer2", USAGE_DYNAMIC);
	sortBuffer2_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("workGroupSize", numWorkGroups));
	sortBuffer2_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("workGroupOffset", numWorkGroups));
	sortBuffer2_->update();
	computeLODPass_->joinShaderInput(sortBuffer2_);

	// Position input buffer
	computeLODPass_->joinStates(tf_);

	// Uniform parameters
	computeLODPass_->joinShaderInput(lodThresholds_);
	computeLODPass_->joinStates(camera_);
	//computeLODPass_->joinShaderInput(camera_->cameraBlock());

	StateConfigurer shaderConfigurer;
	shaderConfigurer.addState(computeLODPass_.get());
	computeLODPass_->createShader(shaderConfigurer.cfg());
}

void LODState::traverseGPU(RenderState *rs) {
	// clear the lodGroupSizeBuffer_ to zero's
	static uint32_t zero = 0;
	rs->copyWriteBuffer().push(lodGroupSizeBuffer_->blockReference()->bufferID());
	glClearBufferSubData(GL_COPY_WRITE_BUFFER, GL_R32UI,
		lodGroupSizeBuffer_->blockReference()->address(),
		lodGroupSizeBuffer_->blockReference()->allocatedSize(),
		GL_RED_INTEGER,
		GL_UNSIGNED_INT,
		&zero);
	rs->copyWriteBuffer().pop();

	// update the index buffer
	computeLODPass_->enable(rs);
	computeLODPass_->disable(rs);

	// Copy lodGroupSizeBuffer_ to lodGroupSizePBO_
	rs->copyReadBuffer().push(lodGroupSizeBuffer_->blockReference()->bufferID());
	rs->copyWriteBuffer().push(lodGroupSizePBO_->id());
	glCopyBufferSubData(
			GL_COPY_READ_BUFFER,
			GL_COPY_WRITE_BUFFER,
			lodGroupSizeBuffer_->blockReference()->address(),
			0,
			lodGroupSizeBuffer_->blockReference()->allocatedSize());
	rs->copyWriteBuffer().pop();
	rs->copyReadBuffer().pop();

	// Read lodGroupSizePBO_ and update lodNumInstances_
    if (m_lodGroupSize_) {
    	if (mesh_->numLODs() == 4) {
			lodNumInstances_[0] = m_lodGroupSize_[0].x;
			lodNumInstances_[1] = m_lodGroupSize_[0].y;
			lodNumInstances_[2] = m_lodGroupSize_[0].z;
			lodNumInstances_[3] = m_lodGroupSize_[0].w;
    	} else if (mesh_->numLODs() == 3) {
			lodNumInstances_[0] = m_lodGroupSize_[0].y;
			lodNumInstances_[1] = m_lodGroupSize_[0].z;
			lodNumInstances_[2] = m_lodGroupSize_[0].w;
			lodNumInstances_[3] = 0;
		} else if (mesh_->numLODs() == 2) {
			lodNumInstances_[0] = m_lodGroupSize_[0].z;
			lodNumInstances_[1] = m_lodGroupSize_[0].w;
			lodNumInstances_[2] = 0;
			lodNumInstances_[3] = 0;
		} else {
			lodNumInstances_[0] = m_lodGroupSize_[0].w;
			lodNumInstances_[1] = 0;
			lodNumInstances_[2] = 0;
			lodNumInstances_[3] = 0;
		}
    }

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
		instanceIDOffset += lodGroupSize;
	}
	// reset LOD level
	activateLOD(0);
}
