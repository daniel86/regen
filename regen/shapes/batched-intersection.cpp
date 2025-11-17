#include "batched-intersection.h"
#include "regen/shapes/frustum.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/obb.h"

using namespace regen;

BatchedIntersectionTest::BatchedIntersectionTest() {
	// Create some structs for each indexed shape type.
	// These are shared across all cases. However, each case only uses a pair of them.
	shapeData_[SPHERE_IDX]  = std::make_unique<IntersectionData_Sphere>();
	shapeData_[AABB_IDX]    = std::make_unique<IntersectionData_AABB>();
	shapeData_[OBB_IDX]     = std::make_unique<IntersectionData_OBB>();
	shapeData_[FRUSTUM_IDX] = std::make_unique<IntersectionData_Frustum>();
	batchesOfShapes_[SPHERE_IDX]  = std::make_unique<BatchOfSpheres>();
	batchesOfShapes_[AABB_IDX]    = std::make_unique<BatchOfAABBs>();
	batchesOfShapes_[OBB_IDX]     = std::make_unique<BatchOfOBBs>();
	batchesOfShapes_[FRUSTUM_IDX] = std::make_unique<BatchOfFrustums>();
	for (auto &batch : batchesOfShapes_) {
		batch->resize(batchOfCapacity_);
	}
	registerAllCases<BoundingShapeType::SPHERE>();
	registerAllCases<BoundingShapeType::AABB>();
	registerAllCases<BoundingShapeType::OBB>();
	registerAllCases<BoundingShapeType::FRUSTUM>();
}

void BatchedIntersectionTest::setIndexedShapes(const std::vector<ref_ptr<BoundingShape>> *shapes) {
	indexedShapes_ = shapes;
	for (auto &caseBuffer : cases_) {
		caseBuffer->indexedShapes = indexedShapes_;
	}
}

void BatchedIntersectionTest::setHitBuffer(HitBuffer *hits) {
	for (auto &caseBuffer : cases_) {
		caseBuffer->hits = hits;
	}
}


void BatchedIntersectionTest::setBatchCapacity(uint32_t capacity) {
	if (batchOfCapacity_ != capacity) {
		batchOfCapacity_ = capacity;
		for (auto &batch : batchesOfShapes_) {
			batch->resize(batchOfCapacity_);
		}
	}
}

void BatchedIntersectionTest::beginFrame(const BoundingShape &testShape) {
	BoundingShapeType shapeType = testShape.shapeType();
#define _idx(x) static_cast<int8_t>(x)
	using ICT = IntersectionCaseType;
	// Assign the corresponding case buffers based on the test shape type.
	if (shapeType == BoundingShapeType::FRUSTUM) {
		activeCases_[SPHERE_IDX]  = cases_[_idx(ICT::FRUSTUM_SPHERES)].get();
		activeCases_[AABB_IDX]    = cases_[_idx(ICT::FRUSTUM_AABBs)].get();
		activeCases_[OBB_IDX]     = cases_[_idx(ICT::FRUSTUM_OBBs)].get();
		activeCases_[FRUSTUM_IDX] = cases_[_idx(ICT::FRUSTUM_FRUSTUMS)].get();
		// Update frustum test shape memory
		static_cast<IntersectionData_Frustum &>(*shapeData_[FRUSTUM_IDX]).update(testShape);
	} else if (shapeType == BoundingShapeType::SPHERE) {
		activeCases_[SPHERE_IDX]  = cases_[_idx(ICT::SPHERE_SPHERES)].get();
		activeCases_[AABB_IDX]    = cases_[_idx(ICT::SPHERE_AABBs)].get();
		activeCases_[OBB_IDX]     = cases_[_idx(ICT::SPHERE_OBBs)].get();
		activeCases_[FRUSTUM_IDX] = cases_[_idx(ICT::SPHERE_FRUSTUMS)].get();
		// Update sphere test shape memory
		static_cast<IntersectionData_Sphere &>(*shapeData_[SPHERE_IDX]).update(testShape);
	} else if (shapeType == BoundingShapeType::AABB) {
		activeCases_[SPHERE_IDX]  = cases_[_idx(ICT::AABB_SPHERES)].get();
		activeCases_[AABB_IDX]    = cases_[_idx(ICT::AABB_AABBs)].get();
		activeCases_[OBB_IDX]     = cases_[_idx(ICT::AABB_OBBs)].get();
		activeCases_[FRUSTUM_IDX] = cases_[_idx(ICT::AABB_FRUSTUMS)].get();
		// Update AABB test shape memory
		static_cast<IntersectionData_AABB &>(*shapeData_[AABB_IDX]).update(testShape);
	} else { // OBB
		activeCases_[SPHERE_IDX]  = cases_[_idx(ICT::OBB_SPHERES)].get();
		activeCases_[AABB_IDX]    = cases_[_idx(ICT::OBB_AABBs)].get();
		activeCases_[OBB_IDX]     = cases_[_idx(ICT::OBB_OBBs)].get();
		activeCases_[FRUSTUM_IDX] = cases_[_idx(ICT::OBB_FRUSTUMS)].get();
		// Update OBB test shape memory
		static_cast<IntersectionData_OBB &>(*shapeData_[OBB_IDX]).update(testShape);
	}
#undef _idx

	activeCases_[SPHERE_IDX]->init(testShape, batchOfCapacity_,
		&BoundingSphere::globalBatchData_t());
	activeCases_[AABB_IDX]->init(testShape, batchOfCapacity_,
		&AABB::globalBatchData_t());
	activeCases_[OBB_IDX]->init(testShape, batchOfCapacity_,
		&OBB::globalBatchData_t());
	activeCases_[FRUSTUM_IDX]->init(testShape, batchOfCapacity_,
		nullptr);
}

