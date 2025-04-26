#include "boid-simulation.h"
#include "regen/scene/resource-manager.h"

using namespace regen;

//#define BOID_DEBUG_TIME
#define BOID_GPU_USE_HALF_VELOCITY
#define BOID_COMPUTE_GROUP_SIZE 256
//#define BOID_USE_FIXED_GRID

BoidSimulation::BoidSimulation(const ref_ptr<ModelTransformation> &tf) : tf_(tf) {
	auto tfInput = tf_->get();
	auto tfData = tfInput->mapClientData<Mat4f>(ShaderData::READ);
	boidsScale_ = ref_ptr<ShaderInput3f>::alloc("scaleFactor");
	boidsScale_->setUniformData(tfData.r[0].scaling());
	numBoids_ = static_cast<int>(tfInput->numInstances());
	tfData.unmap();
	initBoidSimulation0();
}

BoidSimulation::BoidSimulation(const ref_ptr<ShaderInput3f> &position) :
		  position_(position) {
	numBoids_ = static_cast<int>(position_->numInstances());
	boidsScale_ = ref_ptr<ShaderInput3f>::alloc("scaleFactor");
	boidsScale_->setUniformData(Vec3f(1.0f));
	initBoidSimulation0();
}

void BoidSimulation::initBoidSimulation0() {
	baseOrientation_ = createUniform<ShaderInput1f,float>("baseOrientation", 0.0f);
	coherenceWeight_ = createUniform<ShaderInput1f,float>("coherenceWeight", 1.0f);
	alignmentWeight_ = createUniform<ShaderInput1f,float>("alignmentWeight", 1.0f);
	separationWeight_ = createUniform<ShaderInput1f,float>("separationWeight", 1.0f);
	avoidanceWeight_ = createUniform<ShaderInput1f,float>("avoidanceWeight", 1.0f);
	avoidanceDistance_ = createUniform<ShaderInput1f,float>("avoidanceDistance", 0.1f);
	visualRange_ = createUniform<ShaderInput1f,float>("visualRange", 1.6f);
	lookAheadDistance_ = createUniform<ShaderInput1f,float>("lookAheadDistance", 0.1f);
	repulsionFactor_ = createUniform<ShaderInput1f,float>("repulsionFactor", 20.0f);
	maxNumNeighbors_ = createUniform<ShaderInput1ui,unsigned int>("maxNumNeighbors", 20);
	maxBoidSpeed_ = createUniform<ShaderInput1f,float>("maxBoidSpeed", 1.0f);
	maxAngularSpeed_ = createUniform<ShaderInput1f,float>("maxAngularSpeed", 0.05f);
	gridSize_ = createUniform<ShaderInput3i,Vec3i>("gridSize", Vec3i(0));
	cellSize_ = createUniform<ShaderInput1f,float>("cellSize", 0.0f);
	simulationBoundsMin_ = createUniform<ShaderInput3f,Vec3f>("simulationBoundsMin", Vec3f(-10.0f));
	simulationBoundsMax_ = createUniform<ShaderInput3f,Vec3f>("simulationBoundsMax", Vec3f(10.0f));
}

void BoidSimulation::setVisualRange(float range) {
	visualRange_->setVertex(0, range);
	cellSize_->setVertex(0, range * 2.0f );
}

void BoidSimulation::setSimulationBounds(const Bounds<Vec3f> &bounds) {
	simulationBoundsMin_->setVertex(0, bounds.min);
	simulationBoundsMax_->setVertex(0, bounds.max);
}

void BoidSimulation::setBaseOrientation(float orientation) {
	baseOrientation_->setVertex(0, orientation);
}

void BoidSimulation::setMap(
		const Vec3f &mapCenter,
		const Vec2f &mapSize,
		const ref_ptr<Texture2D> &heightMap,
		float heightMapFactor) {
	mapCenter_ = mapCenter;
	mapSize_ = mapSize;
	heightMap_ = heightMap;
	heightMapFactor_ = heightMapFactor;
}

void BoidSimulation::addHomePoint(const Vec3f &homePoint) {
	homePoints_.push_back(homePoint);
}

void BoidSimulation::addObject(
		ObjectType objectType,
		const ref_ptr<ShaderInputMat4> &tf,
		const ref_ptr<ShaderInput3f> &offset) {
	SimulationEntity *entity = nullptr;
	switch (objectType) {
		case ObjectType::ATTRACTOR:
			entity = &attractors_.emplace_back();
			break;
		case ObjectType::DANGER:
			entity = &dangers_.emplace_back();
			break;
	}
	if (!entity) { return; }
	entity->tf = tf;
	entity->pos = offset;
}

Vec3f BoidSimulation::getCellCenter(const Vec3i &gridIndex) const {
	Vec3f v(
		static_cast<float>(gridIndex.x),
		static_cast<float>(gridIndex.y),
		static_cast<float>(gridIndex.z));
	return gridBounds_.min +
		v * cellSize_->getVertex(0).r +
		Vec3f(visualRange_->getVertex(0).r);
}

int BoidSimulation::getGridIndex(const Vec3i &v, const Vec3i &gridSize) {
	return v.x + v.y * gridSize.x + v.z * gridSize.x * gridSize.y;
}

Vec2f BoidSimulation::computeUV(const Vec3f &boidPosition, const Vec3f &mapCenter, const Vec2f &mapSize) {
	Vec2f boidCoord(
			boidPosition.x - mapCenter.x,
			boidPosition.z - mapCenter.z);
	return boidCoord / mapSize + Vec2f(0.5f);
}

