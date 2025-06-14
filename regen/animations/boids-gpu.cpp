#include "boids-gpu.h"
#include "regen/states/radix-sort.h"

using namespace regen;

#define BOID_USE_HALF_VELOCITY
#define BOID_USE_SORTED_DATA
//#define BOID_DEBUG_GRID_OFFSETS
//#define BOID_DEBUG_GRID_SORTING
//#define BOID_DEBUG_BBOX_TIME
//#define BOID_DEBUG_GRID_TIME
//#define BOID_DEBUG_SIMULATION_TIME
#if defined(BOID_DEBUG_SIMULATION_TIME) || defined(BOID_DEBUG_GRID_TIME)
#define BOID_DEBUG_TIME
#endif

BoidsGPU::BoidsGPU(const ref_ptr<ModelTransformation> &tf)
		: BoidSimulation(tf),
		  Animation(true, false) {
}

void BoidsGPU::initBoidSimulation() {
	// TODO: support attractors and dangers.
	//         - join their transform input
	//         - then use define some macros to generate the shader code
	setAnimationName("boids");
	createResource();
	REGEN_INFO("GPU Boids simulation with " << numBoids_ << " boids");
	GL_ERROR_LOG();
}

void BoidsGPU::computeBBox(const Vec3f *posData) {
	boidBounds_.min = posData[0];
	boidBounds_.max = posData[0];
	for (uint32_t i = 1u; i < numBoids_; ++i) {
		boidBounds_.min.setMin(posData[i]);
		boidBounds_.max.setMax(posData[i]);
	}
}

void BoidsGPU::createShader(const ref_ptr<ComputePass> &pass, const ref_ptr<State> &update) {
	StateConfigurer shaderConfigurer;
	shaderConfigurer.define("NUM_BOIDS", REGEN_STRING(numBoids_));
	//shaderConfigurer.define("NUM_GRID_CELLS", REGEN_STRING(numCells_));
	shaderConfigurer.addState(animationState().get());
	shaderConfigurer.addState(update.get());
	pass->createShader(shaderConfigurer.cfg());
}

