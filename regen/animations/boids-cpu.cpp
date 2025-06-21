#include "boids-cpu.h"
#include "regen/math/simd.h"

//#define REGEN_BOID_DEBUG_TIME
#define REGEN_USE_SIMD_GRID_UPDATE
#define REGEN_USE_SIMD_NEIGHBOR_UPDATE

using namespace regen;

// private data struct
struct BoidsCPU::Private {
	// Boid spatial grid. A cell in a 3D grid with edge length equal to boid visual range.
	struct Cell {
		vectorSIMD<int32_t> elements; // size = maxNumNeighbors
		uint32_t numElements = 0;     // (capped) number of boids in this cell
	};
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
	uint32_t gridStamp_ = 0u;
	Vec3f boidsScale_ = Vec3f::zero();
	Quaternion yawAdjust_;
	unsigned int maxNumNeighbors_ = 0;
	Bounds<Vec3f> simBounds_ = Bounds<Vec3f>(-10.0f, 10.0f);

	// some per-boid parameters used in simulation.
	// note: this is only ok in case of single-threaded simulation.
	Vec3f avgPosition_ = Vec3f::zero();
	Vec3f avgVelocity_ = Vec3f::zero();
	Vec3f separation_ = Vec3f::zero();
	Vec3f boidDirection_ = Vec3f::front();
	Quaternion boidRotation_;
};