void BoidSimulation::updateGridSize() {
	auto cs = cellSize_->getVertex(0).r;
	// ensure we stay in simulation bounds
	auto simMin = simulationBoundsMin_->getVertex(0);
	auto simMax = simulationBoundsMax_->getVertex(0);
#ifdef BOID_USE_FIXED_GRID
	gridBounds_.min = simMin.r;
	gridBounds_.max = simMax.r;
#else
	gridBounds_.min = Vec3f::max(boidBounds_.min, simMin.r);
	gridBounds_.max = Vec3f::min(boidBounds_.max, simMax.r);
#endif
	// Here we compute the grid based on cell size which is twice the visual range.
	// The idea is that we only need to consider neighbors in one direction per dimension.
	// Here, we define neighbourhood based on distance, and also compute avoidance influence based on distance.
	// Alternatively, it would be possible to skip distance computation entirely, and define both via cell adjacency.
	auto gridSize = (gridBounds_.max - gridBounds_.min) / cs;
	Vec3i v_gridSize(
		static_cast<int>(ceil(gridSize.x)),
		static_cast<int>(ceil(gridSize.y)),
		static_cast<int>(ceil(gridSize.z)));
	gridSize_->setVertex(0, v_gridSize);
	numCells_ = v_gridSize.x * v_gridSize.y * v_gridSize.z;
}

//////////////////////////
/////////////////////////

// TODO: support interleaved layout using structs. But these are not yet
//       supported in REGEN. might be better for eg. boid and particles

BoidSimulation_GPU::BoidSimulation_GPU(const ref_ptr<ModelTransformation> &tf)
		: BoidSimulation(tf),
		  Animation(true, false) {
}

BoidSimulation_GPU::BoidSimulation_GPU(const ref_ptr<ShaderInput3f> &position)
		: BoidSimulation(position),
		  Animation(true, false) {
}

void BoidSimulation_GPU::initBoidSimulation() {
	// TODO: support attractors and dangers.
	//         - join their transform input
	//         - then use define some macros to generate the shader code
	setAnimationName("boids");
	initBuffers();
	initAnimationState();
	GL_ERROR_LOG();
}

void BoidSimulation_GPU::computeBBox(const Vec3f *posData) {
	boidBounds_.min = posData[0];
	boidBounds_.max = posData[0];
	for (int i = 1; i < numBoids_; ++i) {
		boidBounds_.min.setMin(posData[i]);
		boidBounds_.max.setMax(posData[i]);
	}
}

void BoidSimulation_GPU::initBuffers() {
	std::vector<Vec3f> initialPositions(numBoids_);
#ifdef BOID_GPU_USE_HALF_VELOCITY
	std::vector<Vec2ui> initialVelocities(numBoids_);
#else
	std::vector<Vec3f> initialVelocities(numBoids_);
#endif
	if(position_.get()) {
		auto initialPositionData = position_->mapClientData<Vec3f>(ShaderData::READ);
		for (int i = 0; i < numBoids_; ++i) {
			initialPositions[i] = initialPositionData.r[i];
#ifdef BOID_GPU_USE_HALF_VELOCITY
			initialVelocities[i] = Vec2ui::zero();
#else
			initialVelocities[i] = Vec3f::zero();
#endif
		}
	} else {
		auto tfData = tf_->get()->mapClientData<Mat4f>(ShaderData::READ);
		for (int i = 0; i < numBoids_; ++i) {
			initialPositions[i] = tfData.r[i].position();
#ifdef BOID_GPU_USE_HALF_VELOCITY
			initialVelocities[i] = Vec2ui::zero();
#else
			initialVelocities[i] = Vec3f::zero();
#endif
		}
	}
	// compute initial bbox
	computeBBox(initialPositions.data());
	updateGridSize();

	// UBO with simulation parameters
	simulationUBO_ = ref_ptr<UBO>::alloc("BoidsSimulation");
	simulationUBO_->addBlockInput(simulationBoundsMin_);
	simulationUBO_->addBlockInput(visualRange_);
	simulationUBO_->addBlockInput(simulationBoundsMax_);
	simulationUBO_->addBlockInput(maxBoidSpeed_);
	simulationUBO_->addBlockInput(boidsScale_);
	simulationUBO_->addBlockInput(baseOrientation_);
	simulationUBO_->addBlockInput(maxAngularSpeed_);
	simulationUBO_->addBlockInput(coherenceWeight_);
	simulationUBO_->addBlockInput(alignmentWeight_);
	simulationUBO_->addBlockInput(separationWeight_);
	simulationUBO_->addBlockInput(avoidanceWeight_);
	simulationUBO_->addBlockInput(avoidanceDistance_);
	simulationUBO_->addBlockInput(lookAheadDistance_);
	simulationUBO_->addBlockInput(repulsionFactor_);
	simulationUBO_->addBlockInput(maxNumNeighbors_);

	// UBO with grid parameters
	gridUBO_ = ref_ptr<UBO>::alloc("BoidGrid");
	gridMin_ = ref_ptr<ShaderInput3f>::alloc("gridMin");
	gridMin_->setUniformData(gridBounds_.min);
	gridUBO_->addBlockInput(gridMin_);
	gridUBO_->addBlockInput(cellSize_);
	gridUBO_->addBlockInput(gridSize_);

	// bounding box SSBO as we read back bounding box to CPU
	bboxBuffer_ = ref_ptr<BBoxBuffer>::alloc();

	// SSBO for position, one per boid
	if (tf_.get()) {
		// bind UBO as SSBO for writing model matrix
		auto bufferContainer = tf_->bufferContainer();
		bufferContainer->updateBuffer();
		auto bufferObject = bufferContainer->getBufferObject(tf_->modelMat());
		tfBuffer_ = ref_ptr<SSBO>::alloc(*bufferObject.get());
	}

	// SSBO for velocity, one per boid
	velBuffer_ = ref_ptr<SSBO>::alloc("VelocityBlock", BUFFER_USAGE_DYNAMIC_DRAW);
#ifdef BOID_GPU_USE_HALF_VELOCITY
	auto vel = ref_ptr<ShaderInput2ui>::alloc("vel", numBoids_);
#else
	auto vel = ref_ptr<ShaderInput1f>::alloc("vel", numBoids_ * 3);
#endif
	vel->setInstanceData(1, 1, (byte*)initialVelocities.data());
	velBuffer_->addBlockInput(vel);
	velBuffer_->update();

	// SSBOs for neighbour grid
	listHeadBuffer_ = ref_ptr<SSBO>::alloc("BoidCellHeads", BUFFER_USAGE_DYNAMIC_DRAW);
	listHeads_ = ref_ptr<ShaderInput1i>::alloc("listHeads", numCells_);
	listHeads_->set_forceArray(true);
	// NOTE: listHeads must remain last in the list of buffer inputs as the number of cells might change over time.
	listHeadBuffer_->addBlockInput(listHeads_);
	listHeadBuffer_->update();

	listBodyBuffer_ = ref_ptr<SSBO>::alloc("BoidCellList", BUFFER_USAGE_DYNAMIC_DRAW);
	listBodyBuffer_->addBlockInput(ref_ptr<ShaderInput1i>::alloc("listNext", numBoids_));
	listBodyBuffer_->update();
}

