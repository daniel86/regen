#include "boids-cpu.h"
#include "regen/math/simd.h"
#include "regen/utility/aligned-array.h"

//#define REGEN_BOID_DEBUG_TIME

using namespace regen;

namespace regen {
	static constexpr bool BOID_USE_SIMD = true;
	static constexpr bool BOID_USE_MULTITHREADING = true;

	/**
	 * Data for a single simulation frame.
	 */
	struct BoidSimulationFrame {
		// delta time for this frame
		float dt = 0.0f;
		// start and end indices for this frame
		uint32_t startIdx = 0;
		uint32_t endIdx = 0;
		// thread-local bounds for this frame
		Bounds<Vec3f> localBounds = Bounds<Vec3f>::create(Vec3f::zero(), Vec3f::zero());

		// thread-local temporary storage for boid data
		AlignedArray<float> queuePosX_; // size = maxNumNeighbors
		AlignedArray<float> queuePosY_; // size = maxNumNeighbors
		AlignedArray<float> queuePosZ_; // size = maxNumNeighbors
		AlignedArray<float> queueVelX_; // size = maxNumNeighbors
		AlignedArray<float> queueVelY_; // size = maxNumNeighbors
		AlignedArray<float> queueVelZ_; // size = maxNumNeighbors
		AlignedArray<uint32_t> boidQueue_; // size = maxNumNeighbors

		BoidSimulationFrame& operator=(const BoidSimulationFrame &other) {
			dt = other.dt;
			startIdx = other.startIdx;
			endIdx = other.endIdx;
			localBounds = other.localBounds;
			return *this;
		}

		void resize(unsigned int capacity) {
			queuePosX_.resize(capacity);
			queuePosY_.resize(capacity);
			queuePosZ_.resize(capacity);
			queueVelX_.resize(capacity);
			queueVelY_.resize(capacity);
			queueVelZ_.resize(capacity);
			boidQueue_.resize(capacity);
		}
	};

	/**
	 * Data for a single boid slice (for multithreading).
	 */
	struct BoidSliceData {
		// pointer back to the simulation
		BoidsCPU *sim = nullptr;
		// pointer to private data
		BoidsCPU::Private *priv = nullptr;
		// frame data
		BoidSimulationFrame frame;
		BoidSliceData() = default;
		BoidSliceData(const BoidSliceData &other) {
			sim = other.sim;
			priv = other.priv;
			frame = other.frame;
		}
		BoidSliceData& operator=(const BoidSliceData &other) = default;
	};
}

// private data struct
struct BoidsCPU::Private {
	// Per-boid data
	AlignedArray<uint32_t> boidGridIndex_; // size = numBoids
	AlignedArray<float> boidPositionsX_;  // size = numBoids
	AlignedArray<float> boidPositionsY_;  // size = numBoids
	AlignedArray<float> boidPositionsZ_;  // size = numBoids
	AlignedArray<float> boidVelocityX_;   // size = numBoids
	AlignedArray<float> boidVelocityY_;   // size = numBoids
	AlignedArray<float> boidVelocityZ_;   // size = numBoids
	AlignedArray<float> boidOrientW_;   // size = numBoids
	AlignedArray<float> boidOrientX_;   // size = numBoids
	AlignedArray<float> boidOrientY_;   // size = numBoids
	AlignedArray<float> boidOrientZ_;   // size = numBoids
	AlignedArray<uint32_t> boidNumNeighbors_; // size = numBoids
	std::vector<Vec3f> boidForce_; 	 // size = numBoids
	std::vector<std::vector<uint32_t>> boidNeighbors_; // size = numBoids_ * maxNumNeighbors

	// Spatial grid data
	struct Cell { vectorSIMD<uint32_t> elements; }; // size = maxNumNeighbors
	std::vector<Cell> grid_;
	// (capped) number of boids per cell
	AlignedArray<uint32_t> cellCounts_; // size = numCells

	// Multithreading data
	std::vector<BoidSliceData> boidSlices_;
	uint32_t sliceSize_{};
	JobFrame jobFrame_;

	// Configuration parameters
	float visualRange_ = 1.6f;
	float attractionRange_ = 160.0f;
	float visualRangeSq_ = 0.0f;
	float avoidanceDistance_ = 0.0f;
	float avoidanceDistanceSq_ = 0.0f;
	float avoidanceDistanceHalf_ = 0.0f;
	float repulsionTimesSeparation_ = 0.0f;
	float separationWeight_ = 0.0f;
	float alignmentWeight_ = 0.0f;
	float coherenceWeight_ = 0.0f;
	float lookAheadDistance_ = 0.0f;
	float maxBoidSpeed_ = 0.0f;
	float maxAngularSpeed_ = 0.0f;
	float cos_maxAngularSpeed_ = 1.0f;
	float baseOrientation_ = 0.0f;
	float cellSize_ = 3.2f;
	Vec3i gridSize_ = Vec3i::zero();
	uint32_t gridStamp_ = 0u;
	Vec3f boidsScale_ = Vec3f::zero();
	Quaternion yawAdjust_{};
	unsigned int maxNumNeighbors_ = 0;
	Bounds<Vec3f> simBounds_ = Bounds<Vec3f>::create(Vec3f::create(-10.0f), Vec3f::create(10.0f));

	template <void(*Fn)(void*)> void performSlicedJob();

	static void updateNeighborsJob(void *arg) {
		auto &x = *static_cast<BoidSliceData *>(arg);
		x.priv->updateNeighbours(x);
	}

	static void updateForceJob(void *arg) {
		auto &x = *static_cast<BoidSliceData *>(arg);
		x.priv->updateForces(x);
	}

