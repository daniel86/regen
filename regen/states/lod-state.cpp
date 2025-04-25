#include <regen/states/state-node.h>
#include "lod-state.h"
#include "regen/meshes/mesh-vector.h"
#include "regen/gl-types/gl-param.h"
#include "regen/utility/conversion.h"

#define RADIX_BITS_PER_PASS 4u
// 4-bit radix sort --> 2^4 = 16 buckets
#define RADIX_NUM_BUCKETS 16
#define RADIX_GROUP_SIZE 256
#define RADIX_OFFSET_GROUP_SIZE 512
//#define RADIX_GLOBAL_HIERARCHICAL_SCAN
//#define RADIX_DEBUG_HISTOGRAM
//#define RADIX_DEBUG_RESULT
//#define RADIX_DEBUG_CORRECTNESS

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
	for (auto &m: meshVector_) {
		m->inputContainer()->set_numVisibleInstances(numVisible);
	}
	StateNode::traverse(rs);
	// reset number of visible instances
	for (auto &m: meshVector_) {
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
		if (mesh_->numLODs() <= 1) {
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
	uint32_t maxSharedMem = glParam<int>(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
	uint32_t maxWorkGroupInvocations = glParam<int>(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
	uint32_t numWorkGroups = 0u, histogramSize = 0u;

	{
		radixCull_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.cull");
		radixCull_->computeState()->shaderDefine("LOD_NUM_INSTANCES", REGEN_STRING(numInstances_));
		radixCull_->computeState()->setNumWorkUnits(static_cast<int>(numInstances_), 1, 1);
		radixCull_->computeState()->setGroupSize(RADIX_GROUP_SIZE, 1, 1);

		radixHistogramPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.histogram");
		radixHistogramPass_->computeState()->shaderDefine("LOD_NUM_INSTANCES", REGEN_STRING(numInstances_));
		radixHistogramPass_->computeState()->setNumWorkUnits(static_cast<int>(numInstances_), 1, 1);
		radixHistogramPass_->computeState()->setGroupSize(RADIX_GROUP_SIZE, 1, 1);
		numWorkGroups = radixHistogramPass_->computeState()->numWorkGroups().x;
		histogramSize = RADIX_NUM_BUCKETS * numWorkGroups;

		radixScatterPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.scatter");
		radixScatterPass_->computeState()->shaderDefine("LOD_NUM_INSTANCES", REGEN_STRING(numInstances_));
		radixScatterPass_->computeState()->setNumWorkUnits(static_cast<int>(numInstances_), 1, 1);
		radixScatterPass_->computeState()->setGroupSize(RADIX_GROUP_SIZE, 1, 1);
	}

	// Output: lodGroupSize
	lodGroupSizeBuffer_ = ref_ptr<SSBO>::alloc("LODGroupBuffer", BUFFER_USAGE_STREAM_COPY);
	lodGroupSize_ = ref_ptr<ShaderInput1ui>::alloc("lodGroupSize", 4);
	lodGroupSizeBuffer_->addBlockInput(lodGroupSize_);
	lodGroupSizeBuffer_->update();
	// +PBO for reading back the lodGroupSizeBuffer_
	lodGroupSizePBO_ = ref_ptr<PBO>::alloc(BUFFER_USAGE_STREAM_READ);
	lodGroupSizePBO_->bindPackBuffer();
	glBufferStorage(GL_PIXEL_PACK_BUFFER,
					sizeof(uint32_t) * 4, nullptr,
					GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	m_lodGroupSize_ = (Vec4ui *) glMapBufferRange(
			GL_PIXEL_PACK_BUFFER,
			0,
			sizeof(uint32_t) * 4,
			GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	// Temporary Buffers for sorting.
	keyBuffer_ = ref_ptr<SSBO>::alloc("KeyBuffer", BUFFER_USAGE_STREAM_COPY);
	keyBuffer_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("keys", numInstances_));
	keyBuffer_->update();

	valueBuffer_[0] = instanceIDBuffer_;
	valueBuffer_[1] = ref_ptr<SSBO>::alloc("ValueBuffer2", BUFFER_USAGE_STREAM_COPY);
	valueBuffer_[1]->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("values", numInstances_));
	valueBuffer_[1]->update();

	globalHistogramBuffer_ = ref_ptr<SSBO>::alloc("HistogramBuffer", BUFFER_USAGE_STREAM_COPY);
	globalHistogramBuffer_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc(
			"globalHistogram", RADIX_NUM_BUCKETS * numWorkGroups));
	globalHistogramBuffer_->update();

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
			shaderCfg.define("RADIX_REVERSE_SORT", "TRUE");
		}
		radixCull_->joinShaderInput(cullUBO_);
		radixCull_->joinShaderInput(frustumUBO_);
		radixCull_->joinShaderInput(lodGroupSizeBuffer_);
		radixCull_->joinShaderInput(keyBuffer_);
		radixCull_->joinShaderInput(instanceIDBuffer_);
		radixCull_->joinShaderInput(mesh_->getShapeBuffer());
		radixCull_->joinStates(tf_);
		radixCull_->joinStates(camera_);
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
		shaderCfg.addState(radixCull_.get());
		radixCull_->createShader(shaderCfg.cfg());
	}
	{ // radix histogram
		StateConfigurer shaderCfg;
		radixHistogramPass_->joinShaderInput(globalHistogramBuffer_);
		radixHistogramPass_->joinShaderInput(keyBuffer_);
		shaderCfg.addState(radixHistogramPass_.get());
		radixHistogramPass_->createShader(shaderCfg.cfg());
		// retrieve locations for quick state switching in radix passes
		histogramReadIndex_ = radixHistogramPass_->shaderState()->shader()->uniformLocation("ValueBuffer");
		histogramBitOffsetIndex_ = radixHistogramPass_->shaderState()->shader()->uniformLocation("radixBitOffset");
	}
	// radix offsets. We prefer here to do a single-pass parallel scan, if possible.
	// But we need to check if the histogram fits into shared memory and if the number of
	// work group invocations is not too high. Else we need to do a hierarchical scan.
	auto parallelScanInvocations = static_cast<int32_t>(math::nextPow2(histogramSize));
	int32_t parallelScanMemory = parallelScanInvocations * sizeof(uint32_t);
	bool useParallelScan = (
		parallelScanMemory <= maxSharedMem &&
		parallelScanInvocations <= maxWorkGroupInvocations);
#ifdef RADIX_SERIAL_GLOBAL_SCAN
	useParallelScan = true;
#endif

	if (useParallelScan) {
		StateConfigurer shaderCfg;
#ifdef RADIX_SERIAL_GLOBAL_SCAN
		radixGlobalOffsetsPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.offsets.serial");
		radixGlobalOffsetsPass_->computeState()->setNumWorkUnits(1, 1, 1);
		radixGlobalOffsetsPass_->computeState()->setGroupSize(1, 1, 1);
#else
		radixGlobalOffsetsPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.offsets.parallel");
		radixGlobalOffsetsPass_->computeState()->shaderDefine("RADIX_NUM_THREADS", REGEN_STRING(parallelScanInvocations));
		radixGlobalOffsetsPass_->computeState()->setNumWorkUnits(parallelScanInvocations, 1, 1);
		radixGlobalOffsetsPass_->computeState()->setGroupSize(parallelScanInvocations, 1, 1);
#endif
		radixGlobalOffsetsPass_->computeState()->shaderDefine("RADIX_HISTOGRAM_SIZE", REGEN_STRING(histogramSize));
		radixGlobalOffsetsPass_->computeState()->shaderDefine("RADIX_NUM_WORK_GROUPS", REGEN_STRING(numWorkGroups));
		radixGlobalOffsetsPass_->computeState()->shaderDefine("RADIX_NUM_BUCKETS", REGEN_STRING(RADIX_NUM_BUCKETS));
		radixGlobalOffsetsPass_->joinShaderInput(globalHistogramBuffer_);
		shaderCfg.addState(radixGlobalOffsetsPass_.get());
		radixGlobalOffsetsPass_->createShader(shaderCfg.cfg());
		radixOffsetsPass_ = radixGlobalOffsetsPass_;
	}
	else {
		// divide the histogram into "blocks" of RADIX_OFFSET_GROUP_SIZE
		int32_t numBlocks = ceil(static_cast<float>(histogramSize) / static_cast<float>(RADIX_OFFSET_GROUP_SIZE));
		// need to enforce power of two below
		auto numBlocks2 = static_cast<int32_t>(math::nextPow2(numBlocks));

		// create global memory for the offsets
		blockSumsBuffer_ = ref_ptr<SSBO>::alloc("BlockSumBuffer", BUFFER_USAGE_STREAM_COPY);
		blockSumsBuffer_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("blockSums", numBlocks));
		blockSumsBuffer_->blockInputs()[0].in_->set_forceArray(true);
		blockSumsBuffer_->update();

		blockOffsetsBuffer_ = ref_ptr<SSBO>::alloc("BlockOffsetsBuffer", BUFFER_USAGE_STREAM_COPY);
		blockOffsetsBuffer_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("blockOffsets", numBlocks));
		blockOffsetsBuffer_->blockInputs()[0].in_->set_forceArray(true);
		blockOffsetsBuffer_->update();

		{ // pass 1: local offsets
			radixLocaleOffsetsPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.offsets.local");
			radixLocaleOffsetsPass_->computeState()->setGroupSize(RADIX_OFFSET_GROUP_SIZE, 1, 1);
			radixLocaleOffsetsPass_->computeState()->setNumWorkUnits(numBlocks * RADIX_OFFSET_GROUP_SIZE, 1, 1);
			radixLocaleOffsetsPass_->computeState()->shaderDefine("RADIX_HISTOGRAM_SIZE", REGEN_STRING(histogramSize));
			radixLocaleOffsetsPass_->joinShaderInput(globalHistogramBuffer_);
			radixLocaleOffsetsPass_->joinShaderInput(blockSumsBuffer_);

			StateConfigurer shaderCfg;
			shaderCfg.addState(radixLocaleOffsetsPass_.get());
			radixLocaleOffsetsPass_->createShader(shaderCfg.cfg());
		}
		{ // pass 2: global offsets
			radixGlobalOffsetsPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.offsets.global");
			radixGlobalOffsetsPass_->computeState()->setGroupSize(numBlocks2, 1, 1);
			radixGlobalOffsetsPass_->computeState()->setNumWorkUnits(numBlocks2, 1, 1);
			radixGlobalOffsetsPass_->joinShaderInput(blockSumsBuffer_);
			radixGlobalOffsetsPass_->joinShaderInput(blockOffsetsBuffer_);

			StateConfigurer shaderCfg;
			shaderCfg.addState(radixGlobalOffsetsPass_.get());
			radixGlobalOffsetsPass_->createShader(shaderCfg.cfg());
		}
		{ // pass 3: distribute offsets
			radixDistributeOffsetsPass_ = ref_ptr<ComputePass>::alloc("regen.shapes.lod.radix.offsets.distribute");
			radixDistributeOffsetsPass_->computeState()->setGroupSize(RADIX_OFFSET_GROUP_SIZE, 1, 1);
			radixDistributeOffsetsPass_->computeState()->setNumWorkUnits(numBlocks * RADIX_OFFSET_GROUP_SIZE, 1, 1);
			radixDistributeOffsetsPass_->computeState()->shaderDefine("RADIX_HISTOGRAM_SIZE", REGEN_STRING(histogramSize));
			radixDistributeOffsetsPass_->joinShaderInput(globalHistogramBuffer_);
			radixDistributeOffsetsPass_->joinShaderInput(blockOffsetsBuffer_);

			StateConfigurer shaderCfg;
			shaderCfg.addState(radixDistributeOffsetsPass_.get());
			radixDistributeOffsetsPass_->createShader(shaderCfg.cfg());
		}

		radixOffsetsPass_ = ref_ptr<StateSequence>::alloc();
		radixOffsetsPass_->joinStates(radixLocaleOffsetsPass_);
		radixOffsetsPass_->joinStates(radixGlobalOffsetsPass_);
		radixOffsetsPass_->joinStates(radixDistributeOffsetsPass_);
	}
	{ // radix sort
		StateConfigurer shaderCfg;
		radixScatterPass_->joinShaderInput(globalHistogramBuffer_);
		radixScatterPass_->joinShaderInput(keyBuffer_);
		shaderCfg.addState(radixScatterPass_.get());
		radixScatterPass_->createShader(shaderCfg.cfg());
		// retrieve locations for quick state switching in radix passes
		scatterReadIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("ReadBuffer");
		scatterWriteIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("WriteBuffer");
		scatterBitOffsetIndex_ = radixScatterPass_->shaderState()->shader()->uniformLocation("radixBitOffset");
	}
}