void BoidSimulation_GPU::createShader(const ref_ptr<ComputePass> &pass, const ref_ptr<State> &update) {
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addState(animationState().get());
	shaderConfigurer.addState(update.get());
	pass->createShader(shaderConfigurer.cfg());
}

void BoidSimulation_GPU::initAnimationState() {
	if (tf_.get()) {
		animationState()->joinShaderInput(tfBuffer_);
	} else {
		animationState()->joinShaderInput(position_);
	}

	// create a state that updates the boids grid
	updateGridState_ = ref_ptr<State>::alloc();
	updateGridState_->joinShaderInput(gridUBO_);
	updateGridState_->joinShaderInput(listHeadBuffer_);
	updateGridState_->joinShaderInput(listBodyBuffer_);
	computeGridState_ = ref_ptr<ComputePass>::alloc("regen.animation.boid.grid");
	computeGridState_->computeState()->setNumWorkUnits(numBoids_, 1, 1);
	computeGridState_->computeState()->setGroupSize(BOID_COMPUTE_GROUP_SIZE, 1, 1);
	updateGridState_->joinStates(computeGridState_);
	createShader(computeGridState_, updateGridState_);

	// create a state that updates the boids positions and velocities
	updateBoidsState_ = ref_ptr<State>::alloc();
#ifdef BOID_GPU_USE_HALF_VELOCITY
	updateBoidsState_->shaderDefine("USE_HALF_VELOCITY", "TRUE");
#endif
	updateBoidsState_->joinShaderInput(simulationUBO_);
	updateBoidsState_->joinShaderInput(gridUBO_);
	updateBoidsState_->joinShaderInput(velBuffer_);
	updateBoidsState_->joinShaderInput(listHeadBuffer_);
	updateBoidsState_->joinShaderInput(listBodyBuffer_);
	updateBoidsState_->joinShaderInput(bboxBuffer_);
	if (heightMap_.get()) {
		updateBoidsState_->joinShaderInput(
			createUniform<ShaderInput3f,Vec3f>("mapCenter", mapCenter_));
		updateBoidsState_->joinShaderInput(
			createUniform<ShaderInput1f,float>("heightMapFactor", heightMapFactor_));
		updateBoidsState_->joinShaderInput(
			createUniform<ShaderInput2f,Vec2f>("mapSize", mapSize_));
		updateBoidsState_->joinStates(
			ref_ptr<TextureState>::alloc(heightMap_, "heightMap"));
	}
	if (homePoints_.empty()) {
		updateBoidsState_->shaderDefine("NUM_BOID_HOMES", "0");
	} else {
		updateBoidsState_->shaderDefine("NUM_BOID_HOMES", std::to_string(homePoints_.size()));
		for (uint64_t i = 0u; i < homePoints_.size(); ++i) {
			updateBoidsState_->shaderDefine(
				REGEN_STRING("BOID_HOME" << i),
				REGEN_STRING("vec3(" <<
					homePoints_[i].x << ", " <<
					homePoints_[i].y << ", " <<
					homePoints_[i].z << ")"));
		}
	}
	if (tf_.get()) {
		updateBoidsState_->joinShaderInput(tfBuffer_);
	}
	computeBoidState_ = ref_ptr<ComputePass>::alloc("regen.animation.boid.simulate");
	computeBoidState_->computeState()->setNumWorkUnits(numBoids_, 1, 1);
	computeBoidState_->computeState()->setGroupSize(BOID_COMPUTE_GROUP_SIZE, 1, 1);
	updateBoidsState_->joinStates(computeBoidState_);
	createShader(computeBoidState_, updateBoidsState_);
}

ref_ptr<BoidSimulation_GPU> BoidSimulation_GPU::load(
			LoadingContext &ctx,
			scene::SceneInputNode &input,
			const ref_ptr<ShaderInput3f> &position) {
	auto boids = ref_ptr<BoidSimulation_GPU>::alloc(position);
	boids->loadSettings(ctx, input);
	return boids;
}

ref_ptr<BoidSimulation_GPU> BoidSimulation_GPU::load(
			LoadingContext &ctx,
			scene::SceneInputNode &input,
			const ref_ptr<ModelTransformation> &tf) {
	auto boids = ref_ptr<BoidSimulation_GPU>::alloc(tf);
	boids->loadSettings(ctx, input);
	return boids;
}

void BoidSimulation_GPU::glAnimate(RenderState *rs, GLdouble dt) {
	// limit FPS to 60
	time_ += dt;
	if (time_ > 0.016666) { time_ = 0.0; }
	else { return; }

	bboxBuffer_->clear();
	updateGridState_->enable(rs);
	updateGridState_->disable(rs);
	simulate(rs, dt);
	// update the grid in case the bounding box around the boids changed.
	if(bboxBuffer_->updateBoundingBox() || vrStamp_ != cellSize_->stamp()) {
		boidBounds_ = bboxBuffer_->bbox();
		updateGrid();
		vrStamp_ = visualRange_->stamp();
	}
	GL_ERROR_LOG();
}