void BatchedIntersectionCase::init(
			const BoundingShape &shape,
			uint32_t capacity,
			const BatchOfShapes *globalBatchData_) {
	this->testShape = &shape;
	this->globalBatchData = globalBatchData_;
	numQueued = 0;
	if (batchData->capacity < capacity) {
		batchData->resize(capacity);
	}
	if (queuedIndices.size() < capacity) {
		queuedIndices.resize(capacity);
		globalIndices.resize(capacity);
	}
}

template <BoundingShapeType BatchType> void copyBatchData(BatchedIntersectionCase &buffer) {
	static constexpr int NUM = ShapeTraits<BatchType>::NumSoAArrays;
	static constexpr int PREFETCH_WIDTH = 16;
	static constexpr int PREFETCH_READ = 0;

	auto& local = buffer.batchData->soaData_;
	auto& global = buffer.globalBatchData->soaData_;

	// Preload base pointers once
	float* __restrict ld[NUM];
	float* __restrict gl[NUM];
	for (int i=0; i<NUM; ++i) {
		ld[i] = static_cast<float*>(__builtin_assume_aligned(local[i].data(), 32));
		gl[i] = static_cast<float*>(__builtin_assume_aligned(global[i].data(), 32));
	}

	// Preload global indices
	uint32_t* __restrict gl_i = static_cast<uint32_t*>(
		__builtin_assume_aligned(buffer.globalIndices.data(), 32));

	const uint32_t numItems = buffer.numQueued;

	for (uint32_t localIdx = 0; localIdx < numItems; ++localIdx) {
		const uint32_t globalIdx = gl_i[localIdx];

		// Prefetch next element (if in bounds)
		if (localIdx + PREFETCH_WIDTH < numItems) {
			// index prefetch: fetch global index PREFETCH_WIDTH ahead
			__builtin_prefetch(gl_i + localIdx + PREFETCH_WIDTH, PREFETCH_READ, 1);
			// read data prefetches: fetch all shape data arrays PREFETCH_WIDTH ahead
			const uint32_t g2 = gl_i[localIdx + PREFETCH_WIDTH];
			for (int i=0; i<NUM; ++i) {
				__builtin_prefetch(gl[i] + g2, PREFETCH_READ, 1);
			}
		}

		// Copy all shape data arrays
		for (int i = 0; i < NUM; ++i) {
			ld[i][localIdx] = gl[i][globalIdx];
		}
	}
}

template <BoundingShapeType BatchType>
inline void flushBuffer(BatchedIntersectionCase &buffer) {
	if (buffer.numQueued != 0) {
		copyBatchData<BatchType>(buffer);
		buffer.flush();
	}
}

template <BoundingShapeType BatchType>
inline void pushBatchData(BatchedIntersectionCase &buffer, uint32_t globalIdx, uint32_t shapeIdx) {
	// Increment per-shape-type counter and push into the correct case buffer
	buffer.queuedIndices[buffer.numQueued] = shapeIdx;
	buffer.globalIndices[buffer.numQueued] = globalIdx;
	if (++buffer.numQueued >= buffer.batchData->capacity) {
		copyBatchData<BatchType>(buffer);
		buffer.flush();
	}
}

void BatchedIntersectionTest::push(const BoundingShape &shape, uint32_t shapeIdx) {
	// Increment per-shape-type counter and push into the correct case buffer
	switch (shape.shapeType()) {
		case BoundingShapeType::SPHERE:
			pushBatchData<BoundingShapeType::SPHERE>(
				*activeCases_[SPHERE_IDX], shape.globalIndex(), shapeIdx);
			break;
		case BoundingShapeType::AABB:
			pushBatchData<BoundingShapeType::AABB>(
				*activeCases_[AABB_IDX], shape.globalIndex(), shapeIdx);
			break;
		case BoundingShapeType::OBB:
			pushBatchData<BoundingShapeType::OBB>(
				*activeCases_[OBB_IDX], shape.globalIndex(), shapeIdx);
			break;
		case BoundingShapeType::FRUSTUM: [[unlikely]]
			pushBatchData<BoundingShapeType::FRUSTUM>(
				*activeCases_[FRUSTUM_IDX], shape.globalIndex(), shapeIdx);
			break;
		case BoundingShapeType::LAST: [[unlikely]]
			break;
	}
}

void BatchedIntersectionTest::flushAllBuffers() {
	flushBuffer<BoundingShapeType::SPHERE>(*activeCases_[SPHERE_IDX]);
	flushBuffer<BoundingShapeType::AABB>(*activeCases_[AABB_IDX]);
	flushBuffer<BoundingShapeType::OBB>(*activeCases_[OBB_IDX]);
	flushBuffer<BoundingShapeType::FRUSTUM>(*activeCases_[FRUSTUM_IDX]);
}
