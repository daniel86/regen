#include "boids-cpu.h"
#include "regen/math/simd.h"
#include "regen/utility/aligned-array.h"

//#define REGEN_BOID_DEBUG_TIME
//#define REGEN_BOID_USE_SORTED_GRID
#define REGEN_BOID_USE_GRID_SIMD
#define REGEN_BOID_USE_NEIGHBOR_SIMD
#define REGEN_BOID_USE_FORCE_SIMD

using namespace regen;

// private data struct
struct BoidsCPU::Private {
	// Note: SoA data layout for SIMD-friendly processing
	// Note: vectorSIMD is used to ensure 32-bit alignment which is good for SIMD operations.
#ifdef REGEN_BOID_USE_SORTED_GRID
	AlignedArray<int32_t> sortedGridIndices_; // size = numBoids_
	AlignedArray<int32_t> cellCounts_;  // size = numCells
	AlignedArray<int32_t> cellOffsets_; // size = numCells
#else
	// Boid spatial grid. A cell in a 3D grid with edge length equal to boid visual range.
	struct Cell {
		vectorSIMD<int32_t> elements; // size = maxNumNeighbors
		uint32_t numElements = 0;     // (capped) number of boids in this cell
	};
	std::vector<Cell> grid_;
#endif
	// Per-boid data
	AlignedArray<int32_t> boidGridIndex_;   // size = numBoids_
	AlignedArray<float>   boidPositionsX_;  // size = numBoids_
	AlignedArray<float>   boidPositionsY_;  // size = numBoids_
	AlignedArray<float>   boidPositionsZ_;  // size = numBoids_
	AlignedArray<float>   boidVelocityX_;   // size = numBoids_
	AlignedArray<float>   boidVelocityY_;   // size = numBoids_
	AlignedArray<float>   boidVelocityZ_;   // size = numBoids_
	AlignedArray<float>   boidOrientW_;   // size = numBoids_
	AlignedArray<float>   boidOrientX_;   // size = numBoids_
	AlignedArray<float>   boidOrientY_;   // size = numBoids_
	AlignedArray<float>   boidOrientZ_;   // size = numBoids_

	// Configuration parameters
	float visualRange_ = 1.6f;
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
	float cellSize_ = 3.2f;
	Vec3i gridSize_ = Vec3i::zero();
	uint32_t gridStamp_ = 0u;
	Vec3f boidsScale_ = Vec3f::zero();
	Mat4f tmpMat_ = Mat4f::identity();
	Quaternion yawAdjust_;
	unsigned int maxNumNeighbors_ = 0;
	Bounds<Vec3f> simBounds_ = Bounds<Vec3f>(-10.0f, 10.0f);

	// some per-boid parameters used in simulation.
	// note: this is only ok in case of single-threaded simulation.
	Vec3f boidDirection_ = Vec3f::front();
	Quaternion boidRotation_;

	explicit Private(uint32_t numBoids) :
#ifdef REGEN_BOID_USE_SORTED_GRID
			  sortedGridIndices_(numBoids),
#endif
			  boidGridIndex_(numBoids),
			  boidPositionsX_(numBoids),
			  boidPositionsY_(numBoids),
			  boidPositionsZ_(numBoids),
			  boidVelocityX_(numBoids),
			  boidVelocityY_(numBoids),
			  boidVelocityZ_(numBoids),
			  boidOrientW_(numBoids),
			  boidOrientX_(numBoids),
			  boidOrientY_(numBoids),
			  boidOrientZ_(numBoids)
			  {}
};

BoidsCPU::BoidsCPU(const ref_ptr<ModelTransformation> &tf)
		: BoidSimulation(tf),
		  Animation(false, true),
		  priv_(new Private(numBoids_)) {
	boidData_.resize(numBoids_);
	priv_->boidVelocityX_.setToZero();
	priv_->boidVelocityY_.setToZero();
	priv_->boidVelocityZ_.setToZero();
	priv_->boidGridIndex_.setToZero();
#ifdef REGEN_BOID_USE_SORTED_GRID
	priv_->sortedGridIndices_.setToZero();
#endif
	for (uint32_t i = 0; i < numBoids_; ++i) {
		setBoidPosition(i, tf_->position(i).r);
	}
}

