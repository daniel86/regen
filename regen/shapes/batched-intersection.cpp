#include "batched-intersection.h"
#include "regen/shapes/frustum.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/obb.h"

using namespace regen;

BatchedIntersectionTest::BatchedIntersectionTest() {
	// Create some structs for each indexed shape type.
	// These are shared across all cases. However, each case only uses a pair of them.
#define _idx(x) static_cast<int8_t>(x)
	shapeData_[_idx(BoundingShapeType::SPHERE)]  = std::make_unique<IntersectionData_Sphere>();
	shapeData_[_idx(BoundingShapeType::AABB)]    = std::make_unique<IntersectionData_AABB>();
	shapeData_[_idx(BoundingShapeType::OBB)]     = std::make_unique<IntersectionData_OBB>();
	shapeData_[_idx(BoundingShapeType::FRUSTUM)] = std::make_unique<IntersectionData_Frustum>();
	batchesOfShapes_[_idx(BoundingShapeType::SPHERE)]  = std::make_unique<BatchOfSpheres>();
	batchesOfShapes_[_idx(BoundingShapeType::AABB)]    = std::make_unique<BatchOfAABBs>();
	batchesOfShapes_[_idx(BoundingShapeType::OBB)]     = std::make_unique<BatchOfOBBs>();
	batchesOfShapes_[_idx(BoundingShapeType::FRUSTUM)] = std::make_unique<BatchOfFrustums>();
	for (auto &batch : batchesOfShapes_) {
		batch->resize(batchOfCapacity_);
	}
#undef _idx
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
	using IST = BoundingShapeType;
	using ICT = IntersectionCaseType;
	// Assign the corresponding case buffers based on the test shape type.
	if (shapeType == BoundingShapeType::FRUSTUM) {
		frameCases_[_idx(IST::SPHERE)]  = _idx(ICT::FRUSTUM_SPHERES);
		frameCases_[_idx(IST::AABB)]    = _idx(ICT::FRUSTUM_AABBs);
		frameCases_[_idx(IST::OBB)]     = _idx(ICT::FRUSTUM_OBBs);
		frameCases_[_idx(IST::FRUSTUM)] = _idx(ICT::FRUSTUM_FRUSTUMS);
		// Update frustum test shape memory
		auto &mem = static_cast<IntersectionData_Frustum &>(*shapeData_[_idx(BoundingShapeType::FRUSTUM)]);
		mem.update(testShape);
	} else if (shapeType == BoundingShapeType::SPHERE) {
		frameCases_[_idx(IST::SPHERE)]  = _idx(ICT::SPHERE_SPHERES);
		frameCases_[_idx(IST::AABB)]    = _idx(ICT::SPHERE_AABBs);
		frameCases_[_idx(IST::OBB)]     = _idx(ICT::SPHERE_OBBs);
		frameCases_[_idx(IST::FRUSTUM)] = _idx(ICT::SPHERE_FRUSTUMS);
		// Update sphere test shape memory
		auto &mem = static_cast<IntersectionData_Sphere &>(*shapeData_[_idx(BoundingShapeType::SPHERE)]);
		mem.update(testShape);
	} else if (shapeType == BoundingShapeType::AABB) {
		frameCases_[_idx(IST::SPHERE)]  = _idx(ICT::AABB_SPHERES);
		frameCases_[_idx(IST::AABB)]    = _idx(ICT::AABB_AABBs);
		frameCases_[_idx(IST::OBB)]     = _idx(ICT::AABB_OBBs);
		frameCases_[_idx(IST::FRUSTUM)] = _idx(ICT::AABB_FRUSTUMS);
		// Update AABB test shape memory
		auto &mem = static_cast<IntersectionData_AABB &>(*shapeData_[_idx(BoundingShapeType::AABB)]);
		mem.update(testShape);
	} else { // OBB
		frameCases_[_idx(IST::SPHERE)]  = _idx(ICT::OBB_SPHERES);
		frameCases_[_idx(IST::AABB)]    = _idx(ICT::OBB_AABBs);
		frameCases_[_idx(IST::OBB)]     = _idx(ICT::OBB_OBBs);
		frameCases_[_idx(IST::FRUSTUM)] = _idx(ICT::OBB_FRUSTUMS);
		// Update OBB test shape memory
		auto &mem = static_cast<IntersectionData_OBB &>(*shapeData_[_idx(BoundingShapeType::OBB)]);
		mem.update(testShape);
	}
	currentSpheresCase_  = cases_[frameCases_[_idx(IST::SPHERE)]].get();
	currentAABBsCase_    = cases_[frameCases_[_idx(IST::AABB)]].get();
	currentOBBsCase_     = cases_[frameCases_[_idx(IST::OBB)]].get();
	currentFrustumsCase_ = cases_[frameCases_[_idx(IST::FRUSTUM)]].get();
#undef _idx

	currentSpheresCase_->init(testShape, batchOfCapacity_);
	currentAABBsCase_->init(testShape, batchOfCapacity_);
	currentOBBsCase_->init(testShape, batchOfCapacity_);
	currentFrustumsCase_->init(testShape, batchOfCapacity_);
}