void BoidSimulation_GPU::updateGrid() {
	auto lastNumCells = numCells_;
	updateGridSize();
	gridMin_->setVertex(0, gridBounds_.min);
	if (lastNumCells != numCells_) {
		// need to resize the list head buffer
		listHeads_->set_numArrayElements(numCells_);
	}
}

void BoidSimulation_GPU::simulate(RenderState *rs, double ) {
	updateBoidsState_->enable(rs);
	updateBoidsState_->disable(rs);
}

//////////////////////////
/////////////////////////

// private data struct
struct BoidSimulation_CPU::Private {
	Vec3f avgPosition_ = Vec3f::zero();
	Vec3f avgVelocity_ = Vec3f::zero();
	Vec3f separation_ = Vec3f::zero();
	Vec3f boidDirection_ = Vec3f::front();
	Quaternion boidRotation_;

	// Boid spatial grid. A cell in a 3D grid with edge length equal to boid visual range.
	struct Cell { std::vector<int> elements; };
	std::vector<Cell> grid_;
	// Configuration parameters
	float visualRange_ = 0.0f;
	float avoidanceDistance_ = 0.0f;
	float avoidanceDistanceHalf_ = 0.0f;
	float repulsionTimesSeparation_ = 0.0f;
	float separationWeight_ = 0.0f;
	float alignmentWeight_ = 0.0f;
	float coherenceWeight_ = 0.0f;
	float lookAheadDistance_ = 0.0f;
	float maxBoidSpeed_ = 0.0f;
	float maxAngularSpeed_ = 0.0f;
	float baseOrientation_ = 0.0f;
	float cellSize_ = 0.0f;
	Vec3i gridSize_ = Vec3i::zero();
	Vec3f boidsScale_ = Vec3f::zero();
	unsigned int maxNumNeighbors_ = 0;
	Bounds<Vec3f> simBounds_ = Bounds<Vec3f>(-10.0f, 10.0f);
};

BoidSimulation_CPU::BoidSimulation_CPU(const ref_ptr<ModelTransformation> &tf)
		: BoidSimulation(tf),
		  Animation(false, true),
		  priv_(new Private())
{
	auto tfData = tf_->get()->mapClientData<Mat4f>(ShaderData::READ);
	boidData_.resize(numBoids_);
	boidPositions_.resize(numBoids_);
	for (int i = 0; i < numBoids_; ++i) {
		boidPositions_[i] = tfData.r[i].position();
		boidData_[i].velocity = Vec3f::zero();
	}
}

BoidSimulation_CPU::BoidSimulation_CPU(const ref_ptr<ShaderInput3f> &position)
		: BoidSimulation(position),
		  Animation(false, true),
		  priv_(new Private())
{
	auto initialPositionData = position_->mapClientData<Vec3f>(ShaderData::READ);
	boidData_.resize(numBoids_);
	boidPositions_.resize(numBoids_);
	for (int i = 0; i < numBoids_; ++i) {
		auto &d = boidData_[i];
		boidPositions_[i] = initialPositionData.r[i];
		d.velocity = Vec3f::zero();
	}
}

BoidSimulation_CPU::~BoidSimulation_CPU() {
	delete priv_;
}

void BoidSimulation_CPU::initBoidSimulation() {
	setAnimationName("boids");
	// use a dedicated thread for the boids simulation which is not synchronized with the graphics thread,
	// i.e. it can be slower or faster than the graphics thread.
	setSynchronized(false);
	priv_->baseOrientation_ = baseOrientation_->getVertex(0).r;
	priv_->boidsScale_ = boidsScale_->getVertex(0).r;

	animationState()->joinShaderInput(coherenceWeight_);
	animationState()->joinShaderInput(alignmentWeight_);
	animationState()->joinShaderInput(separationWeight_);
	animationState()->joinShaderInput(avoidanceWeight_);
	animationState()->joinShaderInput(avoidanceDistance_);
	animationState()->joinShaderInput(visualRange_);
	animationState()->joinShaderInput(lookAheadDistance_);
	animationState()->joinShaderInput(repulsionFactor_);
	animationState()->joinShaderInput(maxNumNeighbors_);
	animationState()->joinShaderInput(maxBoidSpeed_);
	animationState()->joinShaderInput(maxAngularSpeed_);
}

ref_ptr<BoidSimulation_CPU> BoidSimulation_CPU::load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ShaderInput3f> &position) {
	auto boids = ref_ptr<BoidSimulation_CPU>::alloc(position);
	boids->loadSettings(ctx, input);
	return boids;
}

ref_ptr<BoidSimulation_CPU> BoidSimulation_CPU::load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf) {
	auto boids = ref_ptr<BoidSimulation_CPU>::alloc(tf);
	boids->loadSettings(ctx, input);
	return boids;
}