BoidsCPU::BoidsCPU(const ref_ptr<ShaderInput4f> &modelOffset)
		: BoidSimulation(modelOffset),
		  Animation(false, true),
		  priv_(new Private(numBoids_)) {
	boidData_.resize(numBoids_);
	priv_->boidVelocityX_.setToZero();
	priv_->boidVelocityY_.setToZero();
	priv_->boidVelocityZ_.setToZero();
	priv_->boidGridIndex_.setToZero();
#ifdef REGEN_BOID_USE_SORTED_GRID
	priv_->sortedGridIndices_.setToZero();
#endif
	for (uint32_t i = 0; i < numBoids_; ++i) {
		setBoidPosition(i, modelOffset->getVertex(i).r.xyz_());
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

	// Initialize grid memory.
#ifdef REGEN_BOID_USE_SORTED_GRID
	auto numCells = priv_->gridSize_.x * priv_->gridSize_.y * priv_->gridSize_.z;
	numCells = std::max(numCells, 0);
	priv_->cellCounts_.resize(numCells);
	priv_->cellOffsets_.resize(numCells);
#else
	for (auto &cell : priv_->grid_) {
		cell.elements.resize(maxNumNeighbors_->getVertex(0).r + 1); // +1 to avoid branching
		cell.numElements = 0;
	}
#endif

	// Initialize per-boid memory.
	for (uint32_t i = 0; i < numBoids_; ++i) {
		auto &d = boidData_[i];
		// note: an additional element is added to the end of the vector which is used
		//       to avoid branching.
		d.neighbors.resize(maxNumNeighbors_->getVertex(0).r + 1);
		d.numNeighbors = 0;
	}

	animationState()->setInput(coherenceWeight_);
	animationState()->setInput(alignmentWeight_);
	animationState()->setInput(separationWeight_);
	animationState()->setInput(avoidanceWeight_);
	animationState()->setInput(avoidanceDistance_);
	animationState()->setInput(visualRange_);
	animationState()->setInput(lookAheadDistance_);
	animationState()->setInput(repulsionFactor_);
	animationState()->setInput(maxNumNeighbors_);
	animationState()->setInput(maxBoidSpeed_);
	animationState()->setInput(maxAngularSpeed_);
	REGEN_INFO("CPU Boids simulation with " << numBoids_ << " boids");
}

void BoidsCPU::setBoidPosition(uint32_t boidIndex, const Vec3f &pos) {
	priv_->boidPositionsX_[boidIndex] = pos.x;
	priv_->boidPositionsY_[boidIndex] = pos.y;
	priv_->boidPositionsZ_[boidIndex] = pos.z;
}

Vec3f BoidsCPU::getBoidPosition(uint32_t boidIndex) const {
	return {
			priv_->boidPositionsX_[boidIndex],
			priv_->boidPositionsY_[boidIndex],
			priv_->boidPositionsZ_[boidIndex]};
}

void BoidsCPU::setBoidVelocity(uint32_t boidIndex, const Vec3f &vel) {
	priv_->boidVelocityX_[boidIndex] = vel.x;
	priv_->boidVelocityY_[boidIndex] = vel.y;
	priv_->boidVelocityZ_[boidIndex] = vel.z;
}

Vec3f BoidsCPU::getBoidVelocity(uint32_t boidIndex) const {
	return {
			priv_->boidVelocityX_[boidIndex],
			priv_->boidVelocityY_[boidIndex],
			priv_->boidVelocityZ_[boidIndex]};
}

void BoidsCPU::setBoidOrientation(
		uint32_t boidIndex, const Quaternion &orientation) {
	priv_->boidOrientW_[boidIndex] = orientation.w;
	priv_->boidOrientX_[boidIndex] = orientation.x;
	priv_->boidOrientY_[boidIndex] = orientation.y;
	priv_->boidOrientZ_[boidIndex] = orientation.z;
}

Quaternion BoidsCPU::getBoidOrientation(uint32_t boidIndex) const {
	return {
			priv_->boidOrientW_[boidIndex],
			priv_->boidOrientX_[boidIndex],
			priv_->boidOrientY_[boidIndex],
			priv_->boidOrientZ_[boidIndex]};
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
#ifndef REGEN_BOID_USE_SORTED_GRID
	if (priv_->grid_.empty()) { return; }
#endif

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
										   "sim=" << std::fixed << std::setprecision(2)
										   << static_cast<float>(simAvg) / 1000.0f << "ms " <<
										   "copy=" << std::fixed << std::setprecision(2)
										   << static_cast<float>(copyAvg) / 1000.0f << "ms " <<
										   "grid=" << std::fixed << std::setprecision(2)
										   << static_cast<float>(gridAvg) / 1000.0f << "ms " <<
										   "total=" << std::fixed << std::setprecision(2)
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
			auto m_matData = tf_->modelMat()->mapClientData<Mat4f>(BUFFER_GPU_WRITE);
			for (uint32_t i = 0; i < numBoids_; ++i) {
				Quaternion orientation(
					priv_->boidOrientW_[i],
					priv_->boidOrientX_[i],
					priv_->boidOrientY_[i],
					priv_->boidOrientZ_[i]);
				m_matData.w[i] = (priv_->yawAdjust_ * orientation).calculateMatrix();
				m_matData.w[i].scale(priv_->boidsScale_);
				m_matData.w[i].x[12] += priv_->boidPositionsX_[i];
				m_matData.w[i].x[13] += priv_->boidPositionsY_[i];
				m_matData.w[i].x[14] += priv_->boidPositionsZ_[i];
			}
		} else if (tf_->hasModelOffset()) {
			auto m_offsetData = tf_->modelOffset()->mapClientData<Vec4f>(BUFFER_GPU_WRITE);
			for (uint32_t i = 0; i < numBoids_; ++i) {
				m_offsetData.w[i] = Vec4f(
					priv_->boidPositionsX_[i],
					priv_->boidPositionsY_[i],
					priv_->boidPositionsZ_[i],
					1.0f);
			}
		}
	} else if (modelOffset_.get()) {
		// update the model offset data
		auto offsetData = modelOffset_->mapClientData<Vec4f>(
				BUFFER_GPU_WRITE, 0, numBoids_ * sizeof(Vec4f));
		for (uint32_t i = 0; i < numBoids_; ++i) {
			offsetData.w[i] = Vec4f(
					priv_->boidPositionsX_[i],
					priv_->boidPositionsY_[i],
					priv_->boidPositionsZ_[i],
					1.0f);
		}
	}
}

