#include "boids-cpu.h"

#define BOID_DEBUG_TIME

using namespace regen;

// private data struct
struct BoidsCPU::Private {
	Vec3f avgPosition_ = Vec3f::zero();
	Vec3f avgVelocity_ = Vec3f::zero();
	Vec3f separation_ = Vec3f::zero();
	Vec3f boidDirection_ = Vec3f::front();
	Quaternion boidRotation_;

	// Boid spatial grid. A cell in a 3D grid with edge length equal to boid visual range.
	struct Cell { std::vector<uint32_t> elements; };
	std::vector<Cell> grid_;
	// Configuration parameters
	float visualRange_ = 0.0f;
	float visualRangeSq_ = 0.0f;
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
	Quaternion yawAdjust_;
	unsigned int maxNumNeighbors_ = 0;
	Bounds<Vec3f> simBounds_ = Bounds<Vec3f>(-10.0f, 10.0f);
};

BoidsCPU::BoidsCPU(const ref_ptr<ModelTransformation> &tf)
		: BoidSimulation(tf),
		  Animation(false, true),
		  priv_(new Private())
{
	boidData_.resize(numBoids_);
#ifdef REGEN_BOID_USE_SSE
	boidPositionsX_.resize(numBoids_);
	boidPositionsY_.resize(numBoids_);
	boidPositionsZ_.resize(numBoids_);
#else
	boidPositions_.resize(numBoids_);
#endif
	if (tf_->hasModelMat()) {
		auto tfData = tf_->modelMat()->mapClientData<Mat4f>(ShaderData::READ);
		for (uint32_t i = 0; i < numBoids_; ++i) {
			auto &pos = tfData.r[i].position();
#ifdef REGEN_BOID_USE_SSE
			boidPositionsX_[i] = pos.x;
			boidPositionsY_[i] = pos.y;
			boidPositionsZ_[i] = pos.z;
#else
			boidPositions_[i] = pos;
#endif
			boidData_[i].velocity = Vec3f::zero();
		}
	} else {
		auto initialPositionData = tf_->modelOffset()->mapClientData<Vec3f>(ShaderData::READ);
		for (uint32_t i = 0; i < numBoids_; ++i) {
			auto &d = boidData_[i];
			auto &pos = initialPositionData.r[i];
#ifdef REGEN_BOID_USE_SSE
			boidPositionsX_[i] = pos.x;
			boidPositionsY_[i] = pos.y;
			boidPositionsZ_[i] = pos.z;
#else
			boidPositions_[i] = initialPositionData.r[i];
#endif
			d.velocity = Vec3f::zero();
		}
	}
}

BoidsCPU::~BoidsCPU() {
	delete priv_;
}

void BoidsCPU::initBoidSimulation() {
	setAnimationName("boids");
	// use a dedicated thread for the boids simulation which is not synchronized with the graphics thread,
	// i.e. it can be slower or faster than the graphics thread.
	setSynchronized(false);
	priv_->baseOrientation_ = baseOrientation_->getVertex(0).r;
	priv_->yawAdjust_.setAxisAngle(Vec3f(0, 1, 0), priv_->baseOrientation_);
	priv_->boidsScale_ = boidsScale_->getVertex(0).r;

	for (uint32_t i = 0; i < numBoids_; ++i) {
		auto &d = boidData_[i];
		d.neighbors.reserve(maxNumNeighbors_->getVertex(0).r);
	}

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
	REGEN_INFO("CPU Boids simulation with " << numBoids_ << " boids");
}

ref_ptr<BoidsCPU> BoidsCPU::load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf) {
	auto boids = ref_ptr<BoidsCPU>::alloc(tf);
	boids->loadSettings(ctx, input);
	return boids;
}

