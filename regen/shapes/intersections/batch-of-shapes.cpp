#include "batch-of-shapes.h"

using namespace regen;

void BatchOfSpheres::doResize(BatchOfShapes &batch, uint32_t newCapacity) {
	auto &self = static_cast<BatchOfSpheres&>(batch);
	if (newCapacity != self.capacity) {
		self.capacity = newCapacity;
		self.posX.resize(newCapacity);
		self.posY.resize(newCapacity);
		self.posZ.resize(newCapacity);
		self.radius.resize(newCapacity);
	}
}

void BatchOfSpheres::doPush(BatchOfShapes &batch, const BoundingShape &shape, uint32_t index) {
	auto &self = static_cast<BatchOfSpheres&>(batch);
	const auto &sphere = static_cast<const BoundingSphere&>(shape);
	const auto &pos = sphere.tfOrigin();
	self.posX[index] = pos.x;
	self.posY[index] = pos.y;
	self.posZ[index] = pos.z;
	self.radius[index] = sphere.radius();
}

void BatchOfAABBs::doResize(BatchOfShapes &batch, uint32_t newCapacity) {
	auto &self = static_cast<BatchOfAABBs&>(batch);
	if (newCapacity != self.capacity) {
		self.capacity = newCapacity;
		self.minX.resize(newCapacity);
		self.minY.resize(newCapacity);
		self.minZ.resize(newCapacity);
		self.maxX.resize(newCapacity);
		self.maxY.resize(newCapacity);
		self.maxZ.resize(newCapacity);
	}
}

void BatchOfAABBs::doPush(BatchOfShapes &batch, const BoundingShape &shape, uint32_t index) {
	auto &self = static_cast<BatchOfAABBs&>(batch);
	const auto &box = static_cast<const BoundingBox&>(shape);
	const auto &bounds = box.tfBounds();
	self.minX[index] = bounds.min.x;
	self.minY[index] = bounds.min.y;
	self.minZ[index] = bounds.min.z;
	self.maxX[index] = bounds.max.x;
	self.maxY[index] = bounds.max.y;
	self.maxZ[index] = bounds.max.z;
}