void BoidSimulation_CPU::animate(double dt) {
	auto dt_f = static_cast<float>(dt) * 0.001f;
	priv_->simBounds_.min = simulationBoundsMin_->getVertex(0).r;
	priv_->simBounds_.max = simulationBoundsMax_->getVertex(0).r;
	priv_->avoidanceDistance_ = avoidanceDistance_->getVertex(0).r;
	priv_->avoidanceDistanceHalf_ = priv_->avoidanceDistance_ * 0.5f;
	priv_->repulsionTimesSeparation_ = repulsionFactor_->getVertex(0).r * separationWeight_->getVertex(0).r;
	priv_->visualRange_ = visualRange_->getVertex(0).r;
	priv_->separationWeight_ = separationWeight_->getVertex(0).r;
	priv_->alignmentWeight_ = alignmentWeight_->getVertex(0).r;
	priv_->coherenceWeight_ = coherenceWeight_->getVertex(0).r;
	priv_->lookAheadDistance_ = lookAheadDistance_->getVertex(0).r;
	priv_->maxBoidSpeed_ = maxBoidSpeed_->getVertex(0).r;
	priv_->maxAngularSpeed_ = maxAngularSpeed_->getVertex(0).r;
	priv_->maxNumNeighbors_ = maxNumNeighbors_->getVertex(0).r;
	priv_->cellSize_ = cellSize_->getVertex(0).r;
#ifdef BOID_DEBUG_TIME
	auto start = std::chrono::high_resolution_clock::now();
	simulateBoids(dt_f);
	auto afterSim = std::chrono::high_resolution_clock::now();
#else
	simulateBoids(dt_f);
#endif
	// update boids model transformation using the boids data
	updateTransforms();
#ifdef BOID_DEBUG_TIME
	auto afterTF = std::chrono::high_resolution_clock::now();
#endif
	int gridIndex = 0;
	// resize the grid, and clear all cells
	updateGrid();
	if (priv_->grid_.empty()) { return; }
	// iterate over all boids and add them to the grid, compute the index
	// based on the boid position and the grid bounds.
	for (int i = 0; i < numBoids_; ++i) {
		auto &boid = boidData_[i];
		boid.gridIndex = getGridIndex3D(boidPositions_[i]);
		gridIndex = getGridIndex(boid.gridIndex, priv_->gridSize_);
		priv_->grid_[gridIndex].elements.push_back(i);
	}
	// recompute neighborhood relationship
	// TODO: do this in one loop, but then we can only consider neighbors with smaller index!
	for (int i = 0; i < numBoids_; ++i) {
		updateNeighbours0(boidData_[i], boidPositions_[i], i);
	}
#ifdef BOID_DEBUG_TIME
	auto afterGrid = std::chrono::high_resolution_clock::now();
	auto simTime = std::chrono::duration_cast<std::chrono::microseconds>(afterSim - start).count();
	auto copyTime = std::chrono::duration_cast<std::chrono::microseconds>(afterTF - afterSim).count();
	auto gridTime = std::chrono::duration_cast<std::chrono::microseconds>(afterGrid - afterTF).count();
	REGEN_INFO("BoidsSimulation_CPU: " <<
		"simTime=" << static_cast<float>(simTime)/1000.0f << "ms " <<
		"copyTime=" << static_cast<float>(copyTime)/1000.0f << "ms " <<
		"gridTime=" << static_cast<float>(gridTime)/1000.0f << "ms " <<
		"totalTime=" << static_cast<float>(simTime + copyTime + gridTime)/1000.0f << "ms");
#endif
}

void BoidSimulation_CPU::updateTransforms() {
	if (tf_.get()) {
		auto &tfInput = tf_->get();
		auto tfData = tfInput->mapClientData<Mat4f>(ShaderData::READ | ShaderData::WRITE);
		float vl;

		for (int i = 0; i < numBoids_; ++i) {
			auto &d = boidData_[i];
			// calculate boid matrix, also need to compute rotation from velocity, model z
			//       should point in the direction of velocity.
			vl = d.velocity.length();
			if (vl > 0.001f) {
				// Convert the normalized direction vector to Euler angles
				priv_->boidRotation_.setEuler(
						atan2(-d.velocity.x/vl, d.velocity.z/vl) + priv_->baseOrientation_,
						asin(-d.velocity.y/vl),
						0.0f);
				tfData.w[i] = priv_->boidRotation_.calculateMatrix();
				tfData.w[i].scale(priv_->boidsScale_);
				tfData.w[i].translate(boidPositions_[i]);
			} else {
				tfData.w[i] = tfData.r[i];
			}
		}
	} else if (position_.get()) {
		auto positionData = position_->mapClientDataRaw(ShaderData::WRITE);
		std::memcpy(positionData.w, (byte*)boidPositions_.data(), numBoids_ * sizeof(Vec3f));
	}
}

void BoidSimulation_CPU::simulateBoids(float dt) {
	// reset bounds of the grid
	newBounds_.min = Vec3f::posMax();
	newBounds_.max = Vec3f::negMax();
	for (int i = 0; i < numBoids_; ++i) {
		auto &boidPos = boidPositions_[i];
		simulateBoid(boidData_[i], boidPos, dt);
		// update the grid bounds
		newBounds_.min.setMin(boidPos);
		newBounds_.max.setMax(boidPos);
	}
	boidBounds_ = newBounds_;
}

Vec3i BoidSimulation_CPU::getGridIndex3D(const Vec3f &x) const {
	auto boidPos = Vec3f::max(
		(x - gridBounds_.min) / priv_->cellSize_,
		// ensure the boid position is within the grid bounds
		Vec3f::zero());
	Vec3i gridIndex(
		static_cast<int>(std::trunc(boidPos.x)),
		static_cast<int>(std::trunc(boidPos.y)),
		static_cast<int>(std::trunc(boidPos.z)));
	return Vec3i::min(gridIndex,
		// ensure the boid position is within the grid bounds
		priv_->gridSize_ - Vec3i::one());
}

void BoidSimulation_CPU::updateNeighbours0(BoidData &boid, const Vec3f &boidPos, int boidIndex) {
	// find neighbours in the grid based on the current position of the boid
	boid.neighbors.clear();
	// find neighbours in the cell of the boid
	updateNeighbours2(boid, boidPos, boidIndex, boid.gridIndex);
	// find neighbours in the adjacent cells.
	// however we only need to consider one direction in each dimension
	// as the grid cell size is equal to the visual range times two.
	auto cellPosition = getCellCenter(boid.gridIndex);
	Vec3i dir;
	dir.x = (boidPos.x < cellPosition.x ? -1 : 1);
	dir.y = (boidPos.y < cellPosition.y ? -1 : 1);
	dir.z = (boidPos.z < cellPosition.z ? -1 : 1);
	updateNeighbours1(boid, boidPos, boidIndex, Vec3i(dir.x, 0, 0) + boid.gridIndex);
	updateNeighbours1(boid, boidPos, boidIndex, Vec3i(0, dir.y, 0) + boid.gridIndex);
	updateNeighbours1(boid, boidPos, boidIndex, Vec3i(0, 0, dir.z) + boid.gridIndex);
	updateNeighbours1(boid, boidPos, boidIndex, Vec3i(dir.x, dir.y, 0) + boid.gridIndex);
	updateNeighbours1(boid, boidPos, boidIndex, Vec3i(dir.x, 0, dir.z) + boid.gridIndex);
	updateNeighbours1(boid, boidPos, boidIndex, Vec3i(0, dir.y, dir.z) + boid.gridIndex);
	updateNeighbours1(boid, boidPos, boidIndex, Vec3i(dir.x, dir.y, dir.z) + boid.gridIndex);
}