void BoidsCPU::clearGrid() {
	// update the grid based on gridBounds_, creating a cell every `2.0*(visual range)` in all directions.
	BoidSimulation::updateGridSize();

	if (priv_->gridStamp_ != gridSize_->stampOfReadData()) {
		priv_->gridStamp_ = gridSize_->stampOfReadData();
		auto gridSize = gridSize_->getVertex(0).r;
		priv_->gridSize_.x = static_cast<int>(gridSize.x);
		priv_->gridSize_.y = static_cast<int>(gridSize.y);
		priv_->gridSize_.z = static_cast<int>(gridSize.z);
		auto numCells = priv_->gridSize_.x * priv_->gridSize_.y * priv_->gridSize_.z;
		numCells = std::max(numCells, 0);
#ifdef REGEN_BOID_USE_SORTED_GRID
		priv_->cellCounts_.resize(numCells);
		priv_->cellOffsets_.resize(numCells);
		priv_->cellOffsets_[0] = 0;
#else
		auto firstAdded = priv_->grid_.size();
		priv_->grid_.resize(numCells);
		// reserve space for the neighbor indices in each cell.
		// NOTE: we limit to maxNumNeighbors_ to avoid excessive memory usage.
		auto maxNumNeighbors = maxNumNeighbors_->getVertex(0).r;
		for (uint32_t i = firstAdded; i < priv_->grid_.size(); ++i) {
			priv_->grid_[i].elements.resize(maxNumNeighbors + 1);
		}
#endif
	}

#ifndef REGEN_BOID_USE_SORTED_GRID
	if (priv_->grid_.empty()) { return; }

	// clear all cells, removing the old boids.
	for (auto &cell: priv_->grid_) {
		cell.numElements = 0;
	}
#endif
}