void BoidsGPU::createResource() {
	std::vector<Vec3f> initialPositions(numBoids_);
#ifdef BOID_USE_HALF_VELOCITY
	std::vector<Vec2ui> initialVelocities(numBoids_);
#else
	std::vector<Vec3f> initialVelocities(numBoids_);
#endif
	if(tf_->hasModelMat()) {
		auto tfData = tf_->modelMat()->mapClientData<Mat4f>(ShaderData::READ);
		for (uint32_t i = 0; i < numBoids_; ++i) {
			initialPositions[i] = tfData.r[i].position();
#ifdef BOID_USE_HALF_VELOCITY
			initialVelocities[i] = Vec2ui::zero();
#else
			initialVelocities[i] = Vec3f::zero();
#endif
		}
	}
	else if(tf_->hasModelOffset()) {
		auto initialPositionData = tf_->modelOffset()->mapClientData<Vec4f>(ShaderData::READ);
		for (uint32_t i = 0; i < numBoids_; ++i) {
			initialPositions[i] = initialPositionData.r[i].xyz_();
#ifdef BOID_USE_HALF_VELOCITY
			initialVelocities[i] = Vec2ui::zero();
#else
			initialVelocities[i] = Vec3f::zero();
#endif
		}
	}
	// compute initial bbox
	computeBBox(initialPositions.data());
	updateGridSize();
#ifdef BOID_DEBUG_TIME
	timeElapsedQuery_ = ref_ptr<TimeElapsedQuery>::alloc();
#endif
	u_numCells_ = ref_ptr<ShaderInput1ui>::alloc("numGridCells");
	u_numCells_->setUniformData(numCells_);

	{ // UBO with grid parameters
		gridUBO_ = ref_ptr<UBO>::alloc("BoidGrid");
		gridMin_ = ref_ptr<ShaderInput3f>::alloc("gridMin");
		gridMin_->setUniformData(gridBounds_.min);
		gridUBO_->addBlockInput(gridMin_);
		gridUBO_->addBlockInput(cellSize_);
		gridUBO_->addBlockInput(gridSize_);
	}

	// SSBO for position, one per boid
	if (tf_.get()) {
		// bind UBO as SSBO for writing model matrix
		auto bufferContainer = tf_->bufferContainer();
		bufferContainer->updateBuffer();
		auto bufferObject = bufferContainer->getBufferObject(tf_->modelMat());
		auto ssbo = ref_ptr<SSBO>::dynamicCast(bufferObject);
		if (ssbo.get()) {
			tfBuffer_ = ssbo;
		} else {
			tfBuffer_ = ref_ptr<SSBO>::alloc(*bufferObject.get());
		}
	}

	// SSBO for velocity, one per boid
	velBuffer_ = ref_ptr<SSBO>::alloc("VelocityBlock",
			BUFFER_USAGE_DYNAMIC_DRAW, SSBO::RESTRICT);
#ifdef BOID_USE_HALF_VELOCITY
	auto vel = ref_ptr<ShaderInput2ui>::alloc("vel", numBoids_);
#else
	auto vel = ref_ptr<ShaderInput1f>::alloc("vel", numBoids_ * 3);
#endif
	vel->setInstanceData(1, 1, (byte*)initialVelocities.data());
	velBuffer_->addBlockInput(vel);
	velBuffer_->update();

	// bounding box SSBO as we read back bounding box to CPU
	bboxBuffer_ = ref_ptr<BBoxBuffer>::alloc();
	{
		bboxPass_ = ref_ptr<ComputePass>::alloc("regen.compute.bbox");
		bboxPass_->computeState()->setNumWorkUnits(numBoids_, 1, 1);
		bboxPass_->computeState()->setGroupSize(simulationGroupSize_, 1, 1);
		if (tf_.get()) {
			bboxPass_->joinShaderInput(tfBuffer_);
		}
		bboxPass_->joinShaderInput(bboxBuffer_);
		StateConfigurer shaderConfigurer;
		shaderConfigurer.define("NUM_ELEMENTS", REGEN_STRING(numBoids_));
		shaderConfigurer.addState(bboxPass_.get());
		bboxPass_->createShader(shaderConfigurer.cfg());
	}
	{ // SSBO for grid offsets
		gridOffsetBuffer_ = ref_ptr<SSBO>::alloc("GridOffsets",
				BUFFER_USAGE_DYNAMIC_DRAW, SSBO::RESTRICT);
		gridOffsetBuffer_->addBlockInput(ref_ptr<ShaderInput1ui>::alloc("globalHistogram", numCells_ + 1));
		gridOffsetBuffer_->update();
	}

	if (tf_.get()) {
		animationState()->joinShaderInput(tfBuffer_);
	}
	// create a state that updates the boids grid
	updateGridState_ = ref_ptr<StateSequence>::alloc();
	#ifdef BOID_USE_SORTED_DATA
	{
		boidDataBuffer_ = ref_ptr<SSBO>::alloc("BoidDataBuffer",
				BUFFER_USAGE_DYNAMIC_DRAW, SSBO::RESTRICT);
		boidDataBuffer_->addBlockInput(ref_ptr<ShaderInputStruct<BoidData>>::alloc("BoidData", "boidData", numBoids_));
		boidDataBuffer_->update();
	}
	#endif
	{
		auto radixSort = ref_ptr<RadixSort>::alloc(numBoids_);
		radixSort->setRadixBits(radixBits_);
		radixSort->setSortGroupSize(sortGroupSize_);
		radixSort->setScanGroupSize(scanGroupSize_);
		radixSort->createResources();
		radixSort_ = radixSort;
	}
	// 1. Fill the key buffer with the grid cell index for each boid,
	//    and reset the grid offset buffer to all zero.
	{
		auto updateState = ref_ptr<State>::alloc();
		updateState->joinShaderInput(gridUBO_);
		updateState->joinShaderInput(((RadixSort*)radixSort_.get())->keyBuffer());
		updateState->joinShaderInput(((RadixSort*)radixSort_.get())->inputIndexBuffer());
		updateState->joinShaderInput(gridOffsetBuffer_);
		updateState->joinShaderInput(u_numCells_);
		gridResetPass_ = ref_ptr<ComputePass>::alloc("regen.animation.boid.grid.reset");
		gridResetPass_->computeState()->setNumWorkUnits(std::max(numBoids_, numCells_+1), 1, 1);
		gridResetPass_->computeState()->setGroupSize(simulationGroupSize_, 1, 1);
		updateState->joinStates(gridResetPass_);
		createShader(gridResetPass_, updateState);
		updateGridState_->joinStates(updateState);
	}
	// 2. Perform radix sort on the key buffer, creating a sorted index array
	updateGridState_->joinStates(radixSort_);
	// 3. Compute offsets of grid cells over sorted indices.
	//    These offsets can be used for accessing boids at a given index in the grid.
	//    Optionally write sorted data into a back buffer for better memory access pattern
	//    when reading boids data in the simulation shader.
	{
		auto updateState = ref_ptr<State>::alloc();
		updateState->joinShaderInput(gridUBO_);
		updateState->joinShaderInput(((RadixSort*)radixSort_.get())->keyBuffer());
		updateState->joinShaderInput(((RadixSort*)radixSort_.get())->sortedIndexBuffer());
		updateState->joinShaderInput(gridOffsetBuffer_);
		#ifdef BOID_USE_SORTED_DATA
		updateState->joinShaderInput(velBuffer_);
		if (tf_.get()) {
			updateState->joinShaderInput(tfBuffer_);
		}
		updateState->joinShaderInput(boidDataBuffer_);
		updateState->shaderDefine("USE_SORTED_DATA", "TRUE");
		#endif
		#ifdef BOID_USE_HALF_VELOCITY
		updateState->shaderDefine("USE_HALF_VELOCITY", "TRUE");
		#endif
		auto computeNode = ref_ptr<ComputePass>::alloc("regen.animation.boid.grid.offsets");
		computeNode->computeState()->setNumWorkUnits(numBoids_, 1, 1);
		computeNode->computeState()->setGroupSize(simulationGroupSize_, 1, 1);
		updateState->joinStates(computeNode);
		createShader(computeNode, updateState);
		updateGridState_->joinStates(updateState);
	}

	// create a state that updates the boids positions and velocities
	simulationState_ = ref_ptr<State>::alloc();
#ifdef BOID_USE_HALF_VELOCITY
	simulationState_->shaderDefine("USE_HALF_VELOCITY", "TRUE");
#endif
	{ // simulation parameters
		simulationState_->joinShaderInput(simulationBoundsMin_);
		simulationState_->joinShaderInput(visualRange_);
		simulationState_->joinShaderInput(simulationBoundsMax_);
		simulationState_->joinShaderInput(maxBoidSpeed_);
		simulationState_->joinShaderInput(boidsScale_);
		simulationState_->joinShaderInput(baseOrientation_);
		simulationState_->joinShaderInput(maxAngularSpeed_);
		simulationState_->joinShaderInput(coherenceWeight_);
		simulationState_->joinShaderInput(alignmentWeight_);
		simulationState_->joinShaderInput(separationWeight_);
		simulationState_->joinShaderInput(avoidanceWeight_);
		simulationState_->joinShaderInput(avoidanceDistance_);
		simulationState_->joinShaderInput(lookAheadDistance_);
		simulationState_->joinShaderInput(repulsionFactor_);
		simulationState_->joinShaderInput(maxNumNeighbors_);
	}
	simulationState_->joinShaderInput(gridUBO_);
	simulationState_->joinShaderInput(velBuffer_);
	simulationState_->joinShaderInput(gridOffsetBuffer_);
	simulationState_->joinShaderInput(((RadixSort*)radixSort_.get())->sortedIndexBuffer());
	simulationState_->joinShaderInput(bboxBuffer_);
	if (tf_.get()) {
		simulationState_->joinShaderInput(tfBuffer_);
	}
#ifdef BOID_USE_SORTED_DATA
	simulationState_->joinShaderInput(boidDataBuffer_);
#endif
	if (heightMap_.get()) {
		simulationState_->joinShaderInput(
			createUniform<ShaderInput3f,Vec3f>("mapCenter", mapCenter_));
		simulationState_->joinShaderInput(
			createUniform<ShaderInput1f,float>("heightMapFactor", heightMapFactor_));
		simulationState_->joinShaderInput(
			createUniform<ShaderInput2f,Vec2f>("mapSize", mapSize_));
		simulationState_->joinStates(
			ref_ptr<TextureState>::alloc(heightMap_, "heightMap"));
	}
	if (homePoints_.empty()) {
		simulationState_->shaderDefine("NUM_BOID_HOMES", "0");
	} else {
		simulationState_->shaderDefine("NUM_BOID_HOMES", std::to_string(homePoints_.size()));
		for (uint64_t i = 0u; i < homePoints_.size(); ++i) {
			simulationState_->shaderDefine(
				REGEN_STRING("BOID_HOME" << i),
				REGEN_STRING("vec3(" <<
					homePoints_[i].x << ", " <<
					homePoints_[i].y << ", " <<
					homePoints_[i].z << ")"));
		}
	}
	auto simulationCompute = ref_ptr<ComputePass>::alloc("regen.animation.boid.simulate");
	simulationCompute->computeState()->setNumWorkUnits(numBoids_, 1, 1);
	simulationCompute->computeState()->setGroupSize(simulationGroupSize_, 1, 1);
	simulationState_->joinStates(simulationCompute);
	createShader(simulationCompute, simulationState_);
	simulationTimeLoc_ = simulationCompute->shaderState()->shader()->uniformLocation("boidTimeDelta");
}