void BoidSimulation_CPU::updateNeighbours1(BoidData &boid,
										   const Vec3f &boidPos, int boidIndex, const Vec3i &gridIndex) {
	if (gridIndex.x < 0 ||
	    gridIndex.y < 0 ||
	    gridIndex.z < 0) { return; }
	updateNeighbours2(boid, boidPos, boidIndex, gridIndex);
}

void BoidSimulation_CPU::updateNeighbours2(BoidData &boid,
										   const Vec3f &boidPos, int boidIndex, const Vec3i &gridIndex) {
	if (boid.neighbors.size() >= priv_->maxNumNeighbors_) { return; }
	if (gridIndex.x >= priv_->gridSize_.x ||
	    gridIndex.y >= priv_->gridSize_.y ||
	    gridIndex.z >= priv_->gridSize_.z) { return; }
	auto index = getGridIndex(gridIndex, priv_->gridSize_);

	for (auto neighborIndex: priv_->grid_[index].elements) {
		auto &neighbor = boidData_[neighborIndex];
		if (&neighbor == &boid) { continue; }

		auto &neighborPos = boidPositions_[neighborIndex];
		if ((boidPos - neighborPos).length() < priv_->visualRange_) {
			boid.neighbors.push_back(neighborIndex);
			if (neighbor.neighbors.size() < priv_->maxNumNeighbors_) {
				neighbor.neighbors.push_back(boidIndex);
			}
			if (boid.neighbors.size() >= priv_->maxNumNeighbors_) {
				return;
			}
		}
	}
}

void BoidSimulation_CPU::updateGrid() {
	// update the grid based on gridBounds_, creating a cell every `2.0*(visual range)` in all directions.
	updateGridSize();
	priv_->gridSize_ = gridSize_->getVertex(0).r;
	auto numCells = priv_->gridSize_.x * priv_->gridSize_.y * priv_->gridSize_.z;
	numCells = std::max(numCells, 0);
	priv_->grid_.resize(numCells);
	if (numCells==0) { return; }
	// clear all cells, removing the old boids.
	for (auto &cell: priv_->grid_) {
		cell.elements.clear();
	}
}

void BoidSimulation_CPU::simulateBoid(BoidData &boid, Vec3f &boidPos, float dt) {
	boid.force = Vec3f::zero();
	// a boid is lost if it is outside the bounds
	bool isBoidLost = !priv_->simBounds_.contains(boidPos);
	if (boid.neighbors.empty()) {
		// a boid without neighbors is lost
		isBoidLost = true;
	} else {
		// simulate the boid using the three rules of boids
		priv_->avgPosition_ = Vec3f::zero();
		priv_->avgVelocity_ = Vec3f::zero();
		priv_->separation_ = Vec3f::zero();
		for (auto neighborIndex: boid.neighbors) {
			auto &neighbor = boidData_[neighborIndex];
			auto &neighborPos = boidPositions_[neighborIndex];
			priv_->avgPosition_ += neighborPos;
			priv_->avgVelocity_ += neighbor.velocity;
			priv_->boidDirection_ = boidPos - neighborPos;
			float distance = priv_->boidDirection_.length();
			if (distance < 0.001f) {
				priv_->boidDirection_ = Vec3f::random();
				priv_->boidDirection_.normalize();
				distance = 0.0f;
			} else {
				priv_->boidDirection_ /= distance * distance;
			}
			if (distance < priv_->avoidanceDistance_) {
				priv_->separation_ += priv_->boidDirection_;
			}
		}
		priv_->avgPosition_ /= static_cast<float>(boid.neighbors.size());
		priv_->avgVelocity_ /= static_cast<float>(boid.neighbors.size());
		boid.force +=
				priv_->separation_ * priv_->separationWeight_ +
				(priv_->avgVelocity_ - boid.velocity) * priv_->alignmentWeight_ +
				(priv_->avgPosition_ - boidPos) * priv_->coherenceWeight_;
	}

	// put some restrictions on the boid's velocity.
	// a boid that cannot avoid collisions is considered lost.
	isBoidLost = !avoidCollisions(boid, boidPos, dt) || isBoidLost;
	auto isInDanger = !dangers_.empty() && !avoidDanger(boid, boidPos);
	if (!isInDanger) {
		// note: ignore attractors if in danger
		attract(boid, boidPos);
	}
	// drift towards home if lost
	if (isBoidLost) { homesickness(boid, boidPos); }

	boidPos += Vec3f(boid.velocity) * dt;
	auto lastDir = boid.velocity;
	lastDir.normalize();
	boid.velocity += boid.force * dt;
	limitVelocity(boid, lastDir);
}

void BoidSimulation_CPU::limitVelocity(BoidData &boid, const Vec3f &lastDir) {
	// limit translation speed
	if (boid.velocity.length() > priv_->maxBoidSpeed_) {
		boid.velocity.normalize();
		boid.velocity *= priv_->maxBoidSpeed_;
	}

	// limit angular speed
	priv_->boidDirection_ = boid.velocity;
	priv_->boidDirection_.normalize();
	auto angle = acos(priv_->boidDirection_.dot(lastDir));
	if (angle > priv_->maxAngularSpeed_) {
		auto axis = priv_->boidDirection_.cross(lastDir);
		axis.normalize();
		priv_->boidRotation_.setAxisAngle(axis, priv_->maxAngularSpeed_);
		auto newDirection = priv_->boidRotation_.rotate(lastDir);
		boid.velocity = newDirection * boid.velocity.length();
	}
}