#ifdef REGEN_BOID_USE_SORTED_GRID
void zeroAligned(AlignedArray<int32_t> &vec) {
    size_t size = vec.size();
    int* data = vec.data();
    size_t i = 0;
    // Use AVX2 to set 8 ints (32 bytes) at a time
    // TODO: use simd abstraction API
    __m256i zero = _mm256_setzero_si256();
    for (; i + 8 <= size; i += 8) {
        _mm256_store_si256(reinterpret_cast<__m256i*>(data + i), zero);
    }
    // Tail loop (in case size is not divisible by 8)
    for (; i < size; ++i) { data[i] = 0; }
}
#endif

void BoidsCPU::updateGrid() {
	// iterate over all boids and add them to the grid, compute the index
	// based on the boid position and the grid bounds.
	int32_t startIdx = 0u;

#ifdef REGEN_BOID_USE_GRID_SIMD
	{
		BatchOf_Vec3f boidBatch; // NOLINT(cppcoreguidelines-pro-type-member-init)
		BatchOf_Vec3f simd_gridMin(gridBounds_.min);
		BatchOf_Vec3i simd_gridSize(priv_->gridSize_ - Vec3i::one());
		BatchOf_Vec3i simd_gridSize_1_X_XY(Vec3i(
			1,
			priv_->gridSize_.x,
			priv_->gridSize_.x * priv_->gridSize_.y));
		BatchOf_float simd_cellSize(priv_->cellSize_);
		BatchOf_float simd_zero(0.0f);

		// we compute the grid index in batches
		for (; startIdx +  regen::simd::RegisterWidth <= static_cast<int32_t>(numBoids_);
			   startIdx += regen::simd::RegisterWidth) {
			// load the boid positions into a SIMD register
			boidBatch.load_aligned(
					priv_->boidPositionsX_.data() + startIdx,
					priv_->boidPositionsY_.data() + startIdx,
					priv_->boidPositionsZ_.data() + startIdx);
			// x = (x - gridBounds_.min) / cellSize
			boidBatch = (boidBatch - simd_gridMin) / simd_cellSize;
			// clamp to 0+
			boidBatch = boidBatch.max(simd_zero);

			// floor to integer grid indices
			BatchOf_Vec3i gridIndices = boidBatch.floor();
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
			regen::simd::storeu_epi32(priv_->boidGridIndex_.data() + startIdx, i_f);
		}
	}
#endif // REGEN_USE_SIMD_GRID_UPDATE

	// Fallback to scalar loop for grid index computation
	for (; startIdx < static_cast<int32_t>(numBoids_);  ++startIdx) {
		const Vec3f boidPos = getBoidPosition(startIdx);
		const Vec3i idx3D = getGridIndex3D(boidPos);
		priv_->boidGridIndex_[startIdx] = getGridIndex(idx3D, priv_->gridSize_);
	}

#ifdef REGEN_BOID_USE_SORTED_GRID
	const int32_t numCells = priv_->cellOffsets_.size();
	// Reset the cell counts to zero
	priv_->cellOffsets_[0] = 0;
	zeroAligned(priv_->cellCounts_);
	// Count how many boids are in each cell
	for (uint32_t i = 0; i < numBoids_; ++i) {
		priv_->cellCounts_[priv_->boidGridIndex_[i]] += 1;
	}
	// Compute prefix sum to get starting offsets
	for (int32_t i = 1; i < numCells; ++i) {
		priv_->cellOffsets_[i] = priv_->cellOffsets_[i - 1] + priv_->cellCounts_[i - 1];
	}

	// Write sorted indices to buffer
	for (int32_t boidIdx = 0; boidIdx < static_cast<int32_t>(numBoids_); ++boidIdx) {
		// cell of i'th boid
		int32_t cell_idx = priv_->boidGridIndex_[boidIdx];
		// the start of the cell in the sorted grid indices
		int32_t cell_offset = priv_->cellOffsets_[cell_idx];
		priv_->cellOffsets_[cell_idx] += 1;
		priv_->sortedGridIndices_[cell_offset] = boidIdx;
	}
	// Finally iterate over the grid cells and update neighbors
	int32_t* cellElements = priv_->sortedGridIndices_.data();

	for (int32_t cellIdx = 0; cellIdx < numCells; ++cellIdx) {
		int32_t count = priv_->cellCounts_[cellIdx];
		if (count == 0) continue; // skip empty cells

		for (int32_t i= 0; i + 1 < count; ++i) {
			int32_t boidIdx = cellElements[i];
			auto &boid = boidData_[boidIdx];
			auto boidPos = getBoidPosition(boidIdx);
			updateNeighbours(boid, boidPos, boidIdx,
				cellElements + i + 1,
				count - i - 1);
		}

		cellElements += count;
	}

#else
	for (int32_t boidIdx = 0; boidIdx < static_cast<int32_t>(numBoids_); ++boidIdx) {
		auto &boid = boidData_[boidIdx];
		const uint32_t cellIndex = priv_->boidGridIndex_[boidIdx];
		auto &cell = priv_->grid_[cellIndex];
		auto boidPos = getBoidPosition(boidIdx);
		updateNeighbours(boid, boidPos, boidIdx,
			cell.elements.data(), cell.numElements);
		// add the boid to the grid cell
		// Note: we can safely write one more element than the maxNumNeighbors_ because
		//       we reserve one additional element in the cell.elements vector.
		// Note: this is not entirely accurate. Better would be to also check the
		//       adjacent cells, but this would cost more performance and results are ok in my opinion.
		cell.elements[cell.numElements] = boidIdx;
		cell.numElements += uint32_t(cell.numElements < priv_->maxNumNeighbors_);
	}
#endif
}

