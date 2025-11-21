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
		for (uint32_t i = 0; i < numBoids_; ++i) {
			initialPositions[i] = tf_->modelMat()->getVertex(i).r.position();
#ifdef BOID_USE_HALF_VELOCITY
			initialVelocities[i] = Vec2ui::zero();
#else
			initialVelocities[i] = Vec3f::zero();
#endif
		}
	}
	else if(tf_->hasModelOffset()) {
		for (uint32_t i = 0; i < numBoids_; ++i) {
			initialPositions[i] = tf_->modelOffset()->getVertex(i).r.xyz();
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
		gridUBO_ = ref_ptr<UBO>::alloc("BoidGrid", BufferUpdateFlags::PARTIAL_RARELY);
		gridMin_ = ref_ptr<ShaderInput3f>::alloc("gridMin");
		gridMin_->setUniformData(gridBounds_.min);
		gridUBO_->addStagedInput(gridMin_);
		gridUBO_->addStagedInput(cellSize_);
		gridUBO_->addStagedInput(gridSize_);
		// create draw buffer and add to staging system
		gridUBO_->update();
	}

	// SSBO for position, one per boid
	if (tf_.get()) {
		// bind UBO as SSBO for writing model matrix
		auto bufferContainer = tf_->tfBuffer();
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
		BufferUpdateFlags::FULL_PER_FRAME,
		SSBO::RESTRICT);
#ifdef BOID_USE_HALF_VELOCITY
	velBuffer_->addStagedInput(ref_ptr<ShaderInput2ui>::alloc("vel", numBoids_));
#else
	velBuffer_->addBlockInput(ref_ptr<ShaderInput1f>::alloc("vel", numBoids_ * 3));
#endif
	velBuffer_->update();
	velBuffer_->setBufferData(initialVelocities.data());

	// bounding box SSBO as we read back bounding box to CPU
	bboxBuffer_ = ref_ptr<BBoxBuffer>::alloc(boidBounds_);
	{
		bboxPass_ = ref_ptr<ComputePass>::alloc("regen.compute.bbox");
		bboxPass_->computeState()->setNumWorkUnits(numBoids_, 1, 1);
		bboxPass_->computeState()->setGroupSize(simulationGroupSize_, 1, 1);
		if (tf_.get()) {
			bboxPass_->setInput(tfBuffer_);
		}
		bboxPass_->setInput(bboxBuffer_);
		StateConfigurer shaderConfigurer;
		shaderConfigurer.define("NUM_ELEMENTS", REGEN_STRING(numBoids_));
		shaderConfigurer.addState(bboxPass_.get());
		bboxPass_->createShader(shaderConfigurer.cfg());
	}
	{ // SSBO for grid offsets
		gridOffsetBuffer_ = ref_ptr<SSBO>::alloc("GridOffsets",
			BufferUpdateFlags::FULL_PER_FRAME,
			SSBO::RESTRICT);
		gridOffsetBuffer_->addStagedInput(ref_ptr<ShaderInput1ui>::alloc(
				"globalHistogram", numCells_ + 1));
		gridOffsetBuffer_->update();
	}

	if (tf_.get()) {
		animationState()->setInput(tfBuffer_);
	}
	// create a state that updates the boids grid
	updateGridState_ = ref_ptr<StateSequence>::alloc();
	#ifdef BOID_USE_SORTED_DATA
	{
		// NOTE: this might be a HUGE buffer. It stores boid
		// positions and velocities.
		boidDataBuffer_ = ref_ptr<SSBO>::alloc("BoidDataBuffer",
			BufferUpdateFlags::NEVER, SSBO::RESTRICT);
		boidDataBuffer_->addStagedInput(ref_ptr<ShaderInputStruct<BoidData>>::alloc("BoidData", "boidData", numBoids_));
		boidDataBuffer_->update();
	}
	#endif
	{
		auto radixSort = ref_ptr<RadixSort_GPU>::alloc(numBoids_);
		// note: with compaction enabled, there will be an additional "numVisibleKeys" field in the Key buffer,
		//       that needs to be filled externally (one value per layer).
		radixSort->setUseCompaction(false);
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
		updateState->setInput(gridUBO_);
		updateState->setInput(((RadixSort_GPU*)radixSort_.get())->keyBuffer());
		updateState->setInput(((RadixSort_GPU*)radixSort_.get())->valueBuffer());
		updateState->setInput(gridOffsetBuffer_);
		updateState->setInput(u_numCells_);
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
		updateState->setInput(gridUBO_);
		updateState->setInput(((RadixSort_GPU*)radixSort_.get())->keyBuffer());
		updateState->setInput(((RadixSort_GPU*)radixSort_.get())->valueBuffer());
		updateState->setInput(gridOffsetBuffer_);
		#ifdef BOID_USE_SORTED_DATA
		updateState->setInput(velBuffer_);
		if (tf_.get()) {
			updateState->setInput(tfBuffer_);
		}
		updateState->setInput(boidDataBuffer_);
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
		simulationState_->setInput(simulationBoundsMin_);
		simulationState_->setInput(visualRange_);
		simulationState_->setInput(simulationBoundsMax_);
		simulationState_->setInput(maxBoidSpeed_);
		simulationState_->setInput(boidsScale_);
		simulationState_->setInput(baseOrientation_);
		simulationState_->setInput(maxAngularSpeed_);
		simulationState_->setInput(coherenceWeight_);
		simulationState_->setInput(alignmentWeight_);
		simulationState_->setInput(separationWeight_);
		simulationState_->setInput(avoidanceWeight_);
		simulationState_->setInput(avoidanceDistance_);
		simulationState_->setInput(lookAheadDistance_);
		simulationState_->setInput(repulsionFactor_);
		simulationState_->setInput(maxNumNeighbors_);
	}
	simulationState_->setInput(gridUBO_);
	simulationState_->setInput(velBuffer_);
	simulationState_->setInput(gridOffsetBuffer_);
	simulationState_->setInput(((RadixSort_GPU*)radixSort_.get())->valueBuffer());
	if (tf_.get()) {
		simulationState_->setInput(tfBuffer_);
	}
#ifdef BOID_USE_SORTED_DATA
	simulationState_->setInput(boidDataBuffer_);
#endif
	if (heightMap_.get()) {
		simulationState_->setInput(
			createUniform<ShaderInput3f,Vec3f>("mapCenter", mapCenter_));
		simulationState_->setInput(
			createUniform<ShaderInput1f,float>("heightMapFactor", heightMapFactor_));
		simulationState_->setInput(
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
		if(bboxBuffer_->updateBoundingBox() || vrStamp_ != visualRange_->stampOfReadData()) {
			auto &newBounds = bboxBuffer_->bbox();
			if (newBounds.max != newBounds.min) {
				boidBounds_ = newBounds;
				updateGrid();
				vrStamp_ = visualRange_->stampOfReadData();
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
}

void BoidsGPU::updateGrid() {
	auto lastNumCells = numCells_;
	updateGridSize();
	numCells_ = std::max(1u, numCells_);
	gridMin_->setVertex(0, gridBounds_.min);
	if (lastNumCells != numCells_) {
		// acquire buffer space of the right size
		gridResetPass_->computeState()->setNumWorkUnits(std::max(numBoids_, numCells_ + 1), 1, 1);
		gridOffsetBuffer_->stagedInputs().front().in_->set_numArrayElements(static_cast<int>(numCells_ + 1u));
		gridOffsetBuffer_->update(true);
		u_numCells_->setVertex(0, numCells_);
		REGEN_INFO("Boid grid size changed to " << numCells_ << " cells.");
	}
}

void BoidsGPU::simulate(RenderState *rs, double boidTimeDelta) {
	simulationState_->enable(rs);
	glUniform1f(simulationTimeLoc_, static_cast<float>(boidTimeDelta*0.001));
	simulationState_->disable(rs);
}

void BoidsGPU::printOffsets(RenderState *rs) {
	auto offsetData = (uint32_t *) glMapNamedBufferRange(
			gridOffsetBuffer_->drawBufferRef()->bufferID(),
			gridOffsetBuffer_->drawBufferRef()->address(),
			gridOffsetBuffer_->drawBufferRef()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (offsetData) {
		std::stringstream sss;
		sss << "    offsets: | ";
		for (uint32_t i = 0; i < numCells_+1; ++i) {
			sss << offsetData[i] << " ";
		}
		REGEN_INFO(" " << sss.str());
		glUnmapNamedBuffer(gridOffsetBuffer_->drawBufferRef()->bufferID());
	}
}

void BoidsGPU::debugVelocity(RenderState *rs) {
#ifndef BOID_USE_HALF_VELOCITY
	auto velData = (Vec3f *) glMapNamedBufferRange(
			velBuffer_->drawBufferRef()->bufferID(),
			velBuffer_->drawBufferRef()->address(),
			velBuffer_->drawBufferRef()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (velData) {
		std::stringstream sss;
		sss << "    velocities: | ";
		for (uint32_t i = 0; i < numBoids_; ++i) {
			sss << velData[i] << " ";
		}
		REGEN_INFO(" " << sss.str());
		glUnmapNamedBuffer(velBuffer_->drawBufferRef()->bufferID());
	}
#endif
}

void BoidsGPU::debugGridSorting(RenderState *rs) {
	// read the key buffer
	std::vector<uint32_t> sortKeys(numBoids_);
	uint32_t bufferID = ((RadixSort_GPU*)radixSort_.get())->keyBuffer()->drawBufferRef()->bufferID();
	auto sortKeysData = (uint32_t *) glMapNamedBufferRange(
			bufferID,
			((RadixSort_GPU*)radixSort_.get())->keyBuffer()->drawBufferRef()->address(),
			((RadixSort_GPU*)radixSort_.get())->keyBuffer()->drawBufferRef()->allocatedSize(),
			GL_MAP_READ_BIT);
	if (sortKeysData) {
		std::set<uint32_t> usedCells;
		for (uint32_t i = 0; i < numBoids_; ++i) {
			sortKeys[i] = sortKeysData[i];
			usedCells.insert(sortKeys[i]);
		}
		REGEN_INFO("Number of occupied cells: " << usedCells.size());
		glUnmapNamedBuffer(bufferID);
	}

	// second map the index buffer and test if the indices are sorted correctly
	bufferID = ((RadixSort_GPU*)radixSort_.get())->valueBuffer()->drawBufferRef()->bufferID();
	auto sortedIndexData = (uint32_t *) glMapNamedBufferRange(
			bufferID,
			((RadixSort_GPU*)radixSort_.get())->valueBuffer()->drawBufferRef()->address(),
			((RadixSort_GPU*)radixSort_.get())->valueBuffer()->drawBufferRef()->allocatedSize(),
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
		glUnmapNamedBuffer(bufferID);
	}
}