BoidsCPU::BoidsCPU(const ref_ptr<ModelTransformation> &tf)
		: BoidSimulation(tf),
		  Animation(false, true),
		  priv_(new Private()) {
	boidData_.resize(numBoids_);
	boidPositionsX_.resize(numBoids_);
	boidPositionsY_.resize(numBoids_);
	boidPositionsZ_.resize(numBoids_);
	boidGridIndices_.resize(numBoids_, 0);
	//boidGridIndicesX_.resize(numBoids_, 0);
	//boidGridIndicesY_.resize(numBoids_, 0);
	//boidGridIndicesZ_.resize(numBoids_, 0);

	if (tf_->hasModelMat()) {
		auto tfData = tf_->modelMat()->mapClientData<Mat4f>(ShaderData::READ);
		for (uint32_t i = 0; i < numBoids_; ++i) {
			setBoidPosition(i, tfData.r[i].position());
			boidData_[i].velocity = Vec3f::zero();
		}
	} else {
		auto initialPositionData = tf_->modelOffset()->mapClientData<Vec3f>(ShaderData::READ);
		for (uint32_t i = 0; i < numBoids_; ++i) {
			auto &d = boidData_[i];
			setBoidPosition(i, initialPositionData.r[i]);
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

	// Initialize per-boid memory.
	for (uint32_t i = 0; i < numBoids_; ++i) {
		auto &d = boidData_[i];
		// note: an additional element is added to the end of the vector which is used
		//       to avoid branching in the SIMD code.
		d.neighbors.resize(maxNumNeighbors_->getVertex(0).r + 1);
		d.numNeighbors = 0;
	}
	// Initialize per-cell memory.
	for (auto &cell : priv_->grid_) {
		cell.elements.resize(maxNumNeighbors_->getVertex(0).r + 1); // +1 to avoid branching in the SIMD code
		cell.numElements = 0;
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

void BoidsCPU::setBoidPosition(uint32_t boidIndex, const Vec3f &pos) {
	boidPositionsX_[boidIndex] = pos.x;
	boidPositionsY_[boidIndex] = pos.y;
	boidPositionsZ_[boidIndex] = pos.z;
}

Vec3f BoidsCPU::getBoidPosition(uint32_t boidIndex) const {
	return {
			boidPositionsX_[boidIndex],
			boidPositionsY_[boidIndex],
			boidPositionsZ_[boidIndex]};
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
	// ensure the boid position is within the grid bounds
	return Vec3i::min(gridIndex,
					  priv_->gridSize_ - Vec3i::one());
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
#ifdef REGEN_BOID_DEBUG_TIME
	auto start = std::chrono::high_resolution_clock::now();
	simulateBoids(dt_f);
	auto afterSim = std::chrono::high_resolution_clock::now();
#else
	simulateBoids(dt_f);
#endif

	// update boids model transformation using the boids data
	updateTransforms();
#ifdef REGEN_BOID_DEBUG_TIME
	auto afterTF = std::chrono::high_resolution_clock::now();
#endif

	// resize the grid, and clear all cells
	clearGrid();
	if (priv_->grid_.empty()) { return; }

	// add boids to the grid and compute their neighborhood relations.
	updateGrid();

#ifdef REGEN_BOID_DEBUG_TIME
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
										   "simTime=" << std::fixed << std::setprecision(2)
										   << static_cast<float>(simAvg) / 1000.0f << "ms " <<
										   "copyTime=" << std::fixed << std::setprecision(2)
										   << static_cast<float>(copyAvg) / 1000.0f << "ms " <<
										   "gridTime=" << std::fixed << std::setprecision(2)
										   << static_cast<float>(gridAvg) / 1000.0f << "ms " <<
										   "totalTime=" << std::fixed << std::setprecision(2)
										   << static_cast<float>(simAvg + copyAvg + gridAvg) / 1000.0f << "ms");
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
			auto tfData = tfInput->mapClientData<Mat4f>(ShaderData::WRITE);
			float vl;

			for (uint32_t i = 0; i < numBoids_; ++i) {
				auto &d = boidData_[i];
				// calculate boid matrix, also need to compute rotation from velocity, model z
				//       should point in the direction of velocity.
				vl = d.velocity.length();
				if (vl > 0.001f) {
					priv_->boidRotation_.setLookRotation(d.velocity / vl);
				}
				auto &matrix = tfData.w[i];
				matrix = (priv_->yawAdjust_ * priv_->boidRotation_).calculateMatrix();
				matrix.scale(priv_->boidsScale_);
				matrix.x[12] += boidPositionsX_[i];
				matrix.x[13] += boidPositionsY_[i];
				matrix.x[14] += boidPositionsZ_[i];
			}
		} else if (tf_->hasModelOffset()) {
			auto positionData = tf_->modelOffset()->mapClientData<Vec4f>(ShaderData::WRITE);
			for (uint32_t i = 0; i < numBoids_; ++i) {
				auto &pos_w = positionData.w[i];
				pos_w.x = boidPositionsX_[i];
				pos_w.y = boidPositionsY_[i];
				pos_w.z = boidPositionsZ_[i];
			}
		}
	}
}

void BoidsCPU::clearGrid() {
	// update the grid based on gridBounds_, creating a cell every `2.0*(visual range)` in all directions.
	BoidSimulation::updateGridSize();

	if (priv_->gridStamp_ != gridSize_->stamp()) {
		priv_->gridStamp_ = gridSize_->stamp();
		auto gridSize = gridSize_->getVertex(0).r;
		priv_->gridSize_.x = static_cast<int>(gridSize.x);
		priv_->gridSize_.y = static_cast<int>(gridSize.y);
		priv_->gridSize_.z = static_cast<int>(gridSize.z);
		auto numCells = priv_->gridSize_.x * priv_->gridSize_.y * priv_->gridSize_.z;
		numCells = std::max(numCells, 0);
		auto firstAdded = priv_->grid_.size();
		priv_->grid_.resize(numCells);
		// reserve space for the neighbor indices in each cell.
		// NOTE: we limit to maxNumNeighbors_ to avoid excessive memory usage.
		auto maxNumNeighbors = maxNumNeighbors_->getVertex(0).r;
		for (uint32_t i = firstAdded; i < priv_->grid_.size(); ++i) {
			priv_->grid_[i].elements.resize(maxNumNeighbors + 1);
		}
	}
	if (priv_->grid_.empty()) { return; }

	// clear all cells, removing the old boids.
	for (auto &cell: priv_->grid_) {
		cell.numElements = 0;
	}
}

void BoidsCPU::updateGrid() {
	// iterate over all boids and add them to the grid, compute the index
	// based on the boid position and the grid bounds.
	int32_t startIdx = 0u;

#ifdef REGEN_USE_SIMD_GRID_UPDATE
	{
		Vec3fBatch boidBatch; // NOLINT(cppcoreguidelines-pro-type-member-init)
		Vec3fSIMD simd_gridMin(gridBounds_.min);
		Vec3iSIMD simd_gridSize(priv_->gridSize_ - Vec3i::one());
		Vec3iSIMD simd_gridSize_1_X_XY(Vec3i(
			1,
			priv_->gridSize_.x,
			priv_->gridSize_.x * priv_->gridSize_.y));
		floatSIMD simd_cellSize(priv_->cellSize_);
		floatSIMD simd_zero(0.0f);

		// we compute the grid index in batches
		for (; startIdx +  regen::simd::RegisterWidth <= static_cast<int32_t>(numBoids_);
			   startIdx += regen::simd::RegisterWidth) {
			// load the boid positions into a SIMD register
			boidBatch.load_aligned(
					boidPositionsX_.data() + startIdx,
					boidPositionsY_.data() + startIdx,
					boidPositionsZ_.data() + startIdx);
			// x = (x - gridBounds_.min) / cellSize
			boidBatch = (boidBatch - simd_gridMin) / simd_cellSize;
			// clamp to 0+
			boidBatch = boidBatch.max(simd_zero);

			// floor to integer grid indices
			Vec3iBatch gridIndices = boidBatch.floor();
			// clamp to max grid bounds
			gridIndices = gridIndices.min(simd_gridSize);
			// iy_f = iy * gridSize.x
			regen::simd::Register_i iy_f = regen::simd::mul_epi32(gridIndices.y, simd_gridSize_1_X_XY.y);
			// iz_f = iz * gridSize.x * gridSize.y;
			regen::simd::Register_i iz_f = regen::simd::mul_epi32(gridIndices.z, simd_gridSize_1_X_XY.z);
			// i_f = ix + iy_f + iz_f;
			regen::simd::Register_i i_f = regen::simd::add_epi32(regen::simd::add_epi32(gridIndices.x, iy_f), iz_f);
			// store results in local array
			//regen::simd::storeu_epi32(boidGridIndicesX_.data() + startIdx, gridIndices.x);
			//regen::simd::storeu_epi32(boidGridIndicesY_.data() + startIdx, gridIndices.y);
			//regen::simd::storeu_epi32(boidGridIndicesZ_.data() + startIdx, gridIndices.z);
			regen::simd::storeu_epi32(boidGridIndices_.data() + startIdx, i_f);
		}
	}
#endif // REGEN_USE_SIMD_GRID_UPDATE

	// Fallback to scalar loop for grid index computation
	for (; startIdx < static_cast<int32_t>(numBoids_);  ++startIdx) {
		Vec3f boidPos = getBoidPosition(startIdx);
		boidGridIndices_[startIdx] = getGridIndex(getGridIndex3D(boidPos), priv_->gridSize_);
	}

	for (int32_t boidIdx = 0; boidIdx < static_cast<int32_t>(numBoids_); ++boidIdx) {
		auto &boid = boidData_[boidIdx];
		auto &cell = priv_->grid_[boidGridIndices_[boidIdx]];
		auto boidPos = getBoidPosition(boidIdx);
		updateNeighbours(boid, boidPos, boidIdx,
			cell.elements, cell.numElements);
		// add the boid to the grid cell
		// Note: we can safely write one more element than the maxNumNeighbors_ because
		//       we reserve one additional element in the cell.elements vector.
		// TODO: Add to all cells where boid could be a neighbor? i.e. 8 cells instead of 1?
		//    OR as alternative if we stick to this, maybe increase the size of cells?
		cell.elements[cell.numElements] = boidIdx;
		cell.numElements += uint32_t(cell.numElements < priv_->maxNumNeighbors_);
	}
}

void BoidsCPU::updateNeighbours(
		BoidData &boid,
		const Vec3f &boidPos,
		int32_t boidIndex,
		const vectorSIMD<int32_t> &neighborIndices,
		uint32_t neighborCount) {
	size_t startIdx = 0;

#ifdef REGEN_USE_SIMD_NEIGHBOR_UPDATE
	if (neighborCount >= regen::simd::RegisterWidth) {
		Vec3fBatch neighborBatch; // NOLINT(cppcoreguidelines-pro-type-member-init)
		Vec3fSIMD boidPos_SIMD(boidPos);
		floatSIMD visualRangeSq_SIMD(priv_->visualRangeSq_);

		for (; startIdx + regen::simd::RegisterWidth <= neighborCount;
			   startIdx += regen::simd::RegisterWidth) {
			// load the indices of the neighbors into a SIMD register
			auto idx = regen::simd::loadu_si256(neighborIndices.data() + startIdx);
			// load the positions of the neighbors into a SIMD register
			neighborBatch.load(
					boidPositionsX_.data(),
					boidPositionsY_.data(),
					boidPositionsZ_.data(),
					idx);

			// finally compute the distance to the boid position for a batch of neighbors
			neighborBatch -= boidPos_SIMD;
			auto lengthSq = neighborBatch.lengthSquared();
			auto mask = regen::simd::cmp_lt(lengthSq, visualRangeSq_SIMD.c);
			// use the result as a bit mask to filter neighbors
			int maskBits = regen::simd::movemask_ps(mask);

			while (maskBits) {
				// find the first bit set in the mask, which indicates a neighbor in range
				int32_t batchLane = __builtin_ctz(maskBits);
				maskBits &= maskBits - 1; // clear lowest set bit

				int32_t neighborIndex = neighborIndices[startIdx + batchLane];
				auto &neighbor = boidData_[neighborIndex];

				// define reflexive neighbor relation
				boid.neighbors[boid.numNeighbors] = neighborIndex;
				boid.numNeighbors += uint32_t(boid.numNeighbors < priv_->maxNumNeighbors_);
				neighbor.neighbors[neighbor.numNeighbors] = boidIndex;
				neighbor.numNeighbors += uint32_t(neighbor.numNeighbors < priv_->maxNumNeighbors_);
			}
		}
	}
#endif // REGEN_USE_SIMD_NEIGHBOR_UPDATE

	// Fallback to scalar loop for remaining elements
	for (; startIdx < neighborCount &&
		   boid.numNeighbors < priv_->maxNumNeighbors_;
		   ++startIdx) {
		auto neighborIndex = neighborIndices[startIdx];
		auto &neighbor = boidData_[neighborIndex];

		// make distance check
		Vec3f dx = boidPos - getBoidPosition(neighborIndex);
		uint32_t isNeighbor(dx.lengthSquared() <= priv_->visualRangeSq_);

		// define reflexive neighbor relation
		boid.neighbors[boid.numNeighbors] = neighborIndex;
		boid.numNeighbors += isNeighbor;
		neighbor.neighbors[neighbor.numNeighbors] = boidIndex;
		neighbor.numNeighbors += isNeighbor*uint32_t(neighbor.numNeighbors < priv_->maxNumNeighbors_);
	}
}

void BoidsCPU::simulateBoids(float dt) {
	// reset bounds of the grid
	newBounds_.min = Vec3f::posMax();
	newBounds_.max = Vec3f::negMax();
	for (int32_t boidIdx = 0; boidIdx < static_cast<int32_t>(numBoids_); ++boidIdx) {
		simulateBoid(boidIdx, dt);
		// update the grid bounds
		newBounds_.min.x = std::min(newBounds_.min.x, boidPositionsX_[boidIdx]);
		newBounds_.min.y = std::min(newBounds_.min.y, boidPositionsY_[boidIdx]);
		newBounds_.min.z = std::min(newBounds_.min.z, boidPositionsZ_[boidIdx]);
		newBounds_.max.x = std::max(newBounds_.max.x, boidPositionsX_[boidIdx]);
		newBounds_.max.y = std::max(newBounds_.max.y, boidPositionsY_[boidIdx]);
		newBounds_.max.z = std::max(newBounds_.max.z, boidPositionsZ_[boidIdx]);
	}
	boidBounds_ = newBounds_;
}

void BoidsCPU::simulateBoid(int32_t boidIdx, float dt) {
	auto &boid = boidData_[boidIdx];
	Vec3f boidPos = getBoidPosition(boidIdx);

	// a boid is lost if it is outside the bounds
	bool isBoidLost = !priv_->simBounds_.contains(boidPos);
	if (boid.numNeighbors == 0) {
		// a boid without neighbors is lost
		isBoidLost = true;
		boid.force = Vec3f::zero();
	} else {
		// simulate the boid using the three rules of boids
		// TODO: We could compute per-boid force via SIMD operations.
		priv_->avgPosition_ = Vec3f::zero();
		priv_->avgVelocity_ = Vec3f::zero();
		priv_->separation_ = Vec3f::zero();
		for (uint32_t i=0; i<boid.numNeighbors; i++) {
			auto neighborIndex = boid.neighbors[i];
			auto &neighbor = boidData_[neighborIndex];
			auto neighborPos = getBoidPosition(neighborIndex);

			priv_->avgPosition_ += neighborPos;
			priv_->boidDirection_ = boidPos - neighborPos;
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
		priv_->avgPosition_ /= static_cast<float>(boid.numNeighbors);
		priv_->avgVelocity_ /= static_cast<float>(boid.numNeighbors);
		boid.force =
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

	auto dx = Vec3f(boid.velocity) * dt;
	boidPositionsX_[boidIdx] += dx.x;
	boidPositionsY_[boidIdx] += dx.y;
	boidPositionsZ_[boidIdx] += dx.z;
	auto lastDir = boid.velocity;
	lastDir.normalize();
	boid.velocity += boid.force * dt;
	limitVelocity(boid, lastDir);

	// clear the neighbors for the next frame
	boid.numNeighbors = 0;
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
	} else if (lookAhead.x > priv_->simBounds_.max.x - priv_->avoidanceDistance_) {
		boid.force.x -= priv_->repulsionTimesSeparation_;
	}
	if (lookAhead.y < priv_->simBounds_.min.y + priv_->avoidanceDistance_) {
		boid.force.y += priv_->repulsionTimesSeparation_;
	} else if (lookAhead.y > priv_->simBounds_.max.y - priv_->avoidanceDistance_) {
		boid.force.y -= priv_->repulsionTimesSeparation_;
	}
	if (lookAhead.z < priv_->simBounds_.min.z + priv_->avoidanceDistance_) {
		boid.force.z += priv_->repulsionTimesSeparation_;
	} else if (lookAhead.z > priv_->simBounds_.max.z - priv_->avoidanceDistance_) {
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

ref_ptr<BoidsCPU>
BoidsCPU::load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf) {
	auto boids = ref_ptr<BoidsCPU>::alloc(tf);
	boids->loadSettings(ctx, input);
	return boids;
}