	static void advanceBoidJob(void *arg) {
		auto &x = *static_cast<BoidSliceData *>(arg);
		x.priv->advanceBoid(x);
	}

	void updateNeighbours(BoidSliceData &slice);

	void updateForces(const BoidSliceData &slice);

	void advanceBoid(BoidSliceData &slice);

	void updateForces(const BoidSimulationFrame &frame, uint32_t boidIdx);

	void updateNeighbours(BoidSimulationFrame &frame, uint32_t boidIdx, uint32_t cellIdx);

	explicit Private(uint32_t numBoids) :
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
			  boidOrientZ_(numBoids),
			  boidNumNeighbors_(numBoids),
			  boidForce_(numBoids, Vec3f::zero()),
			  boidNeighbors_(numBoids)
	{}
};

BoidsCPU::BoidsCPU(const ref_ptr<ModelTransformation> &tf)
		: BoidSimulation(tf),
		  Animation(false, true),
		  priv_(new Private(numBoids_)) {
	priv_->boidVelocityX_.setToZero();
	priv_->boidVelocityY_.setToZero();
	priv_->boidVelocityZ_.setToZero();
	priv_->boidGridIndex_.setToZero();
	for (uint32_t i = 0; i < numBoids_; ++i) {
		setBoidPosition(i, tf_->position(i).r);
	}
}

BoidsCPU::BoidsCPU(const ref_ptr<ShaderInput4f> &modelOffset)
		: BoidSimulation(modelOffset),
		  Animation(false, true),
		  priv_(new Private(numBoids_)) {
	priv_->boidVelocityX_.setToZero();
	priv_->boidVelocityY_.setToZero();
	priv_->boidVelocityZ_.setToZero();
	priv_->boidGridIndex_.setToZero();
	for (uint32_t i = 0; i < numBoids_; ++i) {
		setBoidPosition(i, modelOffset->getVertex(i).r.xyz());
	}
}

BoidsCPU::~BoidsCPU() {
	delete priv_;
}

