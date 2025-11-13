#include "batch-of-shapes.h"
#include "regen/shapes/obb.h"

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

void BatchOfOBBs::doResize(BatchOfShapes &batch, uint32_t newCapacity) {
	auto &self = static_cast<BatchOfOBBs&>(batch);
	if (newCapacity != self.capacity) {
		self.capacity = newCapacity;

		self.centerX.resize(newCapacity);
		self.centerY.resize(newCapacity);
		self.centerZ.resize(newCapacity);

		self.halfSizeX.resize(newCapacity);
		self.halfSizeY.resize(newCapacity);
		self.halfSizeZ.resize(newCapacity);

		for (uint32_t i=0; i < 3; ++i) {
			self.axes[i].x.resize(newCapacity);
			self.axes[i].y.resize(newCapacity);
			self.axes[i].z.resize(newCapacity);
		}
	}
}

void BatchOfOBBs::doPush(BatchOfShapes &batch, const BoundingShape &shape, uint32_t index) {
	auto &self = static_cast<BatchOfOBBs&>(batch);
	const auto &obb = static_cast<const OBB&>(shape);
	const auto &center = obb.tfOrigin();
	const auto *axes = obb.boxAxes();
	auto halfSize = (obb.baseBounds().max - obb.baseBounds().min) * 0.5f;

	self.centerX[index] = center.x;
	self.centerY[index] = center.y;
	self.centerZ[index] = center.z;

	self.halfSizeX[index] = halfSize.x;
	self.halfSizeY[index] = halfSize.y;
	self.halfSizeZ[index] = halfSize.z;

	for (uint32_t i=0; i < 3; ++i) {
		self.axes[i].x[index] = axes[i].x;
		self.axes[i].y[index] = axes[i].y;
		self.axes[i].z[index] = axes[i].z;
	}
}
