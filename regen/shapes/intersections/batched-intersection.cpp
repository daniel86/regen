#include "batched-intersection.h"
#include "frustum-intersection.h"
#include "aabb-intersection.h"
#include "obb-intersection.h"
#include "sphere-intersection.h"
#include "regen/shapes/frustum.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/obb.h"

using namespace regen;

BatchedIntersectionTest::BatchedIntersectionTest() {
	// Create some structs for each indexed shape type.
	// These are shared across all cases. However, each case only uses a pair of them.
#define _idx(x) static_cast<int8_t>(x)
	shapeData_[_idx(IntersectionShapeType::SPHERE)]  = std::make_unique<IntersectionData_Sphere>();
	shapeData_[_idx(IntersectionShapeType::AABB)]    = std::make_unique<IntersectionData_AABB>();
	shapeData_[_idx(IntersectionShapeType::OBB)]     = std::make_unique<IntersectionData_OBB>();
	shapeData_[_idx(IntersectionShapeType::FRUSTUM)] = std::make_unique<IntersectionData_Frustum>();
	batchesOfShapes_[_idx(IntersectionShapeType::SPHERE)]  = std::make_unique<BatchOfSpheres>();
	batchesOfShapes_[_idx(IntersectionShapeType::AABB)]    = std::make_unique<BatchOfAABBs>();
	batchesOfShapes_[_idx(IntersectionShapeType::OBB)]     = std::make_unique<BatchOfAABBs>();
	batchesOfShapes_[_idx(IntersectionShapeType::FRUSTUM)] = std::make_unique<BatchOfFrustums>();
	for (auto &batch : batchesOfShapes_) {
		batch->resize(batchOfCapacity_);
	}
#undef _idx
	registerAllCases<IntersectionShapeType::SPHERE>();
	registerAllCases<IntersectionShapeType::AABB>();
	registerAllCases<IntersectionShapeType::OBB>();
	registerAllCases<IntersectionShapeType::FRUSTUM>();
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
	using IST = IntersectionShapeType;
	using ICT = IntersectionCaseType;
	// Assign the corresponding case buffers based on the test shape type.
	if (shapeType == BoundingShapeType::FRUSTUM) {
		frameCases_[_idx(IST::SPHERE)]  = _idx(ICT::FRUSTUM_SPHERES);
		frameCases_[_idx(IST::AABB)]    = _idx(ICT::FRUSTUM_AABBs);
		frameCases_[_idx(IST::OBB)]     = _idx(ICT::FRUSTUM_OBBs);
		frameCases_[_idx(IST::FRUSTUM)] = _idx(ICT::FRUSTUM_FRUSTUMS);
		// Update frustum test shape memory
		auto &mem = static_cast<IntersectionData_Frustum &>(*shapeData_[_idx(IntersectionShapeType::FRUSTUM)]);
		mem.update(testShape);
	} else if (shapeType == BoundingShapeType::SPHERE) {
		frameCases_[_idx(IST::SPHERE)]  = _idx(ICT::SPHERE_SPHERES);
		frameCases_[_idx(IST::AABB)]    = _idx(ICT::SPHERE_BOXES);
		frameCases_[_idx(IST::OBB)]     = _idx(ICT::SPHERE_BOXES);
		frameCases_[_idx(IST::FRUSTUM)] = _idx(ICT::SPHERE_FRUSTUMS);
		// Update sphere test shape memory
		auto &mem = static_cast<IntersectionData_Sphere &>(*shapeData_[_idx(IntersectionShapeType::SPHERE)]);
		mem.update(testShape);
	} else if (shapeType == BoundingShapeType::BOX) {
		BoundingBoxType boxType = static_cast<const BoundingBox &>(testShape).boxType();
		if (boxType == BoundingBoxType::AABB) {
			frameCases_[_idx(IST::SPHERE)]  = _idx(ICT::AABB_SPHERES);
			frameCases_[_idx(IST::AABB)]    = _idx(ICT::AABB_AABBs);
			frameCases_[_idx(IST::OBB)]     = _idx(ICT::AABB_OBBs);
			frameCases_[_idx(IST::FRUSTUM)] = _idx(ICT::AABB_FRUSTUMS);
			// Update AABB test shape memory
			auto &mem = static_cast<IntersectionData_AABB &>(*shapeData_[_idx(IntersectionShapeType::AABB)]);
			mem.update(testShape);
		} else { // OBB
			frameCases_[_idx(IST::SPHERE)]  = _idx(ICT::OBB_SPHERES);
			frameCases_[_idx(IST::AABB)]    = _idx(ICT::OBB_BOXES);
			frameCases_[_idx(IST::OBB)]     = _idx(ICT::OBB_BOXES);
			frameCases_[_idx(IST::FRUSTUM)] = _idx(ICT::OBB_FRUSTUMS);
			// Update OBB test shape memory
			auto &mem = static_cast<IntersectionData_OBB &>(*shapeData_[_idx(IntersectionShapeType::OBB)]);
			mem.update(testShape);
		}
	}
#undef _idx
	cases_[frameCases_[0]]->init(testShape, batchOfCapacity_);
	cases_[frameCases_[1]]->init(testShape, batchOfCapacity_);
	cases_[frameCases_[2]]->init(testShape, batchOfCapacity_);
	cases_[frameCases_[3]]->init(testShape, batchOfCapacity_);
}

inline IntersectionShapeType getShapeType(const ref_ptr<BoundingShape> &shape) {
	switch (shape->shapeType()) {
		case BoundingShapeType::SPHERE:
			return IntersectionShapeType::SPHERE;
		case BoundingShapeType::BOX: {
			if (static_cast<const BoundingBox *>(shape.get())->isAABB()) {
				return IntersectionShapeType::AABB;
			} else {
				return IntersectionShapeType::OBB;
			}
		}
		case BoundingShapeType::FRUSTUM:
			return IntersectionShapeType::FRUSTUM;
		default:
			return IntersectionShapeType::LAST; // should not happen
	}
}

void BatchedIntersectionTest::push(uint32_t shapeIdx) {
	auto &shape = (*indexedShapes_)[shapeIdx];
	const IntersectionShapeType shapeType = getShapeType(shape);
	auto &caseBuffer = cases_[frameCases_[static_cast<uint32_t>(shapeType)]];
	caseBuffer->push(shape, shapeIdx);
	numQueuedShapes_ += 1u;
	// Flush if we reached capacity
	if (numQueuedShapes_ >= batchOfCapacity_) flush();
}

void BatchedIntersectionTest::flush() {
	static constexpr int NUM_CASES = static_cast<int>(IntersectionShapeType::LAST);
	for (uint32_t i=0; i<NUM_CASES; i++) {
		cases_[frameCases_[i]]->flush();
	}
	numQueuedShapes_ = 0u;
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
	doInit(*this, shape);
}

void BatchedIntersectionCase::push(const ref_ptr<BoundingShape> &shape, uint32_t shapeIdx) {
	queuedIndices[numQueued] = shapeIdx;
	batchData->push(*shape.get(), numQueued);
	++numQueued;
}