void BoidsCPU::updateNeighbours(
		BoidData &boid,
		const Vec3f &boidPos,
		int32_t boidIndex,
		const int32_t *neighborIndices,
		uint32_t neighborCount) {
	size_t startIdx = 0;

#ifdef REGEN_BOID_USE_NEIGHBOR_SIMD
	if (neighborCount >= regen::simd::RegisterWidth) {
		// NOTE: unfortunately, this does not buy us much as num neighbors is usually capped
		//       to rather small values, e.g. 100.
		BatchOf_Vec3f neighborBatch; // NOLINT(cppcoreguidelines-pro-type-member-init)
		BatchOf_Vec3f boidPos_SIMD(boidPos);
		BatchOf_float visualRangeSq_SIMD(priv_->visualRangeSq_);

		for (; startIdx + regen::simd::RegisterWidth <= neighborCount;
			   startIdx += regen::simd::RegisterWidth) {
			// load the indices of the neighbors into a SIMD register
			auto idx = regen::simd::loadu_si256(neighborIndices + startIdx);
			// load the positions of the neighbors into a SIMD register
			neighborBatch.load(
					priv_->boidPositionsX_.data(),
					priv_->boidPositionsY_.data(),
					priv_->boidPositionsZ_.data(),
					idx);

			// finally compute the distance to the boid position for a batch of neighbors
			neighborBatch -= boidPos_SIMD;
			auto lengthSq = neighborBatch.lengthSquared();
			auto mask = regen::simd::cmp_lt(lengthSq.c, visualRangeSq_SIMD.c);
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
		newBounds_.min.x = std::min(newBounds_.min.x, priv_->boidPositionsX_[boidIdx]);
		newBounds_.min.y = std::min(newBounds_.min.y, priv_->boidPositionsY_[boidIdx]);
		newBounds_.min.z = std::min(newBounds_.min.z, priv_->boidPositionsZ_[boidIdx]);
		newBounds_.max.x = std::max(newBounds_.max.x, priv_->boidPositionsX_[boidIdx]);
		newBounds_.max.y = std::max(newBounds_.max.y, priv_->boidPositionsY_[boidIdx]);
		newBounds_.max.z = std::max(newBounds_.max.z, priv_->boidPositionsZ_[boidIdx]);
	}
	boidBounds_ = newBounds_;
}

Vec3f BoidsCPU::accumulateForce(BoidData &boid, const Vec3f &boidPos, const Vec3f &boidVel) {
	size_t startIdx = 0;
	boid.sumPos = Vec3f::zero();
	boid.sumVel = Vec3f::zero();
	boid.sumSep = Vec3f::zero();

#ifdef REGEN_BOID_USE_FORCE_SIMD
	if (boid.numNeighbors >= regen::simd::RegisterWidth) {
		// NOTE: unfortunately, this does not buy us much as num neighbors is usually capped
		//       to rather small values, e.g. 100.
		{ // pass 1: compute average position and velocity
			BatchOf_Vec3f avgPosition_SIMD(Vec3f::zero());
			BatchOf_Vec3f avgVelocity_SIMD(Vec3f::zero());
			BatchOf_Vec3f neighborVel_SIMD; // NOLINT(cppcoreguidelines-pro-type-member-init)
			BatchOf_Vec3f neighborPos_SIMD; // NOLINT(cppcoreguidelines-pro-type-member-init)
			for (; startIdx + regen::simd::RegisterWidth <= boid.numNeighbors;
				   startIdx += regen::simd::RegisterWidth) {
				// load the neighbor indices into a SIMD register
				auto idx = regen::simd::loadu_si256(boid.neighbors.data() + startIdx);
				// load the positions of the neighbors into a SIMD register
				neighborPos_SIMD.load(
						priv_->boidPositionsX_.data(),
						priv_->boidPositionsY_.data(),
						priv_->boidPositionsZ_.data(),
						idx);
				neighborVel_SIMD.load(
						priv_->boidVelocityX_.data(),
						priv_->boidVelocityY_.data(),
						priv_->boidVelocityZ_.data(),
						idx);
				avgPosition_SIMD += neighborPos_SIMD;
				avgVelocity_SIMD += neighborVel_SIMD;
			}
			boid.sumPos += avgPosition_SIMD.hsum();
			boid.sumVel += avgVelocity_SIMD.hsum();
		}

		{ // pass 2: compute separation term
			startIdx = 0;
			const BatchOf_Vec3f boidPos_SIMD(boidPos);
			BatchOf_Vec3f neighborPos_SIMD; // NOLINT(cppcoreguidelines-pro-type-member-init)
			BatchOf_Vec3f separation_SIMD(Vec3f::zero());
			const BatchOf_float avoidanceDistanceSq_SIMD(priv_->avoidanceDistance_ * priv_->avoidanceDistance_);
			const BatchOf_float zero_SIMD(0.0f);
			const BatchOf_float one_SIMD(1.0f);

			for (; startIdx + regen::simd::RegisterWidth <= boid.numNeighbors;
				   startIdx += regen::simd::RegisterWidth) {
				// load the neighbor indices into a SIMD register
				auto idx = regen::simd::loadu_si256(boid.neighbors.data() + startIdx);
				// load the positions of the neighbors into a SIMD register
				neighborPos_SIMD.load(
						priv_->boidPositionsX_.data(),
						priv_->boidPositionsY_.data(),
						priv_->boidPositionsZ_.data(),
						idx);

				auto dir = boidPos_SIMD - neighborPos_SIMD;
				auto distSq = dir.lengthSquared();
				auto invDistSq = one_SIMD / distSq;
				auto mask = regen::simd::cmp_lt(distSq.c, avoidanceDistanceSq_SIMD.c);
				invDistSq.c = _mm256_blendv_ps(zero_SIMD.c, invDistSq.c, mask);
				// TODO: push into random direction if distance below threshold?
				separation_SIMD += dir * invDistSq;
			}
			boid.sumSep += separation_SIMD.hsum();
		}
	}
#endif
	{
		for (; startIdx<boid.numNeighbors; startIdx++) {
			auto neighborIdx = boid.neighbors[startIdx];
			auto neighborPos = getBoidPosition(neighborIdx);

			boid.sumPos += neighborPos;
			boid.sumVel += getBoidVelocity(neighborIdx);

			auto boidDirection = boidPos - neighborPos;
			float distance = boidDirection.length();
			if (distance < priv_->avoidanceDistance_) {
				if (distance < 0.001f) {
					boidDirection = Vec3f::random();
					boidDirection.normalize();
				} else {
					boidDirection /= distance * distance;
				}
				boid.sumSep += boidDirection;
			}
		}
	}

	float alpha = 1.0f / static_cast<float>(std::max(boid.numNeighbors,1u));
	return
		(boid.sumSep * priv_->separationWeight_) +
		(boid.sumVel*alpha - boidVel) * priv_->alignmentWeight_ +
		(boid.sumPos*alpha - boidPos) * priv_->coherenceWeight_;
}

void BoidsCPU::simulateBoid(int32_t boidIdx, float dt) {
	auto &boid = boidData_[boidIdx];
	Vec3f boidPos = getBoidPosition(boidIdx);
	Vec3f boidVel = getBoidVelocity(boidIdx);
	// simulate the boid using the three rules of boids
	boid.force = accumulateForce(boid, boidPos, boidVel);

	// a boid is lost if it is outside the bounds
	bool isBoidLost    = (boid.numNeighbors == 0 || !priv_->simBounds_.contains(boidPos));
	bool isInCollision = avoidCollisions(boidPos, boidVel, boid.force, dt);
	bool isInDanger    = !dangers_.empty() && avoidDanger(boidPos, boid.force);

	if (!isInDanger) {
		// note: ignore attractors if in danger
		attract(boidPos, boid.force);
	}
	if (isBoidLost || isInCollision) {
		// drift towards home if lost or in collision
		homesickness(boidPos, boid.force);
	}

	auto dx = boidVel * dt;
	priv_->boidPositionsX_[boidIdx] += dx.x;
	priv_->boidPositionsY_[boidIdx] += dx.y;
	priv_->boidPositionsZ_[boidIdx] += dx.z;
	auto lastVelNorm = boidVel;
	lastVelNorm.normalize();

	float boidSpeed = 0.0f;
	boidVel = limitVelocity(
			lastVelNorm,
			boidVel + boid.force * dt,
			boidSpeed);
	setBoidVelocity(boidIdx, boidVel);
	if (boidSpeed > 0.001f) {
		priv_->boidRotation_.setLookRotation(boidVel / boidSpeed);
		setBoidOrientation(boidIdx, priv_->boidRotation_);
	}

	// clear the neighbors for the next frame
	boid.numNeighbors = 0;
}

Vec3f BoidsCPU::limitVelocity(
		const Vec3f &lastVelNorm,
		const Vec3f &nextVel,
		float &boidSpeed) {
	boidSpeed = nextVel.length();
	Vec3f nextVelNorm = nextVel / boidSpeed;

	// limit translation speed
	boidSpeed = std::min(boidSpeed, priv_->maxBoidSpeed_);
	Vec3f limitedVel = nextVelNorm * boidSpeed;

	// limit angular speed
	auto angle = acos(nextVelNorm.dot(lastVelNorm));
	if (angle > priv_->maxAngularSpeed_) {
		auto axis = nextVelNorm.cross(lastVelNorm);
		axis.normalize();
		priv_->boidRotation_.setAxisAngle(axis, priv_->maxAngularSpeed_);
		auto newDirection = priv_->boidRotation_.rotate(lastVelNorm);
		limitedVel = newDirection * boidSpeed;
	}

	return limitedVel;
}

void BoidsCPU::homesickness(const Vec3f &boidPos, Vec3f &boidForce) {
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
	boidForce += priv_->boidDirection_ * priv_->repulsionTimesSeparation_ * 0.1;
}

bool BoidsCPU::avoidDanger(const Vec3f &boidPos, Vec3f &boidForce) {
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
			boidForce += dir * priv_->repulsionTimesSeparation_;
			isInDanger = true;
		}
	}
	return isInDanger;
}