void LODState::radixSortGPU(RenderState *rs) {
	// now we can make the radix passes starting with values_[0] as input
	// and writing to values_[1]. Then we swap the buffers each pass.
	// In the end, we will have the sorted instanceIDs in values_[0].
	uint32_t readIndex = 0u;
	uint32_t writeIndex = 1u;
	for (uint32_t bitOffset = 0u; bitOffset < 32u; bitOffset += RADIX_BITS_PER_PASS) {
		// Run histogram pass. As a result we will have global counts for each bucket
		// and work group in the globalHistogramBuffer_.
		radixHistogramPass_->enable(rs);
		glUniform1ui(histogramBitOffsetIndex_, bitOffset);
		valueBuffer_[readIndex]->bind(histogramReadIndex_);
		radixHistogramPass_->disable(rs);
#ifdef RADIX_DEBUG_HISTOGRAM
		printHistogram(rs);
#endif

		// Run offsets pass. As a result we will have the offsets for each work group
		// in the globalHistogramBuffer_.
		radixOffsetsPass_->enable(rs);
		radixOffsetsPass_->disable(rs);
#ifdef RADIX_DEBUG_HISTOGRAM
		printHistogram(rs);
#endif

		// Finally, run the scatter pass. As a result we will have the sorted instanceIDs
		// in the values_[writeIndex] buffer.
		radixScatterPass_->enable(rs);
		glUniform1ui(scatterBitOffsetIndex_, bitOffset);
		valueBuffer_[readIndex]->bind(scatterReadIndex_);
		valueBuffer_[writeIndex]->bind(scatterWriteIndex_);
		radixScatterPass_->disable(rs);

		// swap read and write buffers
		std::swap(readIndex, writeIndex);
	}
	GL_ERROR_LOG();

#ifdef RADIX_DEBUG_RESULT
	printInstanceMap(rs);
#elifdef RADIX_DEBUG_CORRECTNESS
	printInstanceMap(rs);
#endif
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

	// Update the frustum planes in the UBO
	// TODO: how to handle multi layer rendering with GPU LOD?
	auto &frustumPlanes = camera_->frustum()[0].planes;
	for (int i = 0; i < 6; ++i) {
		frustumPlanes_[i] = frustumPlanes[i].equation();
	}
	rs->copyWriteBuffer().push(frustumUBO_->blockReference()->bufferID());
	glBufferSubData(
			GL_COPY_WRITE_BUFFER,
			frustumUBO_->blockReference()->address(),
			frustumUBO_->blockReference()->allocatedSize(),
			&frustumPlanes_[0].x);
	rs->copyWriteBuffer().pop();

	// compute lod, write keys, and initialize values_[0] (instanceIDMap_)
	radixCull_->enable(rs);
	radixCull_->disable(rs);

	radixSortGPU(rs);

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
		lodNumInstances_[0] = m_lodGroupSize_[0].x;
		lodNumInstances_[1] = m_lodGroupSize_[0].y;
		lodNumInstances_[2] = m_lodGroupSize_[0].z;
		lodNumInstances_[3] = m_lodGroupSize_[0].w;
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

void LODState::printHistogram(RenderState *rs) {
	// debug histogram
	auto numWorkGroups = radixHistogramPass_->computeState()->numWorkGroups().x;
	auto numBuckets = RADIX_NUM_BUCKETS;
	rs->copyReadBuffer().push(globalHistogramBuffer_->blockReference()->bufferID());
	auto histogramData = (uint32_t *) glMapBufferRange(
			GL_COPY_READ_BUFFER,
			globalHistogramBuffer_->blockReference()->address(),
			globalHistogramBuffer_->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (histogramData) {
		std::stringstream sss;
		sss << "    histogram: | ";
		for (uint32_t i = 0; i < numBuckets * numWorkGroups; ++i) {
			sss << histogramData[i] << " ";
			if ((i + 1) % numBuckets == 0) {
				sss << " | ";
			}
		}
		REGEN_INFO(" " << sss.str());
		glUnmapBuffer(GL_COPY_READ_BUFFER);
	}
	rs->copyReadBuffer().pop();
}

void LODState::printInstanceMap(RenderState *rs) {
	// debug sorted output
	std::vector<double> distances(numInstances_);
	rs->copyReadBuffer().push(keyBuffer_->blockReference()->bufferID());
	auto sortKeys = (uint32_t *) glMapBufferRange(
			GL_COPY_READ_BUFFER,
			keyBuffer_->blockReference()->address(),
			keyBuffer_->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (sortKeys) {
		for (uint32_t i = 0; i < numInstances_; ++i) {
			distances[i] = conversion::uintToFloat(sortKeys[i]);
		}
		glUnmapBuffer(GL_COPY_READ_BUFFER);
	}
	rs->copyReadBuffer().pop();

	auto idRef = instanceIDBuffer_->blockReference();
	rs->copyReadBuffer().push(idRef->bufferID());
	auto instanceIDs = (uint32_t *) glMapBufferRange(
			GL_COPY_READ_BUFFER,
			idRef->address(),
			idRef->allocatedSize(),
			GL_MAP_READ_BIT);
	if (instanceIDs) {
		double lastDistance = 0.0;
		bool validSortedIDs_ = true;
		for (uint32_t i = 0; i < numInstances_; ++i) {
			auto mappedID = instanceIDs[i];
			if (mappedID >= numInstances_) {
				REGEN_ERROR("mappedID " << mappedID << " >= numInstances_ " << numInstances_);
				validSortedIDs_ = false;
				break;
			}
			auto distance = distances[mappedID];
			if (distance < lastDistance) {
				validSortedIDs_ = false;
			}
			lastDistance = distance;
		}
		if (validSortedIDs_) {
			REGEN_INFO("   sortedIDs are valid");
		} else {
			REGEN_INFO("   sortedIDs are INVALID");
		}
#ifdef RADIX_DEBUG_RESULT
		{
			std::stringstream sss;
			sss << "    ID data (" << numInstances_ << "): ";
			for (uint32_t i = 0; i < numInstances_; ++i) {
				if (instanceIDs[i] >= numInstances_) {
					break;
				}
				sss << instanceIDs[i] << " (" << distances[instanceIDs[i]] << ") ";
			}
			REGEN_INFO(" " << sss.str());
		}
#endif
		glUnmapBuffer(GL_COPY_READ_BUFFER);
	}
	rs->copyReadBuffer().pop();
}