ref_ptr<BoidsGPU> BoidsGPU::load(
			LoadingContext &ctx,
			scene::SceneInputNode &input,
			const ref_ptr<ModelTransformation> &tf) {
	auto boids = ref_ptr<BoidsGPU>::alloc(tf);
	boids->loadSettings(ctx, input);
	return boids;
}

void BoidsGPU::glAnimate(RenderState *rs, GLdouble dt) {
	// limit FPS to 60
	time_ += dt;
	bbox_time_ += dt;
	if (time_ < 16.0) { return; }

	// limit FPS to 6, grid must not be entirely accurate.
	// usually the grid size changes only every second or so.
	if (bbox_time_ > 166.0) {
#ifdef BOID_DEBUG_BBOX_TIME
		timeElapsedQuery_->begin();
#endif
		bboxBuffer_->clear();
		bboxPass_->enable(rs);
		bboxPass_->disable(rs);
		// update the grid in case the bounding box around the boids changed.
		if(bboxBuffer_->updateBoundingBox() || vrStamp_ != cellSize_->stamp()) {
			auto &newBounds = bboxBuffer_->bbox();
			if (newBounds.max != newBounds.min) {
				boidBounds_ = newBounds;
				updateGrid();
				vrStamp_ = visualRange_->stamp();
			}
		}
#ifdef BOID_DEBUG_BBOX_TIME
		REGEN_INFO("BBox computation took: " << timeElapsedQuery_->end() << " ms");
#endif
		bbox_time_ = 0.0;
	}

	if (numCells_ > 0) {
#ifdef BOID_DEBUG_GRID_TIME
		timeElapsedQuery_->begin();
#endif
		updateGridState_->enable(rs);
		updateGridState_->disable(rs);
#ifdef BOID_DEBUG_GRID_TIME
		REGEN_INFO("Grid update took: " << timeElapsedQuery_->end() << " ms");
#endif
#ifdef BOID_DEBUG_GRID_OFFSETS
		printOffsets(rs);
#endif
#ifdef BOID_DEBUG_GRID_SORTING
		debugGridSorting(rs);
#endif
#ifdef BOID_DEBUG_SIMULATION_TIME
		timeElapsedQuery_->begin();
#endif
		// update the boids positions and velocities
		simulate(rs, time_);
#ifdef BOID_DEBUG_SIMULATION_TIME
		REGEN_INFO("Boid simulation took: " << timeElapsedQuery_->end() << " ms");
#endif
	}
	time_ = 0.0;
	GL_ERROR_LOG();
}

