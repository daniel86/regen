#include "batch-of-shapes.h"

using namespace regen;

void BatchOfShapes::resize(uint32_t newCapacity, bool preserveData) {
	if (newCapacity != capacity) {
		capacity = newCapacity;
		for (auto &array : soaData_) {
			array.resize(newCapacity, preserveData);
		}
	}
}

void BatchOfShapes::push(const BoundingShape &shape, uint32_t localIdx) {
	const auto &global = shape.globalBatchData();
	const uint32_t globalIdx = shape.globalIndex();
	for (size_t i = 0; i < soaData_.size(); ++i) {
		soaData_[i][localIdx] = global.soaData_[i][globalIdx];
	}
}