void BoidsCPU::animate(double dt) {
	auto dt_f = static_cast<float>(dt) * 0.001f;
	priv_->simBounds_.min = simulationBoundsMin_->getVertex(0).r;
	priv_->simBounds_.max = simulationBoundsMax_->getVertex(0).r;
	priv_->avoidanceDistance_ = avoidanceDistance_->getVertex(0).r;
	priv_->avoidanceDistanceHalf_ = priv_->avoidanceDistance_ * 0.5f;
	priv_->repulsionTimesSeparation_ = repulsionFactor_->getVertex(0).r * separationWeight_->getVertex(0).r;
	priv_->visualRange_ = visualRange_->getVertex(0).r;
	priv_->visualRangeSq_ = priv_->visualRange_ * priv_->visualRange_;
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
	uint32_t gridIndex = 0u;
	// resize the grid, and clear all cells
	updateGrid();
	if (priv_->grid_.empty()) { return; }
	// iterate over all boids and add them to the grid, compute the index
	// based on the boid position and the grid bounds.
	for (uint32_t i = 0; i < numBoids_; ++i) {
		auto &boid = boidData_[i];
#ifdef REGEN_BOID_USE_SSE
		// TODO: optimize this, use SIMD operations
		Vec3f boidPos(
			boidPositionsX_[i],
			boidPositionsY_[i],
			boidPositionsZ_[i]);
#else
		auto &boidPos = boidPositions_[i];
#endif

		boid.gridIndex = getGridIndex3D(boidPos);
		gridIndex = getGridIndex(boid.gridIndex, priv_->gridSize_);
		priv_->grid_[gridIndex].elements.push_back(i);
		updateNeighbours0(boid, boidPos, i);
	}
#ifdef BOID_DEBUG_TIME
	static std::vector<long> simTimes;
	static std::vector<long> copyTimes;
	static std::vector<long> gridTimes;
	auto afterGrid = std::chrono::high_resolution_clock::now();
	auto simTime = std::chrono::duration_cast<std::chrono::microseconds>(afterSim - start).count();
	auto copyTime = std::chrono::duration_cast<std::chrono::microseconds>(afterTF - afterSim).count();
	auto gridTime = std::chrono::duration_cast<std::chrono::microseconds>(afterGrid - afterTF).count();
	simTimes.push_back(simTime);
	copyTimes.push_back(copyTime);
	gridTimes.push_back(gridTime);
	if (simTimes.size() > 100) {
		// print the average time for the last 100 frames
		long simAvg = 0;
		long copyAvg = 0;
		long gridAvg = 0;
		for (size_t i = 0; i < simTimes.size(); ++i) {
			simAvg += simTimes[i];
			copyAvg += copyTimes[i];
			gridAvg += gridTimes[i];
		}
		simAvg /= static_cast<long>(simTimes.size());
		copyAvg /= static_cast<long>(copyTimes.size());
		gridAvg /= static_cast<long>(gridTimes.size());
		REGEN_INFO("BoidsSimulation_CPU: " <<
			"simTime="   << std::fixed << std::setprecision(2) << static_cast<float>(simAvg)/1000.0f << "ms " <<
			"copyTime="  << std::fixed << std::setprecision(2) << static_cast<float>(copyAvg)/1000.0f << "ms " <<
			"gridTime="  << std::fixed << std::setprecision(2) << static_cast<float>(gridAvg)/1000.0f << "ms " <<
			"totalTime=" << std::fixed << std::setprecision(2) << static_cast<float>(simAvg + copyAvg + gridAvg)/1000.0f << "ms");
		simTimes.clear();
		copyTimes.clear();
		gridTimes.clear();
	}
#endif
}

void BoidsCPU::updateTransforms() {
	if (tf_.get()) {
		if (tf_->hasModelMat()) {
			auto &tfInput = tf_->modelMat();
			auto tfData = tfInput->mapClientData<Mat4f>(ShaderData::READ | ShaderData::WRITE);
			float vl;

			for (uint32_t i = 0; i < numBoids_; ++i) {
				auto &d = boidData_[i];
				// calculate boid matrix, also need to compute rotation from velocity, model z
				//       should point in the direction of velocity.
				vl = d.velocity.length();
				if (vl > 0.001f) {
					priv_->boidRotation_.setLookRotation(d.velocity / vl);
					auto &matrix = tfData.w[i];
					matrix = (priv_->yawAdjust_ * priv_->boidRotation_).calculateMatrix();
					matrix.scale(priv_->boidsScale_);
#ifdef REGEN_BOID_USE_SSE
					matrix.x[12] += boidPositionsX_[i];
					matrix.x[13] += boidPositionsY_[i];
					matrix.x[14] += boidPositionsZ_[i];
#else
					tfData.w[i].translate(boidPositions_[i]);
#endif
				} else {
					tfData.w[i] = tfData.r[i];
				}
			}
		}
		else if (tf_->hasModelOffset()) {
			auto positionData = tf_->modelOffset()->mapClientData<Vec4f>(ShaderData::WRITE);
			for (uint32_t i = 0; i < numBoids_; ++i) {
				auto &pos_w = positionData.w[i];
#ifdef REGEN_BOID_USE_SSE
				pos_w.x = boidPositionsX_[i];
				pos_w.y = boidPositionsY_[i];
				pos_w.z = boidPositionsZ_[i];
#else
				pos_w = boidPositions_[i];
#endif
			}
		}
	}
}