void BoidSimulation_CPU::homesickness(BoidData &boid, const Vec3f &boidPos) {
	// a boid seems to have lost track, and wants to go home!
	// first find closest home point...
	const Vec3f *closestHomePoint = &Vec3f::zero();
	float minDistance;
	if (!homePoints_.empty()) {
		minDistance = std::numeric_limits<float>::max();
		for (auto &home: homePoints_) {
			auto distance = (boidPos - home).length();
			if (distance < minDistance) {
				minDistance = distance;
				closestHomePoint = &home;
			}
		}
	} else {
		minDistance = (boidPos - *closestHomePoint).length();
	}
	// second steer towards the closest home point ...
	if (minDistance < 0.001f) {
		priv_->boidDirection_ = Vec3f::random();
		priv_->boidDirection_.normalize();
	} else {
		priv_->boidDirection_ = (*closestHomePoint - boidPos) / minDistance;
	}
	boid.force += priv_->boidDirection_ * priv_->repulsionTimesSeparation_ * 0.1;
}

bool BoidSimulation_CPU::avoidDanger(BoidData &boid, const Vec3f &boidPos) {
	bool isInDanger = false;
	Vec3f dir;

	for (auto &danger: dangers_) {
		dir = Vec3f::zero();
		if (danger.pos.get()) {
			dir = danger.pos.get()->getVertex(0).r;
		}
		if (danger.tf.get()) {
			dir += danger.tf.get()->getVertex(0).r.position();
		}
		dir -= boidPos;
		auto distance = dir.length();
		if (distance < priv_->visualRange_) {
			if (distance < 0.001f) {
				dir = Vec3f::random();
				dir.normalize();
			} else {
				dir /= distance;
			}
			boid.force += dir * priv_->repulsionTimesSeparation_;
			isInDanger = true;
		}
	}
	return !isInDanger;
}

void BoidSimulation_CPU::attract(BoidData &boid, const Vec3f &boidPos) {
	auto maxDistance = priv_->visualRange_ * 100; // TODO: parameter
	Vec3f dir;
	for (auto &attractor: attractors_) {
		dir = Vec3f::zero();
		if (attractor.pos.get()) {
			dir = attractor.pos.get()->getVertex(0).r;
		}
		if (attractor.tf.get()) {
			dir += attractor.tf.get()->getVertex(0).r.position();
		}
		dir -= boidPos;
		auto distance = dir.length();
		if (distance < maxDistance && distance > 0.001f) {
			dir /= distance;
			boid.force += dir * priv_->repulsionTimesSeparation_;
		}
	}
}

bool BoidSimulation_CPU::avoidCollisions(BoidData &boid, const Vec3f &boidPos, float dt) {
	auto nextVelocity = boid.velocity + boid.force * dt;
	nextVelocity.normalize();
	auto lookAhead = boidPos + nextVelocity * priv_->lookAheadDistance_;
	bool isCollisionFree = true;

	////////////////
	/////// Collision with boundaries.
	////////////////
	if (lookAhead.x < priv_->simBounds_.min.x + priv_->avoidanceDistance_) {
		boid.force.x += priv_->repulsionTimesSeparation_;
	}
	else if (lookAhead.x > priv_->simBounds_.max.x - priv_->avoidanceDistance_) {
		boid.force.x -= priv_->repulsionTimesSeparation_;
	}
	if (lookAhead.y < priv_->simBounds_.min.y + priv_->avoidanceDistance_) {
		boid.force.y += priv_->repulsionTimesSeparation_;
	}
	else if (lookAhead.y > priv_->simBounds_.max.y - priv_->avoidanceDistance_) {
		boid.force.y -= priv_->repulsionTimesSeparation_;
	}
	if (lookAhead.z < priv_->simBounds_.min.z + priv_->avoidanceDistance_) {
		boid.force.z += priv_->repulsionTimesSeparation_;
	}
	else if (lookAhead.z > priv_->simBounds_.max.z - priv_->avoidanceDistance_) {
		boid.force.z -= priv_->repulsionTimesSeparation_;
	}

	////////////////
	/////// Collision with height map.
	////////////////
	if (heightMap_.get()) {
		// boid position in height map space [0, 1]
		auto boidCoord = computeUV(boidPos, mapCenter_, mapSize_);
		auto currentY = heightMap_->sampleLinear<float>(boidCoord, heightMap_->textureData());
		currentY *= heightMapFactor_;
		currentY += mapCenter_.y + priv_->avoidanceDistanceHalf_;
		// sample the height map at the projected boid position
		boidCoord = computeUV(lookAhead, mapCenter_, mapSize_);
		auto nextY = heightMap_->sampleLinear<float>(boidCoord, heightMap_->textureData());
		nextY *= heightMapFactor_;
		nextY += mapCenter_.y + priv_->avoidanceDistanceHalf_;

		if (boidPos.y < currentY) {
			// The boid is below the minimum height of the height map -> push boid up.
			if (currentY < priv_->simBounds_.max.y - priv_->avoidanceDistanceHalf_) {
				// only push up in case the height map y is below the maximum y of boids
				boid.force.y += priv_->repulsionTimesSeparation_;
			}
			isCollisionFree = false;
		}
		if (lookAhead.y < nextY) {
			// Assuming the boid follows the lookAhead direction, it will eventually reach a point where
			// it is below the surface of the height map -> push boid up.
			if (nextY < priv_->simBounds_.max.y - priv_->avoidanceDistanceHalf_) {
				// only push up in case the height map y is below the maximum y of boids
				boid.force.y += priv_->repulsionTimesSeparation_;
			}
		}
		if (nextY > priv_->simBounds_.max.y - priv_->avoidanceDistanceHalf_) {
			// Assuming the boid follows the lookAhead direction, it will eventually reach a point where
			// it is above the max y of the boid bounds. A simple approach is to push the boid to where it came from:
			boid.force.x -= nextVelocity.x * priv_->repulsionTimesSeparation_;
			boid.force.z -= nextVelocity.z * priv_->repulsionTimesSeparation_;
		}
	}

	return isCollisionFree;
}