template <BoundingShapeType BatchType>
static void addToBatchTest(BatchedIntersectionCase &ic, const BoundingShape &shape, uint32_t shapeIdx) {
	static constexpr int NUM_ARRAYS = ShapeTraits<BatchType>::NumSoAArrays;

	auto &localSoAData = ic.batchData->soaData_;
	const auto &globalSoAData = shape.globalBatchData().soaData_;
	const uint32_t localIdx = ic.numQueued;
	const uint32_t globalIdx = shape.globalIndex();

	// Record the shape index at this local position.
	ic.queuedIndices[localIdx] = shapeIdx;

	// Copy SoA data from global to local batch.
	// We do this such that we can do aligned loading in the vectorized code.
	// This is all the data needed for the intersection tests.
	// The reason we need to do this is that our local batch may be smaller than the global batch,
	// and with different ordering.
	// Note: copy could be avoided by gathering over global data directly, but that might kill
	//       performance due to unaligned loads in most cases.
	for (size_t i = 0; i < NUM_ARRAYS; ++i) {
		float* __restrict ld = static_cast<float*>(__builtin_assume_aligned(localSoAData[i].data(), 32));
		float* __restrict gl = static_cast<float*>(__builtin_assume_aligned(globalSoAData[i].data(), 32));
		ld[localIdx] = gl[globalIdx];
	}

	++ic.numQueued;
}

void BatchedIntersectionTest::push(uint32_t shapeIdx) {
	auto &shape = (*indexedShapes_)[shapeIdx];

	// Dispatch to the correct test case
	switch (shape->shapeType()) {
		case BoundingShapeType::SPHERE:
			addToBatchTest<BoundingShapeType::SPHERE>(*currentSpheresCase_, *shape.get(), shapeIdx);
			break;
		case BoundingShapeType::AABB:
			addToBatchTest<BoundingShapeType::AABB>(*currentAABBsCase_, *shape.get(), shapeIdx);
			break;
		case BoundingShapeType::OBB:
			addToBatchTest<BoundingShapeType::OBB>(*currentOBBsCase_, *shape.get(), shapeIdx);
			break;
		case BoundingShapeType::FRUSTUM:
			addToBatchTest<BoundingShapeType::FRUSTUM>(*currentFrustumsCase_, *shape.get(), shapeIdx);
			break;
		default:
			// Unsupported shape type
			return;
	}

	// Increment and flush if we reached capacity
	if (++numQueuedShapes_ >= batchOfCapacity_) {
		flush();
	}
}

void BatchedIntersectionCase::init(const BoundingShape &shape, uint32_t capacity) {
	this->testShape = &shape;
	numQueued = 0;
	if (batchData->capacity < capacity) {
		batchData->resize(capacity);
	}
	if (queuedIndices.size() < capacity) {
		queuedIndices.resize(capacity);
	}
}