void BoidsCPU::simulateBoids(float dt) {
	// reset bounds of the grid
	newBounds_.min = Vec3f::posMax();
	newBounds_.max = Vec3f::negMax();
	for (uint32_t i = 0; i < numBoids_; ++i) {
		simulateBoid(i, dt);
		// update the grid bounds
#ifdef REGEN_BOID_USE_SSE
		// TODO: optimize this, use SIMD operations
		newBounds_.min.x = std::min(newBounds_.min.x, boidPositionsX_[i]);
		newBounds_.min.y = std::min(newBounds_.min.y, boidPositionsY_[i]);
		newBounds_.min.z = std::min(newBounds_.min.z, boidPositionsZ_[i]);
		newBounds_.max.x = std::max(newBounds_.max.x, boidPositionsX_[i]);
		newBounds_.max.y = std::max(newBounds_.max.y, boidPositionsY_[i]);
		newBounds_.max.z = std::max(newBounds_.max.z, boidPositionsZ_[i]);
#else
		auto &boidPos = boidPositions_[i];
		newBounds_.min.setMin(boidPos);
		newBounds_.max.setMax(boidPos);
#endif
	}
	boidBounds_ = newBounds_;
}

Vec3i BoidsCPU::getGridIndex3D(const Vec3f &x) const {
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

void BoidsCPU::updateNeighbours0(BoidData &boid, const Vec3f &boidPos, uint32_t boidIndex) {
	// find neighbours in the grid based on the current position of the boid
	boid.neighbors.clear();
	// find neighbours in the cell of the boid
	updateNeighbours2(boid, boidPos, boidIndex, boid.gridIndex);
	// find neighbours in the adjacent cells.
	// however we only need to consider one direction in each dimension
	// as the grid cell size is equal to the visual range times two.
	auto cellPosition = getCellCenter(boid.gridIndex);
	Vec3i dir, gridIdx;
	dir.x = (boidPos.x < cellPosition.x ? -1 : 1);
	dir.y = (boidPos.y < cellPosition.y ? -1 : 1);
	dir.z = (boidPos.z < cellPosition.z ? -1 : 1);
	gridIdx = Vec3i(dir.x, 0, 0) + boid.gridIndex;
	updateNeighbours1(boid, boidPos, boidIndex, gridIdx);
	gridIdx = Vec3i(0, dir.y, 0) + boid.gridIndex;
	updateNeighbours1(boid, boidPos, boidIndex, gridIdx);
	gridIdx = Vec3i(0, 0, dir.z) + boid.gridIndex;
	updateNeighbours1(boid, boidPos, boidIndex, gridIdx);
	gridIdx = Vec3i(dir.x, dir.y, 0) + boid.gridIndex;
	updateNeighbours1(boid, boidPos, boidIndex, gridIdx);
	gridIdx = Vec3i(dir.x, 0, dir.z) + boid.gridIndex;
	updateNeighbours1(boid, boidPos, boidIndex, gridIdx);
	gridIdx = Vec3i(0, dir.y, dir.z) + boid.gridIndex;
	updateNeighbours1(boid, boidPos, boidIndex, gridIdx);
	gridIdx = Vec3i(dir.x, dir.y, dir.z) + boid.gridIndex;
	updateNeighbours1(boid, boidPos, boidIndex, gridIdx);
}

void BoidsCPU::updateNeighbours1(BoidData &boid,
								 const Vec3f &boidPos, uint32_t boidIndex, const Vec3i &gridIndex) {
	if (gridIndex.x < 0 ||
	    gridIndex.y < 0 ||
	    gridIndex.z < 0) { return; }
	updateNeighbours2(boid, boidPos, boidIndex, gridIndex);
}

void BoidsCPU::updateNeighbours2(BoidData &boid,
								 const Vec3f &boidPos, uint32_t boidIndex, const Vec3i &gridIndex) {
	if (boid.neighbors.size() >= priv_->maxNumNeighbors_) { return; }
	if (gridIndex.x >= priv_->gridSize_.x ||
	    gridIndex.y >= priv_->gridSize_.y ||
	    gridIndex.z >= priv_->gridSize_.z) { return; }
	auto index = getGridIndex(gridIndex, priv_->gridSize_);

	for (auto neighborIndex: priv_->grid_[index].elements) {
		auto &neighbor = boidData_[neighborIndex];
		if (&neighbor == &boid) { continue; }

#ifdef REGEN_BOID_USE_SSE
		// TODO: optimize this, use SIMD operations
		Vec3f neighborPos(
			boidPositionsX_[neighborIndex],
			boidPositionsY_[neighborIndex],
			boidPositionsZ_[neighborIndex]);
#else
		auto &neighborPos = boidPositions_[neighborIndex];
#endif
		if ((boidPos - neighborPos).lengthSquared() > priv_->visualRangeSq_) {
			continue;
		}
		boid.neighbors.push_back(neighborIndex);
		if (neighbor.neighbors.size() < priv_->maxNumNeighbors_) {
			neighbor.neighbors.push_back(boidIndex);
		}
		if (boid.neighbors.size() >= priv_->maxNumNeighbors_) {
			break;
		}
	}
}

void BoidsCPU::updateGrid() {
	// update the grid based on gridBounds_, creating a cell every `2.0*(visual range)` in all directions.
	updateGridSize();
	auto gridSize = gridSize_->getVertex(0).r;
	priv_->gridSize_.x = static_cast<int>(gridSize.x);
	priv_->gridSize_.y = static_cast<int>(gridSize.y);
	priv_->gridSize_.z = static_cast<int>(gridSize.z);
	auto numCells = priv_->gridSize_.x * priv_->gridSize_.y * priv_->gridSize_.z;
	numCells = std::max(numCells, 0);
	priv_->grid_.resize(numCells);
	if (numCells==0) { return; }
	// clear all cells, removing the old boids.
	for (auto &cell: priv_->grid_) {
		cell.elements.clear();
	}
}

void BoidsCPU::simulateBoid(uint32_t boidIdx, float dt) {
	auto &boid = boidData_[boidIdx];
#ifdef REGEN_BOID_USE_SSE
	// TODO: use SSE here too?
	Vec3f boidPos(
		boidPositionsX_[boidIdx],
		boidPositionsY_[boidIdx],
		boidPositionsZ_[boidIdx]);
#else
	auto &boidPos = boidPositions_[boidIdx];
#endif

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
#ifdef REGEN_BOID_USE_SSE
			// TODO: use SSE here too?
			auto &neighborPosX = boidPositionsX_[neighborIndex];
			auto &neighborPosY = boidPositionsY_[neighborIndex];
			auto &neighborPosZ = boidPositionsZ_[neighborIndex];
			priv_->avgPosition_.x += neighborPosX;
			priv_->avgPosition_.y += neighborPosY;
			priv_->avgPosition_.z += neighborPosZ;
			priv_->boidDirection_.x = boidPos.x - neighborPosX;
			priv_->boidDirection_.y = boidPos.y - neighborPosY;
			priv_->boidDirection_.z = boidPos.z - neighborPosZ;
#else
			auto &neighborPos = boidPositions_[neighborIndex];
			priv_->avgPosition_ += neighborPos;
			priv_->boidDirection_ = boidPos - neighborPos;
#endif
			priv_->avgVelocity_ += neighbor.velocity;
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

#ifdef REGEN_BOID_USE_SSE
	auto dx = Vec3f(boid.velocity) * dt;
	boidPositionsX_[boidIdx] += dx.x;
	boidPositionsY_[boidIdx] += dx.y;
	boidPositionsZ_[boidIdx] += dx.z;
#else
	boidPos += Vec3f(boid.velocity) * dt;
#endif
	auto lastDir = boid.velocity;
	lastDir.normalize();
	boid.velocity += boid.force * dt;
	limitVelocity(boid, lastDir);
}

void BoidsCPU::limitVelocity(BoidData &boid, const Vec3f &lastDir) {
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

void BoidsCPU::homesickness(BoidData &boid, const Vec3f &boidPos) {
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

bool BoidsCPU::avoidDanger(BoidData &boid, const Vec3f &boidPos) {
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

void BoidsCPU::attract(BoidData &boid, const Vec3f &boidPos) {
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

bool BoidsCPU::avoidCollisions(BoidData &boid, const Vec3f &boidPos, float dt) {
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