namespace regen {
	std::ostream &operator<<(std::ostream &out, const BoidSimulation::ObjectType &mode) {
		switch (mode) {
			case BoidSimulation::ObjectType::ATTRACTOR:
				out << "attractor";
				break;
			case BoidSimulation::ObjectType::DANGER:
				out << "danger";
				break;
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BoidSimulation::ObjectType &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "attractor") mode = BoidSimulation::ObjectType::ATTRACTOR;
		else if (val == "danger") mode = BoidSimulation::ObjectType::DANGER;
		else {
			REGEN_WARN("Unknown Boid Object Type '" << val <<
													"'. Using default ATTRACTOR.");
			mode = BoidSimulation_CPU::ObjectType::ATTRACTOR;
		}
		return in;
	}
}


//////////////////////////
/////////////////////////

void BoidSimulation::loadSettings(LoadingContext &ctx, scene::SceneInputNode &input) {
	// set the bounds of the boids simulation
	if (input.hasAttribute("boids-area") && input.hasAttribute("boids-center")) {
		auto boidsArea = input.getValue<Vec3f>("boids-area", Vec3f(10.0f));
		auto boidsCenter = input.getValue<Vec3f>("boids-center", Vec3f(0.0f));
		Bounds<Vec3f> bounds(boidsCenter - boidsArea * 0.5f, boidsCenter + boidsArea * 0.5f);
		setSimulationBounds(bounds);
	} else {
		auto boidBounds = Bounds<Vec3f>(
				input.getValue<Vec3f>("bounds-min", Vec3f(-5.0f)),
				input.getValue<Vec3f>("bounds-max", Vec3f(5.0f))
		);
		setSimulationBounds(boidBounds);
	}

	if (input.hasAttribute("base-orientation")) {
		setBaseOrientation(input.getValue<float>("base-orientation", 0.0f));
	}
	if (input.hasAttribute("look-ahead")) {
		setLookAheadDistance(input.getValue<float>("look-ahead", 1.0f));
	}
	if (input.hasAttribute("repulsion")) {
		setRepulsionFactor(input.getValue<float>("repulsion", 1.0f));
	}
	if (input.hasAttribute("max-speed")) {
		setMaxBoidSpeed(input.getValue<float>("max-speed", 1.0f));
	}
	if (input.hasAttribute("max-neighbors")) {
		setMaxNumNeighbors(input.getValue<int>("max-neighbors", 100));
	}
	if (input.hasAttribute("visual-range")) {
		setVisualRange(input.getValue<float>("visual-range", 1.0f));
	}
	if (input.hasAttribute("coherence-weight")) {
		setCoherenceWeight(input.getValue<float>("coherence-weight", 0.5f));
	}
	if (input.hasAttribute("alignment-weight")) {
		setAlignmentWeight(input.getValue<float>("alignment-weight", 0.5f));
	}
	if (input.hasAttribute("avoidance-weight")) {
		setAvoidanceWeight(input.getValue<float>("avoidance-weight", 0.5f));
	}
	if (input.hasAttribute("avoidance-distance")) {
		setAvoidanceDistance(input.getValue<float>("avoidance-distance", 1.0f));
	}
	if (input.hasAttribute("separation-weight")) {
		setSeparationWeight(input.getValue<float>("separation-weight", 0.5f));
	}

	if (input.hasAttribute("height-map")) {
		auto heightMap = ctx.scene()->getResource<Texture2D>(input.getValue("height-map"));
		if (heightMap.get() != nullptr) {
			heightMap->ensureTextureData();
			auto heightScale = input.getValue<float>("height-map-factor", 1.0f);
			auto mapCenter = input.getValue<Vec3f>("map-center", Vec3f(0.0f));
			auto mapSize = input.getValue<Vec2f>("map-size", Vec2f(10.0f));
			setMap(mapCenter, mapSize, heightMap, heightScale);
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load height map textures.");
		}
	}

	for (auto &homePointNode: input.getChildren("home-point")) {
		addHomePoint(homePointNode->getValue<Vec3f>("value", Vec3f(0.0f)));
	}
	for (auto &objectNode: input.getChildren("object")) {
		auto objectType = objectNode->getValue<ObjectType>("type", ObjectType::ATTRACTOR);
		ref_ptr<ShaderInputMat4> entityTF;
		ref_ptr<ShaderInput3f> entityPos;
		if (objectNode->hasAttribute("tf")) {
			auto transformID = objectNode->getValue("tf");
			auto transform = ctx.scene()->getResource<ModelTransformation>(transformID);
			if (transform.get() != nullptr) {
				entityTF = transform->get();
			}
		} else if (objectNode->hasAttribute("point")) {
			entityTF = ref_ptr<ShaderInputMat4>::alloc("attractorPoint");
			entityTF->setUniformData(Mat4f::translationMatrix(
					objectNode->getValue<Vec3f>("point", Vec3f(0.0f))));
		} else if (objectNode->hasAttribute("value")) {
			auto attractorPosValue = objectNode->getValue<Vec3f>("value", Vec3f(0.0f));
			entityPos = ref_ptr<ShaderInput3f>::alloc("attractorPos");
			entityPos->setUniformData(attractorPosValue);
		} else {
			REGEN_WARN("No position or transform specified for boid object.");
			continue;
		}
		addObject(objectType, entityTF, entityPos);
	}

	initBoidSimulation();
}