void BoidsCPU::initBoidSimulation() {
	const auto maxNumNeighbors = maxNumNeighbors_->getVertex(0).r;

	setAnimationName("boids");
	// use a dedicated thread for the boids simulation which is not synchronized with the graphics thread,
	// i.e. it can be slower or faster than the graphics thread.
	setSynchronized(false);
	priv_->baseOrientation_ = baseOrientation_->getVertex(0).r;
	priv_->yawAdjust_.setAxisAngle(Vec3f(0, 1, 0), priv_->baseOrientation_);
	priv_->boidsScale_ = boidsScale_->getVertex(0).r;

	if constexpr (BOID_USE_MULTITHREADING) {
		JobPool& pool = threading::getJobPool();
		const uint32_t numWorkerThreads = pool.numThreads();
		// Compute the number of instances in each slice.
		const uint32_t numSlices = std::min(numBoids_, numWorkerThreads + 1);
		priv_->boidSlices_.resize(numSlices);
		priv_->sliceSize_ = (numBoids_ + numSlices - 1) / numSlices;
		// Make sure each slice starts aligned with simd RegisterWidth
		priv_->sliceSize_ = (priv_->sliceSize_ + simd::RegisterWidth - 1) & ~(simd::RegisterWidth - 1);

		auto &firstSlice = priv_->boidSlices_[0];
		firstSlice.sim = this;
		firstSlice.priv = priv_;
		firstSlice.frame.startIdx = 0;
		firstSlice.frame.endIdx = std::min(priv_->sliceSize_, numBoids_);

		for (uint32_t i = 1; i < numSlices; ++i) {
			auto &slice = priv_->boidSlices_[i];
			slice.sim = this;
			slice.priv = priv_;
			slice.frame.startIdx = i * priv_->sliceSize_;
			slice.frame.endIdx = std::min(slice.frame.startIdx + priv_->sliceSize_, numBoids_);
		}

		// resize slice buffers
		for (auto &slice : priv_->boidSlices_) {
			slice.frame.resize(maxNumNeighbors + 1);
		}
	} else {
		// Only allocate one slice for single-threaded execution.
		auto &firstSlice = priv_->boidSlices_.emplace_back();
		firstSlice.sim = this;
		firstSlice.priv = priv_;
		firstSlice.frame.startIdx = 0;
		firstSlice.frame.endIdx = numBoids_;
		firstSlice.frame.resize(maxNumNeighbors + 1);
		priv_->sliceSize_ = numBoids_;
	}

	const auto numCells = priv_->gridSize_.x * priv_->gridSize_.y * priv_->gridSize_.z;

	// Initialize grid memory.
	for (auto &cell : priv_->grid_) {
		cell.elements.resize(maxNumNeighbors + 1); // +1 to avoid branching
	}
	priv_->cellCounts_.resize(numCells);

	// Initialize per-boid memory.
	for (uint32_t i = 0; i < numBoids_; ++i) {
		// note: an additional element is added to the end of the vector which is used
		//       to avoid branching.
		priv_->boidNeighbors_[i].resize(maxNumNeighbors + 1);
		priv_->boidNumNeighbors_[i] = 0;
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

void BoidsCPU::setBoidOrientation(uint32_t boidIndex, const Quaternion &orientation) {
	priv_->boidOrientW_[boidIndex] = orientation.w;
	priv_->boidOrientX_[boidIndex] = orientation.x;
	priv_->boidOrientY_[boidIndex] = orientation.y;
	priv_->boidOrientZ_[boidIndex] = orientation.z;
}

void BoidsCPU::cpuUpdate(double dt) {
	#ifdef REGEN_BOID_DEBUG_TIME
	thread_local ElapsedTimeDebugger elapsedTime("BoidsCPU Update",
		1000, ElapsedTimeDebugger::CPU_ONLY);
	elapsedTime.beginFrame();
	elapsedTime.push("starting");
	#endif
	const auto dt_f = static_cast<float>(dt) * 0.001f;
	priv_->simBounds_.min = simulationBoundsMin_->getVertex(0).r;
	priv_->simBounds_.max = simulationBoundsMax_->getVertex(0).r;
	priv_->avoidanceDistance_ = avoidanceDistance_->getVertex(0).r;
	priv_->avoidanceDistanceSq_ = priv_->avoidanceDistance_ * priv_->avoidanceDistance_;
	priv_->avoidanceDistanceHalf_ = priv_->avoidanceDistance_ * 0.5f;
	priv_->repulsionTimesSeparation_ = repulsionFactor_->getVertex(0).r * separationWeight_->getVertex(0).r;
	priv_->visualRange_ = visualRange_->getVertex(0).r;
	priv_->visualRangeSq_ = priv_->visualRange_ * priv_->visualRange_;
	priv_->attractionRange_ = attractionRange_->getVertex(0).r;
	priv_->separationWeight_ = separationWeight_->getVertex(0).r;
	priv_->alignmentWeight_ = alignmentWeight_->getVertex(0).r;
	priv_->coherenceWeight_ = coherenceWeight_->getVertex(0).r;
	priv_->lookAheadDistance_ = lookAheadDistance_->getVertex(0).r;
	priv_->maxBoidSpeed_ = maxBoidSpeed_->getVertex(0).r;
	priv_->maxAngularSpeed_ = maxAngularSpeed_->getVertex(0).r;
	priv_->cos_maxAngularSpeed_ = cosf(priv_->maxAngularSpeed_);
	priv_->maxNumNeighbors_ = maxNumNeighbors_->getVertex(0).r;
	priv_->cellSize_ = cellSize_->getVertex(0).r;
	// update dt in all slices
	for (auto &slice : priv_->boidSlices_) {
		slice.frame.dt = dt_f;
	}

	Bounds<Vec3f> bounds = Bounds<Vec3f>::create(Vec3f::posMax(), Vec3f::negMax());
	if constexpr (BOID_USE_MULTITHREADING) {
		priv_->performSlicedJob<Private::updateForceJob>();
		priv_->performSlicedJob<Private::advanceBoidJob>();
		// Accumulate the new bounds from all slices
		for (const auto &slice : priv_->boidSlices_) {
			bounds.extend(slice.frame.localBounds);
		}
	}
	else {
		// Single-threaded update
		const auto numBoids = numBoids_;
		auto &slice = priv_->boidSlices_.front();
		priv_->updateForces(slice);

		for (uint32_t boidIdx = 0; boidIdx < numBoids; ++boidIdx) {
			advanceBoid(boidIdx, dt_f);
			// update the grid bounds
			bounds.min.x = std::min(bounds.min.x, priv_->boidPositionsX_[boidIdx]);
			bounds.min.y = std::min(bounds.min.y, priv_->boidPositionsY_[boidIdx]);
			bounds.min.z = std::min(bounds.min.z, priv_->boidPositionsZ_[boidIdx]);
			bounds.max.x = std::max(bounds.max.x, priv_->boidPositionsX_[boidIdx]);
			bounds.max.y = std::max(bounds.max.y, priv_->boidPositionsY_[boidIdx]);
			bounds.max.z = std::max(bounds.max.z, priv_->boidPositionsZ_[boidIdx]);
		}
	}
	boidBounds_ = bounds;
	#ifdef REGEN_BOID_DEBUG_TIME
	elapsedTime.push("simulation");
	#endif

	// update boids model transformation using the boids data
	updateTransforms();
	#ifdef REGEN_BOID_DEBUG_TIME
	elapsedTime.push("transforms");
	#endif

	// resize the grid, and clear all cells
	clearGrid();
	if (priv_->grid_.empty()) { return; }

	// Update boidGridIndex_: per boid grid cell index
	updateCellIndex();
	// add boids to the grid and compute their neighborhood relations.
	insertIntoCells();
	#ifdef REGEN_BOID_DEBUG_TIME
	elapsedTime.push("binning");
	#endif

	// Update neighbors of each boid (this is expensive)
	if constexpr (BOID_USE_MULTITHREADING) {
		priv_->performSlicedJob<Private::updateNeighborsJob>();
	} else {
		priv_->updateNeighbours(priv_->boidSlices_.front());
	}
	#ifdef REGEN_BOID_DEBUG_TIME
	elapsedTime.push("neighbours");
	elapsedTime.endFrame();
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

		auto firstAdded = priv_->grid_.size();
		priv_->grid_.resize(numCells);
		// reserve space for the neighbor indices in each cell.
		// NOTE: we limit to maxNumNeighbors_ to avoid excessive memory usage.
		const auto maxNumNeighbors = priv_->maxNumNeighbors_;
		for (uint32_t i = firstAdded; i < priv_->grid_.size(); ++i) {
			priv_->grid_[i].elements.resize(maxNumNeighbors + 1);
		}
	}

	if (!priv_->grid_.empty()) {
		const uint32_t numCells = priv_->grid_.size();
		priv_->cellCounts_.resize(numCells);
		// Reset the cell counts to zero
		BatchOf_int32::zeroAligned(priv_->cellCounts_.data(), numCells);
	}
}

template <void(*Fn)(void*)>
void BoidsCPU::Private::performSlicedJob() {
	JobPool& pool = threading::getJobPool();
	const uint32_t numSlices = boidSlices_.size();
	auto &jobFrame = jobFrame_;

	jobFrame.numPushed = 0;

	for (uint32_t i = 1; i < numSlices; ++i) {
		auto &slice = boidSlices_[i];
		pool.addJobPreFrame(jobFrame, Job{ .fn = Fn, .arg = &slice });
	}

	// Execute jobs
	pool.beginFrame(jobFrame, 1u); // one local job
	FramedJob localJob = { { Fn, &boidSlices_[0] }, &jobFrame };
	do {
		JobPool::performJob(localJob);
	} while (pool.stealJob(localJob));
	JobPool::endFrame(jobFrame);
}

void BoidsCPU::updateCellIndex() {
	// NOTE: This could easily be done in parallel if needed.
	//       But it seems superfast already, but in case it becomes a bottleneck,
	//       we can parallelize it.
	const auto numBoids = numBoids_;

	const auto* __restrict boidPosX = static_cast<float*>(__builtin_assume_aligned(priv_->boidPositionsX_.data(), 32));
	const auto* __restrict boidPosY = static_cast<float*>(__builtin_assume_aligned(priv_->boidPositionsY_.data(), 32));
	const auto* __restrict boidPosZ = static_cast<float*>(__builtin_assume_aligned(priv_->boidPositionsZ_.data(), 32));
	auto* __restrict boidGridIndex = static_cast<uint32_t*>(__builtin_assume_aligned(priv_->boidGridIndex_.data(), 32));

	uint32_t boidIdx = 0u;

	if constexpr (BOID_USE_SIMD) {
		const BatchOf_Vec3f gridMin = BatchOf_Vec3f::fromScalar(gridBounds_.min);
		const BatchOf_Vec3i gridSize = BatchOf_Vec3i::fromScalar(priv_->gridSize_);
		const BatchOf_int32 allOne = BatchOf_int32::fromScalar(1);
		const BatchOf_float allZero = BatchOf_float::allZeros();
		const BatchOf_float simd_cellSize = BatchOf_float::fromScalar(priv_->cellSize_);

		BatchOf_Vec3f boidBatch; // NOLINT(cppcoreguidelines-pro-type-member-init)
		// we compute the grid index in batches
		for (; boidIdx + simd::RegisterWidth <= numBoids; boidIdx += simd::RegisterWidth) {
			// load the boid positions into a SIMD register
			boidBatch.setAligned(
					boidPosX + boidIdx,
					boidPosY + boidIdx,
					boidPosZ + boidIdx);
			// x = (x - gridBounds_.min) / cellSize
			boidBatch = (boidBatch - gridMin) / simd_cellSize;
			// clamp to 0+
			boidBatch.x = BatchOf_float::max(boidBatch.x, allZero);
			boidBatch.y = BatchOf_float::max(boidBatch.y, allZero);
			boidBatch.z = BatchOf_float::max(boidBatch.z, allZero);

			// floor to integer grid indices using cvttps_epi32 + clamp to max grid bounds
			BatchOf_Vec3i gridIndices = boidBatch.floor().min(gridSize - allOne);
			// i_f = ix + iy_f + iz_f;
			//    - iy_f = iy * gridSize.x
			//    - iz_f = iz * gridSize.x * gridSize.y;
			const BatchOf_int32 i_f = (gridIndices.x +
				(gridIndices.y * gridSize.x) +
				(gridIndices.z * gridSize.x * gridSize.y));
			// store results in local array
			i_f.storeAligned(boidGridIndex + boidIdx);
		}
	}

	// Fallback to scalar loop for grid index computation
	for (; boidIdx < numBoids;  ++boidIdx) {
		const Vec3f boidPos = Vec3f::max(
				(getBoidPosition(boidIdx) - gridBounds_.min) / priv_->cellSize_,
				// ensure the boid position is within the grid bounds
				Vec3f::zero());
		Vec3i gridIndex(
				static_cast<int>(std::trunc(boidPos.x)),
				static_cast<int>(std::trunc(boidPos.y)),
				static_cast<int>(std::trunc(boidPos.z)));
		// ensure the boid position is within the grid bounds
		const Vec3i idx3D = Vec3i::min(gridIndex,
						  priv_->gridSize_ - Vec3i::one());

		boidGridIndex[boidIdx] = getGridIndex(idx3D, priv_->gridSize_);
	}
}

void BoidsCPU::insertIntoCells() {
	// Note: This function would be a bit more difficult to parallelize since multiple threads
	//       could write to the same cell simultaneously.
	//       E.g. if each thread has a private copy of the grid, we would need to merge them at the end.
	//       However, since this function is relatively fast already, we can leave it as is for now.
	//       Prefix sum could also be used to parallelize this function over cells instead of boids.
	const auto numBoids = numBoids_;
	uint32_t boidIdx = 0u;

	auto* __restrict cellCounts =
		static_cast<uint32_t*>(__builtin_assume_aligned(priv_->cellCounts_.data(), 32));
	auto* __restrict grid = priv_->grid_.data();
	const auto* __restrict boidGridIndex =
		static_cast<const uint32_t*>(__builtin_assume_aligned(priv_->boidGridIndex_.data(), 32));

	for (boidIdx = 0; boidIdx < numBoids; ++boidIdx) {
		const uint32_t cellIndex = boidGridIndex[boidIdx];
		const uint32_t numElements = cellCounts[cellIndex];
		auto* cell = grid[cellIndex].elements.data();
		cell[numElements] = boidIdx;
		cellCounts[cellIndex] += static_cast<uint32_t>(numElements < priv_->maxNumNeighbors_);
	}
}

void BoidsCPU::Private::updateNeighbours(BoidSliceData &slice) {
	// Update neighbors for all boids in the given slice.
	const uint32_t startIdx = slice.frame.startIdx;
	const uint32_t endIdx = slice.frame.endIdx;

	const auto* __restrict boidGridIndex =
		static_cast<const uint32_t*>(__builtin_assume_aligned(boidGridIndex_.data(), 32));

	for (uint32_t boidIdx = startIdx; boidIdx < endIdx; ++boidIdx) {
		const uint32_t cellIndex = boidGridIndex[boidIdx];
		updateNeighbours(slice.frame, boidIdx, cellIndex);
	}
}

void BoidsCPU::Private::updateNeighbours(BoidSimulationFrame &frame, uint32_t boidIdx, uint32_t cellIdx) {
	const auto* __restrict globalX = static_cast<float*>(__builtin_assume_aligned(boidPositionsX_.data(), 32));
	const auto* __restrict globalY = static_cast<float*>(__builtin_assume_aligned(boidPositionsY_.data(), 32));
	const auto* __restrict globalZ = static_cast<float*>(__builtin_assume_aligned(boidPositionsZ_.data(), 32));
	const auto* __restrict cellCounts = static_cast<const uint32_t*>(__builtin_assume_aligned(cellCounts_.data(), 32));
	auto* __restrict queueX = static_cast<float*>(__builtin_assume_aligned(frame.queuePosX_.data(), 32));
	auto* __restrict queueY = static_cast<float*>(__builtin_assume_aligned(frame.queuePosY_.data(), 32));
	auto* __restrict queueZ = static_cast<float*>(__builtin_assume_aligned(frame.queuePosZ_.data(), 32));
	auto* __restrict boidQueue = static_cast<uint32_t*>(__builtin_assume_aligned(frame.boidQueue_.data(), 32));
	auto* __restrict neighbors = boidNeighbors_[boidIdx].data();

	const uint32_t neighborCount = cellCounts[cellIdx];
	const auto* __restrict neighborIndices = grid_[cellIdx].elements.data();

	const float boidPosX = globalX[boidIdx];
	const float boidPosY = globalY[boidIdx];
	const float boidPosZ = globalZ[boidIdx];
	const uint32_t maxNumNeighbors = maxNumNeighbors_;

	uint32_t numHits = 0;
	uint32_t queueIdx = 0;

	// Load the neighbor positions into local SOA arrays
	for (uint32_t i = 0; i < neighborCount; ++i) {
		const uint32_t neighborIdx = neighborIndices[i];
		queueX[i] = globalX[neighborIdx];
		queueY[i] = globalY[neighborIdx];
		queueZ[i] = globalZ[neighborIdx];
	}

	if constexpr (BOID_USE_SIMD) {
		const BatchOf_Vec3f b_boidPos{
			BatchOf_float::fromScalar(boidPosX),
			BatchOf_float::fromScalar(boidPosY),
			BatchOf_float::fromScalar(boidPosZ) };
		const BatchOf_float b_visualRangeSq = BatchOf_float::fromScalar(visualRangeSq_);

		for (; queueIdx + simd::RegisterWidth <= neighborCount; queueIdx += simd::RegisterWidth) {
			// load the positions of the neighbors into a SIMD register
			const BatchOf_Vec3f candidatePos = BatchOf_Vec3f::loadAligned(
				queueX + queueIdx,
				queueY + queueIdx,
				queueZ + queueIdx);

			// finally compute the distance to the boid position for a batch of neighbors
			const BatchOf_float lengthSq = (candidatePos - b_boidPos).lengthSquared();
			const BatchOf_float mask     = (lengthSq < b_visualRangeSq);

			// Convert lanes (1/0) to bitmask value
			uint8_t maskBits = mask.toBitmask8();
			while (maskBits) {
				int bitIndex = simd::nextBitIndex<uint8_t>(maskBits);
				boidQueue[numHits++] = queueIdx + bitIndex;
			}
		}
	}

	// Fallback to scalar loop for remaining elements
	for (; queueIdx < neighborCount; queueIdx++) {
		const Vec3f dx {
			queueX[queueIdx] - boidPosX,
			queueY[queueIdx] - boidPosY,
			queueZ[queueIdx] - boidPosZ };
		if (dx.lengthSquared() <= visualRangeSq_) {
			boidQueue[numHits++] = queueIdx;
		}
	}

	// Process the found neighbors
	uint32_t numNeighbors = 0u;
	for (uint32_t hitIdx = 0;
			hitIdx < numHits &&
			numNeighbors < maxNumNeighbors; ++hitIdx) {
		const uint32_t neighborIdx = neighborIndices[boidQueue[hitIdx]];
		if (neighborIdx == boidIdx) continue; // skip self
		neighbors[numNeighbors++] = neighborIdx;
	}
	boidNumNeighbors_[boidIdx] = numNeighbors;
}

void BoidsCPU::Private::updateForces(const BoidSliceData &slice) {
	// Update forces for all boids in the given slice.
	const uint32_t startIdx = slice.frame.startIdx;
	const uint32_t endIdx = slice.frame.endIdx;
	for (uint32_t boidIdx = startIdx; boidIdx < endIdx; ++boidIdx) {
		updateForces(slice.frame, boidIdx);
	}
}

void BoidsCPU::Private::updateForces(const BoidSimulationFrame &frame, uint32_t boidIdx) {
	const uint32_t numNeighbors = boidNumNeighbors_[boidIdx];
	const float alpha = 1.0f / static_cast<float>(std::max(numNeighbors,1u));
	const float boidPosX = boidPositionsX_[boidIdx];
	const float boidPosY = boidPositionsY_[boidIdx];
	const float boidPosZ = boidPositionsZ_[boidIdx];

	auto* __restrict queuePosX = static_cast<float*>(__builtin_assume_aligned(frame.queuePosX_.data(), 32));
	auto* __restrict queuePosY = static_cast<float*>(__builtin_assume_aligned(frame.queuePosY_.data(), 32));
	auto* __restrict queuePosZ = static_cast<float*>(__builtin_assume_aligned(frame.queuePosZ_.data(), 32));
	auto* __restrict queueVelX = static_cast<float*>(__builtin_assume_aligned(frame.queueVelX_.data(), 32));
	auto* __restrict queueVelY = static_cast<float*>(__builtin_assume_aligned(frame.queueVelY_.data(), 32));
	auto* __restrict queueVelZ = static_cast<float*>(__builtin_assume_aligned(frame.queueVelZ_.data(), 32));
	const auto* __restrict globalPosX = static_cast<const float*>(__builtin_assume_aligned(boidPositionsX_.data(), 32));
	const auto* __restrict globalPosY = static_cast<const float*>(__builtin_assume_aligned(boidPositionsY_.data(), 32));
	const auto* __restrict globalPosZ = static_cast<const float*>(__builtin_assume_aligned(boidPositionsZ_.data(), 32));
	const auto* __restrict globalVelX = static_cast<const float*>(__builtin_assume_aligned(boidVelocityX_.data(), 32));
	const auto* __restrict globalVelY = static_cast<const float*>(__builtin_assume_aligned(boidVelocityY_.data(), 32));
	const auto* __restrict globalVelZ = static_cast<const float*>(__builtin_assume_aligned(boidVelocityZ_.data(), 32));
	const auto* __restrict neighborIndices = boidNeighbors_[boidIdx].data();

	// Load the neighbor positions into local SOA arrays
	for (uint32_t i = 0; i < numNeighbors; ++i) {
		const uint32_t neighborIdx = neighborIndices[i];
		queuePosX[i] = globalPosX[neighborIdx];
		queuePosY[i] = globalPosY[neighborIdx];
		queuePosZ[i] = globalPosZ[neighborIdx];
		queueVelX[i] = globalVelX[neighborIdx];
		queueVelY[i] = globalVelY[neighborIdx];
		queueVelZ[i] = globalVelZ[neighborIdx];
	}

	Vec3f sumPos = Vec3f::zero();
	Vec3f sumVel = Vec3f::zero();
	Vec3f sumSep = Vec3f::zero();
	uint32_t queueIdx = 0;

	if constexpr (BOID_USE_SIMD) {
		{ // pass 1:
			const BatchOf_Vec3f fixedPos = BatchOf_Vec3f{
				BatchOf_float::fromScalar(boidPosX),
				BatchOf_float::fromScalar(boidPosY),
				BatchOf_float::fromScalar(boidPosZ) };
			const BatchOf_float avoidance = BatchOf_float::fromScalar(avoidanceDistanceSq_);
			const BatchOf_float epsilon = BatchOf_float::fromScalar(0.0001f);

			BatchOf_Vec3f sumPosVec = BatchOf_Vec3f::fromScalar(Vec3f::zero());
			BatchOf_Vec3f separation = BatchOf_Vec3f::fromScalar(Vec3f::zero());

			for (; queueIdx + simd::RegisterWidth <= numNeighbors; queueIdx += simd::RegisterWidth) {
				// load the positions of the neighbors into a SIMD register
				BatchOf_Vec3f x = BatchOf_Vec3f::loadAligned(
						queuePosX + queueIdx,
						queuePosY + queueIdx,
						queuePosZ + queueIdx);
				// Accumulate position for the cohesion term.
				sumPosVec += x;
				// Compute vector from neighbor (x) to boid (fixedPos) for the separation term.
				x = fixedPos - x;
				// Compute distance squared which is used for weighting the separation force
				// (closer neighbors contribute more).
				BatchOf_float distSq = x.lengthSquared();
				// Use masking to only consider neighbors within the avoidance distance.
				distSq = (distSq < avoidance).maskToFloat() / (distSq + epsilon);
				// Finally, accumulate the separation force.
				separation += x * distSq;
			}
			// Horizontal sum of SIMD registers to get final results
			sumSep += separation.hsum();
			sumPos += sumPosVec.hsum();
		}

		{ // pass 2: Compute average velocity of neighbors (alignment term)
			// note: we do this separately because the number of available registers is limited,
			//       and we might create too much register pressure otherwise.
			BatchOf_Vec3f sumVelVec = BatchOf_Vec3f::fromScalar(Vec3f::zero());
			for (queueIdx=0; queueIdx + simd::RegisterWidth <= numNeighbors; queueIdx += simd::RegisterWidth) {
				sumVelVec += BatchOf_Vec3f::loadAligned(
					queueVelX + queueIdx,
					queueVelY + queueIdx,
					queueVelZ + queueIdx);
			}
			sumVel += sumVelVec.hsum();
		}
	}

	// Fallback to scalar loop for remaining elements
	for (; queueIdx < numNeighbors; queueIdx++) {
		sumPos.x += queuePosX[queueIdx];
		sumPos.y += queuePosY[queueIdx];
		sumPos.z += queuePosZ[queueIdx];

		const Vec3f boidDirection {
			boidPosX - queuePosX[queueIdx],
			boidPosY - queuePosY[queueIdx],
			boidPosZ - queuePosZ[queueIdx] };
		const float dSq = boidDirection.lengthSquared();
		const auto mask = static_cast<float>(dSq < avoidanceDistanceSq_);
		sumSep += boidDirection * mask / (dSq + 0.0001f); // avoid division by zero

		sumVel.x += queueVelX[queueIdx];
		sumVel.y += queueVelY[queueIdx];
		sumVel.z += queueVelZ[queueIdx];
	}

	sumVel *= alpha;
	sumVel.x -= boidVelocityX_[boidIdx];
	sumVel.y -= boidVelocityY_[boidIdx];
	sumVel.z -= boidVelocityZ_[boidIdx];

	sumPos *= alpha;
	sumPos.x -= boidPosX;
	sumPos.y -= boidPosY;
	sumPos.z -= boidPosZ;

	boidForce_[boidIdx] = sumSep*separationWeight_ + sumVel*alignmentWeight_ + sumPos*coherenceWeight_;
}

void BoidsCPU::Private::advanceBoid(BoidSliceData &slice) {
	const auto startIdx = slice.frame.startIdx;
	const auto endIdx = slice.frame.endIdx;
	const auto dt = slice.frame.dt;
	auto &sim = *slice.sim;

	// Each frame, we compute new bounds. In a threaded setting
	// each thread must compute local bounds, these are later consolidated
	// into the global bounds.
	Bounds<Vec3f> &localBounds = slice.frame.localBounds;
	localBounds.min = Vec3f::posMax();
	localBounds.max = Vec3f::negMax();

	for (uint32_t boidIdx = startIdx; boidIdx < endIdx; ++boidIdx) {
		sim.advanceBoid(boidIdx, dt);
		// update the local bounds
		localBounds.min.x = std::min(localBounds.min.x, boidPositionsX_[boidIdx]);
		localBounds.min.y = std::min(localBounds.min.y, boidPositionsY_[boidIdx]);
		localBounds.min.z = std::min(localBounds.min.z, boidPositionsZ_[boidIdx]);
		localBounds.max.x = std::max(localBounds.max.x, boidPositionsX_[boidIdx]);
		localBounds.max.y = std::max(localBounds.max.y, boidPositionsY_[boidIdx]);
		localBounds.max.z = std::max(localBounds.max.z, boidPositionsZ_[boidIdx]);
	}
}

void BoidsCPU::advanceBoid(uint32_t boidIdx, float dt) {
	const uint32_t numNeighbors = priv_->boidNumNeighbors_[boidIdx];
	const Vec3f boidPos = getBoidPosition(boidIdx);
	Vec3f boidVel = getBoidVelocity(boidIdx);
	Vec3f boidForce = priv_->boidForce_[boidIdx];

	// a boid is lost if it is outside the bounds
	const bool isBoidLost    = (numNeighbors == 0 || !priv_->simBounds_.contains(boidPos));
	const bool isInCollision = avoidCollisions(boidPos, boidVel, boidForce, dt);
	const bool isInDanger    = !dangers_.empty() && avoidDanger(boidPos, boidForce);

	if (!isInDanger) {
		// note: ignore attractors if in danger
		attract(boidPos, boidForce);
	}
	if (isBoidLost || isInCollision) {
		// drift towards home if lost or in collision
		homesickness(boidPos, boidForce);
	}

	const auto dx = boidVel * dt;
	priv_->boidPositionsX_[boidIdx] += dx.x;
	priv_->boidPositionsY_[boidIdx] += dx.y;
	priv_->boidPositionsZ_[boidIdx] += dx.z;
	auto lastVelNorm = boidVel;
	lastVelNorm.normalize();

	float boidSpeed = 0.0f;
	boidVel = limitVelocity(
			lastVelNorm,
			boidVel + boidForce * dt,
			boidSpeed);
	setBoidVelocity(boidIdx, boidVel);
	if (boidSpeed > 0.001f) {
		Quaternion boidRotation;
		boidRotation.setLookRotation(boidVel / boidSpeed);
		setBoidOrientation(boidIdx, boidRotation);
	}
}

Vec3f BoidsCPU::limitVelocity(const Vec3f &lastVelNorm, const Vec3f &nextVel, float &boidSpeed) const {
	boidSpeed = nextVel.length();
	const Vec3f nextVelNorm = nextVel / boidSpeed;

	// limit translation speed
	boidSpeed = std::min(boidSpeed, priv_->maxBoidSpeed_);
	Vec3f limitedVel = nextVelNorm * boidSpeed;

	// limit angular speed
	if (nextVelNorm.dot(lastVelNorm) < priv_->cos_maxAngularSpeed_) {
		auto axis = nextVelNorm.cross(lastVelNorm);
		axis.normalize();
		Quaternion boidRotation;
		boidRotation.setAxisAngle(axis, priv_->maxAngularSpeed_);
		const auto newDirection = boidRotation.rotate(lastVelNorm);
		limitedVel = newDirection * boidSpeed;
	}

	return limitedVel;
}

void BoidsCPU::homesickness(const Vec3f &boidPos, Vec3f &boidForce) const {
	static constexpr float MaxFloat = std::numeric_limits<float>::max();
	if (homePoints_.empty()) { return; }

	// a boid seems to have lost track, and wants to go home!
	// first find closest home point...
	const Vec3f *closestHomePoint = &Vec3f::zero();
	float minDistance = MaxFloat;
	for (auto &home: homePoints_) {
		auto distance = (boidPos - home).lengthSquared();
		if (distance < minDistance) {
			minDistance = distance;
			closestHomePoint = &home;
		}
	}
	const Vec3f boidDirection = (*closestHomePoint - boidPos) / sqrt(minDistance);
	boidForce += boidDirection * priv_->repulsionTimesSeparation_ * 0.1;
}

bool BoidsCPU::avoidDanger(const Vec3f &boidPos, Vec3f &boidForce) const {
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
		auto distance = dir.lengthSquared();
		if (distance < priv_->visualRangeSq_) {
			if (distance < 0.001f) {
				dir = Vec3f::random();
				dir.normalize();
			} else {
				dir /= sqrt(distance);
			}
			boidForce += dir * priv_->repulsionTimesSeparation_;
			isInDanger = true;
		}
	}
	return isInDanger;
}