void BoidsCPU::attract(const Vec3f &boidPos, Vec3f &boidForce) {
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
			boidForce += dir * priv_->repulsionTimesSeparation_;
		}
	}
}

bool BoidsCPU::avoidCollisions(
		const Vec3f &boidPos,
		const Vec3f &boidVel,
		Vec3f &boidForce,
		float dt) {
	auto nextVelocity = boidVel + boidForce * dt;
	nextVelocity.normalize();
	auto lookAhead = boidPos + nextVelocity * priv_->lookAheadDistance_;
	bool isCollisionFree = true;

	////////////////
	/////// Collision with boundaries.
	////////////////
	if (lookAhead.x < priv_->simBounds_.min.x + priv_->avoidanceDistance_) {
		boidForce.x += priv_->repulsionTimesSeparation_;
	} else if (lookAhead.x > priv_->simBounds_.max.x - priv_->avoidanceDistance_) {
		boidForce.x -= priv_->repulsionTimesSeparation_;
	}
	if (lookAhead.y < priv_->simBounds_.min.y + priv_->avoidanceDistance_) {
		boidForce.y += priv_->repulsionTimesSeparation_;
	} else if (lookAhead.y > priv_->simBounds_.max.y - priv_->avoidanceDistance_) {
		boidForce.y -= priv_->repulsionTimesSeparation_;
	}
	if (lookAhead.z < priv_->simBounds_.min.z + priv_->avoidanceDistance_) {
		boidForce.z += priv_->repulsionTimesSeparation_;
	} else if (lookAhead.z > priv_->simBounds_.max.z - priv_->avoidanceDistance_) {
		boidForce.z -= priv_->repulsionTimesSeparation_;
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
				boidForce.y += priv_->repulsionTimesSeparation_;
			}
			isCollisionFree = false;
		}
		if (lookAhead.y < nextY) {
			// Assuming the boid follows the lookAhead direction, it will eventually reach a point where
			// it is below the surface of the height map -> push boid up.
			if (nextY < priv_->simBounds_.max.y - priv_->avoidanceDistanceHalf_) {
				// only push up in case the height map y is below the maximum y of boids
				boidForce.y += priv_->repulsionTimesSeparation_;
			}
		}
		if (nextY > priv_->simBounds_.max.y - priv_->avoidanceDistanceHalf_) {
			// Assuming the boid follows the lookAhead direction, it will eventually reach a point where
			// it is above the max y of the boid bounds. A simple approach is to push the boid to where it came from:
			boidForce.x -= nextVelocity.x * priv_->repulsionTimesSeparation_;
			boidForce.z -= nextVelocity.z * priv_->repulsionTimesSeparation_;
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