void BoidsGPU::updateGrid() {
	auto lastNumCells = numCells_;
	updateGridSize();
	gridMin_->setVertex(0, gridBounds_.min);
	if (lastNumCells != numCells_) {
		// acquire buffer space of the right size
		gridResetPass_->computeState()->setNumWorkUnits(std::max(numBoids_, numCells_ + 1), 1, 1);
		gridOffsetBuffer_->blockInputs().front().in_->set_numArrayElements(static_cast<int>(numCells_ + 1u));
		gridOffsetBuffer_->update(true);
		u_numCells_->setVertex(0, numCells_);
		REGEN_DEBUG("Boid grid size changed to " << numCells_ << " cells.");
	}
}

void BoidsGPU::simulate(RenderState *rs, double boidTimeDelta) {
	simulationState_->enable(rs);
	glUniform1f(simulationTimeLoc_, static_cast<float>(boidTimeDelta*0.001));
	simulationState_->disable(rs);
}

void BoidsGPU::printOffsets(RenderState *rs) {
	rs->shaderStorageBuffer().apply(gridOffsetBuffer_->blockReference()->bufferID());
	auto offsetData = (uint32_t *) glMapBufferRange(
			GL_SHADER_STORAGE_BUFFER,
			gridOffsetBuffer_->blockReference()->address(),
			gridOffsetBuffer_->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (offsetData) {
		std::stringstream sss;
		sss << "    offsets: | ";
		for (uint32_t i = 0; i < numCells_+1; ++i) {
			sss << offsetData[i] << " ";
		}
		REGEN_INFO(" " << sss.str());
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}
}

void BoidsGPU::debugVelocity(RenderState *rs) {
#ifndef BOID_USE_HALF_VELOCITY
	rs->shaderStorageBuffer().apply(velBuffer_->blockReference()->bufferID());
	auto velData = (Vec3f *) glMapBufferRange(
			GL_SHADER_STORAGE_BUFFER,
			velBuffer_->blockReference()->address(),
			velBuffer_->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (velData) {
		std::stringstream sss;
		sss << "    velocities: | ";
		for (uint32_t i = 0; i < numBoids_; ++i) {
			sss << velData[i] << " ";
		}
		REGEN_INFO(" " << sss.str());
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}
#endif
}

void BoidsGPU::debugGridSorting(RenderState *rs) {
	// read the key buffer
	std::vector<uint32_t> sortKeys(numBoids_);
	rs->shaderStorageBuffer().apply(((RadixSort*)radixSort_.get())->keyBuffer()->blockReference()->bufferID());
	auto sortKeysData = (uint32_t *) glMapBufferRange(
			GL_SHADER_STORAGE_BUFFER,
			((RadixSort*)radixSort_.get())->keyBuffer()->blockReference()->address(),
			((RadixSort*)radixSort_.get())->keyBuffer()->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (sortKeysData) {
		std::set<uint32_t> usedCells;
		for (uint32_t i = 0; i < numBoids_; ++i) {
			sortKeys[i] = sortKeysData[i];
			usedCells.insert(sortKeys[i]);
		}
		REGEN_INFO("Number of occupied cells: " << usedCells.size());
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	// second map the index buffer and test if the indices are sorted correctly
	rs->shaderStorageBuffer().apply(((RadixSort*)radixSort_.get())->sortedIndexBuffer()->blockReference()->bufferID());
	auto sortedIndexData = (uint32_t *) glMapBufferRange(
			GL_SHADER_STORAGE_BUFFER,
			((RadixSort*)radixSort_.get())->sortedIndexBuffer()->blockReference()->address(),
			((RadixSort*)radixSort_.get())->sortedIndexBuffer()->blockReference()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (sortedIndexData) {
		uint32_t lastCell = 0;
		std::set<uint32_t> uniqueBoidIDs;
		bool isSorted = true;
		for (uint32_t i = 0; i < numBoids_; ++i) {
			auto mappedID = sortedIndexData[i];
			if (mappedID >= numBoids_) {
				REGEN_ERROR("sorted index out of range: " << mappedID);
				isSorted = false;
			}
			if (sortKeys[mappedID] < lastCell) {
				isSorted = false;
			}
			lastCell = sortKeys[mappedID];
			uniqueBoidIDs.insert(mappedID);
			if (!isSorted) {
				REGEN_INFO("XXX: indices are not sorted");
			}
		}
		REGEN_INFO("Number of unique IDs: " << uniqueBoidIDs.size());
		REGEN_INFO("Number of repeated IDs (should be 0): " << numBoids_ - uniqueBoidIDs.size());
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}
}