void BoidsCPU::attract(const Vec3f &boidPos, Vec3f &boidForce) const {
	auto maxDistance = priv_->attractionRange_;
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
		float dt) const {
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
	/////// Collision with collision map which stores solid areas in the red channel.
	////////////////
	if (collisionMap_.get()) {
		auto uv0 = computeUV(boidPos, collisionMapCenter_, collisionMapSize_);
		uv0.x = math::clamp(uv0.x, 0.0f, 1.0f);
		uv0.y = math::clamp(uv0.y, 0.0f, 1.0f);
		auto uv1 = computeUV(lookAhead, collisionMapCenter_, collisionMapSize_);
		uv1.x = math::clamp(uv1.x, 0.0f, 1.0f);
		uv1.y = math::clamp(uv1.y, 0.0f, 1.0f);

		if (collisionMapType_ == COLLISION_VECTOR_FIELD) {
			auto sample0 = collisionMap_->sampleLinear<Vec3f,3>(uv0, collisionMap_->textureData());
			auto sample1 = collisionMap_->sampleLinear<Vec3f,3>(uv1, collisionMap_->textureData());
			Vec2f collisionFlow = sample0.xy();
			float collisionStrength = sample0.z;
			if (sample1.z > collisionStrength) {
				collisionStrength = sample1.z;
				collisionFlow = sample1.xy();
			}

			if (collisionStrength > 0.01f) {
				// Compute avoidance force proportional to strength
				float strength = priv_->repulsionTimesSeparation_ * collisionStrength * 3.0f;
				boidForce.x += collisionFlow.x * strength;
				boidForce.z += collisionFlow.y * strength;
				// dampen forward velocity slightly near obstacles
				boidForce -= nextVelocity * (strength * 0.2f);
			}
		} else {
			// Sample at current and lookahead positions
			float collisionValue = 0.0f;
			static constexpr int NumSamples = 3; // sample along velocity
			for (int i = 0; i < NumSamples; ++i) {
				float t = (float)i / (NumSamples - 1);
				Vec3f pos = boidPos + nextVelocity * t;
				auto uvSample = computeUV(pos, collisionMapCenter_, collisionMapSize_);
				uvSample.x = 1.0f - std::clamp(uvSample.x, 0.0f, 1.0f);
				uvSample.y = std::clamp(uvSample.y, 0.0f, 1.0f);
				collisionValue = std::max(collisionValue,
					collisionMap_->sampleLinear<float,1>(uvSample, collisionMap_->textureData()));
			}

			if (collisionValue > 0.05f) {
				const float eps = 1.0f / float(collisionMap_->width());
				uv0.x = 1.0f - uv0.x; // HACK: flip x for sampling
				float c = collisionMap_->sampleLinear<float,1>(uv0, collisionMap_->textureData()); // central sample
				float dx = collisionMap_->sampleLinear<float,1>(uv0 + Vec2f(eps,0), collisionMap_->textureData())
						 - collisionMap_->sampleLinear<float,1>(uv0 - Vec2f(eps,0), collisionMap_->textureData());
				float dy = collisionMap_->sampleLinear<float,1>(uv0 + Vec2f(0,eps), collisionMap_->textureData())
						 - collisionMap_->sampleLinear<float,1>(uv0 - Vec2f(0,eps), collisionMap_->textureData());

				Vec3f normal(dx, 0.0f, dy);
				float normalLength = normal.lengthSquared();
				if (normalLength > 1e-8f) normal /= sqrt(normalLength);
				else {
					Vec3f up(0,1,0);
					normal = nextVelocity.cross(up);
					normalLength = normal.lengthSquared();
					if (normalLength > 1e-8f) normal /= sqrt(normalLength);
				}

				// use both the max along lookahead and the center sample to compute final strength
				float strength = priv_->repulsionTimesSeparation_ * std::clamp(0.75f * collisionValue + 0.25f * c, 0.0f, 1.0f);

				boidForce -= normal * strength;
				boidForce -= nextVelocity * (strength * 0.3f);
			}
		}
	}

	////////////////
	/////// Collision with height map.
	////////////////
	if (heightMap_.get()) {
		// boid position in height map space [0, 1]
		auto boidCoord = computeUV(boidPos, mapCenter_, mapSize_);
		auto currentY = heightMap_->sampleLinear<float,1>(boidCoord, heightMap_->textureData());
		currentY = currentY*heightMapFactor_ + mapCenter_.y + priv_->avoidanceDistanceHalf_;

		// sample the height map at the projected boid position
		boidCoord = computeUV(lookAhead, mapCenter_, mapSize_);
		auto nextY = heightMap_->sampleLinear<float,1>(boidCoord, heightMap_->textureData());
		nextY = nextY*heightMapFactor_ + mapCenter_.y + priv_->avoidanceDistanceHalf_;

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
