#include "bounding-sphere.h"
#include "regen/objects/primitives/sphere.h"
#include <regen/math/simd.h>

using namespace regen;

#define REGEN_SPHERE_BATCH_DATA(name, batch) \
	auto* __restrict name##_posX = static_cast<float*>(__builtin_assume_aligned(batch.posX().data(), 32)); \
	auto* __restrict name##_posY = static_cast<float*>(__builtin_assume_aligned(batch.posY().data(), 32)); \
	auto* __restrict name##_posZ = static_cast<float*>(__builtin_assume_aligned(batch.posZ().data(), 32)); \
	auto* __restrict name##_radius = static_cast<float*>(__builtin_assume_aligned(batch.radius().data(), 32))

static Vec3f computeBasePosition(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts) {
	if (mesh.get() && parts.empty()) {
		return mesh->centerPosition();
	}
	Vec3f min, max;
	if (mesh.get()) {
		min = mesh->minPosition();
		max = mesh->maxPosition();
	}
	for (const auto &part : parts) {
		if (part.get()) {
			min.setMin(part->minPosition());
			max.setMax(part->maxPosition());
		}
	}
	return (min + max) * 0.5;
}

BoundingSphere::BoundingSphere(const Vec3f &basePosition, float radius)
		: BatchedBoundingShape(BoundingShapeType::SPHERE),
		  basePosition_(basePosition) {
	radiusSquared_ = radius * radius;
	globalBatchData_.radius()[globalIndex_] = radius;
}

BoundingSphere::BoundingSphere(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts, float radius)
		: BatchedBoundingShape(BoundingShapeType::SPHERE, mesh, parts),
		  basePosition_(computeBasePosition(mesh, parts)) {
	radius = radius > 0.0f ? radius : computeRadius(mesh, parts);
	if (auto sphereMesh = dynamic_cast<Sphere *>(mesh.get())) {
		radius = sphereMesh->radius();
	}
	radiusSquared_ = radius * radius;
	globalBatchData_.radius()[globalIndex_] = radius;
}

void BoundingSphere::updateBaseBounds(const Vec3f &min, const Vec3f &max) {
	basePosition_ = (min + max) * 0.5 + baseOffset_;
	float radius;
	if (auto sphereMesh = dynamic_cast<Sphere *>(mesh_.get())) {
		radius = sphereMesh->radius();
	} else {
		radius = (max - basePosition_).length();
	}
	radiusSquared_ = radius * radius;
	globalBatchData_.radius()[globalIndex_] = radius;
}

float BoundingSphere::computeRadius(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts) const {
	Vec3f min, max;
	if (mesh_.get()) {
		if (parts.empty()) {
			if (auto sphereMesh = dynamic_cast<Sphere *>(mesh.get())) {
				return sphereMesh->radius();
			}
		}
		min = mesh_->minPosition();
		max = mesh_->maxPosition();
	}
	for (const auto &part : parts) {
		if (part.get()) {
			min.setMin(part->minPosition());
			max.setMax(part->maxPosition());
		}
	}
	return (max - basePosition_).length();
}

bool BoundingSphere::updateTransform(bool forceUpdate) {
	if (!forceUpdate && tfStamp() == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = tfStamp();
		updateShapeOrigin();
		return true;
	}
}

void BoundingSphere::updateShapeOrigin() {
	tfOrigin_ = basePosition_ + translation();
	globalBatchData_.posX()[globalIndex_] = tfOrigin_.x;
	globalBatchData_.posY()[globalIndex_] = tfOrigin_.y;
	globalBatchData_.posZ()[globalIndex_] = tfOrigin_.z;
}

Vec3f BoundingSphere::closestPointOnSurface(const Vec3f &point) const {
	const Vec3f &p = tfOrigin();
	Vec3f d = point - p;
	if (d.lengthSquared() < 1e-6f) {
		return p + Vec3f(0, 0, radius());
	} else {
		d.normalize();
		return p + d * radius();
	}
}

bool BoundingSphere::hasIntersectionWithSphere(const BoundingSphere &other) const {
	const Vec3f &p_this = tfOrigin();
	const Vec3f &p_other = other.tfOrigin();
	const float r_sum = radiusSquared_ + other.radiusSquared_;
	return (p_this - p_other).lengthSquared() <= (r_sum * r_sum);
}

void BoundingSphere::batchTest_Spheres(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);

	REGEN_SPHERE_BATCH_DATA(t, testShape->globalBatchData_t());
	const uint32_t t_idx = testShape->globalIndex();

	auto *batchData = static_cast<const BatchOfSpheres *>(td.batchData);
	const float *d_spherePosX = batchData->posX().data();
	const float *d_spherePosY = batchData->posY().data();
	const float *d_spherePosZ = batchData->posZ().data();
	const float *d_sphereRadius = batchData->radius().data();

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the sphere data into SIMD registers
		BatchOf_float spherePosX   = BatchOf_float::loadAligned(d_spherePosX + queuedIdx);
		BatchOf_float spherePosY   = BatchOf_float::loadAligned(d_spherePosY + queuedIdx);
		BatchOf_float spherePosZ   = BatchOf_float::loadAligned(d_spherePosZ + queuedIdx);
		BatchOf_float sphereRadius = BatchOf_float::loadAligned(d_sphereRadius + queuedIdx);
		// r_sum = r1 + r2
		sphereRadius += t_radius[t_idx];
		// r_sum = r_sum * r_sum
		sphereRadius *= sphereRadius;
		// delta = p0 - p1
		spherePosX -= t_posX[t_idx];
		spherePosY -= t_posY[t_idx];
		spherePosZ -= t_posZ[t_idx];
		// tmp = delta(dot)delta = lengthSquared(delta)
		BatchOf_float tmp = (
			(spherePosX * spherePosX) +
			(spherePosY * spherePosY) +
			(spherePosZ * spherePosZ));
		// tmp = (delta(dot)delta) < r_sum*r_sum
		tmp = (tmp < sphereRadius);
		// Convert lanes (1/0) to bitmask value
		uint8_t mask = tmp.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		const Vec3f delta(
			d_spherePosX[queuedIdx] - t_posX[t_idx],
			d_spherePosY[queuedIdx] - t_posY[t_idx],
			d_spherePosZ[queuedIdx] - t_posZ[t_idx]);
		// r_sum = r1 + r2
		const float r_sum = testShape->radius() + d_sphereRadius[queuedIdx];
		// test if squared distance between centers <= squared sum of radii
		if (delta.lengthSquared() <= (r_sum * r_sum)) {
			td.hits->push(itemIdx);
		}
	}
}

bool BoundingSphere::hasIntersectionWithAABB(const AABB &box) const {
	const Vec3f &p_this = tfOrigin();
	const Vec3f &p_other = box.tfOrigin();
	if ((p_this - p_other).lengthSquared() <= radiusSquared_) {
		return true;
	}
	Vec3f closestPoint = box.closestPointOnSurface(p_this);
	return (closestPoint - p_this).lengthSquared() <= radiusSquared_;
}

void BoundingSphere::batchTest_AABBs(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);

	REGEN_SPHERE_BATCH_DATA(t, testShape->globalBatchData_t());
	const uint32_t t_idx = testShape->globalIndex();

	auto *batchData = static_cast<const BatchOfAABBs *>(td.batchData);
	const float *d_aabbMinX = batchData->minX().data();
	const float *d_aabbMinY = batchData->minY().data();
	const float *d_aabbMinZ = batchData->minZ().data();
	const float *d_aabbMaxX = batchData->maxX().data();
	const float *d_aabbMaxY = batchData->maxY().data();
	const float *d_aabbMaxZ = batchData->maxZ().data();

	// Load sphere data into SIMD registers, we need 4 registers.
	const BatchOf_float pX = BatchOf_float::fromScalar(t_posX[t_idx]);
	const BatchOf_float pY = BatchOf_float::fromScalar(t_posY[t_idx]);
	const BatchOf_float pZ = BatchOf_float::fromScalar(t_posZ[t_idx]);
	const BatchOf_float r  = BatchOf_float::fromScalar(t_radius[t_idx]);

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the AABB data into SIMD registers, 6 registers.
		const BatchOf_float aabbMinX = BatchOf_float::loadAligned(d_aabbMinX + queuedIdx);
		const BatchOf_float aabbMinY = BatchOf_float::loadAligned(d_aabbMinY + queuedIdx);
		const BatchOf_float aabbMinZ = BatchOf_float::loadAligned(d_aabbMinZ + queuedIdx);
		const BatchOf_float aabbMaxX = BatchOf_float::loadAligned(d_aabbMaxX + queuedIdx);
		const BatchOf_float aabbMaxY = BatchOf_float::loadAligned(d_aabbMaxY + queuedIdx);
		const BatchOf_float aabbMaxZ = BatchOf_float::loadAligned(d_aabbMaxZ + queuedIdx);
		// Intersection test:
		//		((spherePos + sphereRadius) > aabbMin) &&
		//		((spherePos - sphereRadius) < aabbMax)
		const BatchOf_float isInside = (
			// x axis
			((pX + r) > aabbMinX) && ((pX - r) < aabbMaxX) &&
			// y axis
			((pY + r) > aabbMinY) && ((pY - r) < aabbMaxY) &&
			// z axis
			((pZ + r) > aabbMinZ) && ((pZ - r) < aabbMaxZ));
		// Convert lanes (1/0) to bitmask value
		uint8_t mask = isInside.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		bool isOutside =
			t_posX[t_idx] + t_radius[t_idx] < d_aabbMinX[queuedIdx] ||
			t_posX[t_idx] - t_radius[t_idx] > d_aabbMaxX[queuedIdx] ||
			t_posY[t_idx] + t_radius[t_idx] < d_aabbMinY[queuedIdx] ||
			t_posY[t_idx] - t_radius[t_idx] > d_aabbMaxY[queuedIdx] ||
			t_posZ[t_idx] + t_radius[t_idx] < d_aabbMinZ[queuedIdx] ||
			t_posZ[t_idx] - t_radius[t_idx] > d_aabbMaxZ[queuedIdx];
		if (!isOutside) {
			td.hits->push(itemIdx);
		}
	}
}

bool BoundingSphere::hasIntersectionWithOBB(const OBB &box) const {
	const Vec3f &p_this = tfOrigin();
	const Vec3f &p_other = box.tfOrigin();
	if ((p_this - p_other).lengthSquared() <= radiusSquared_) {
		return true;
	}
	Vec3f closestPoint = box.closestPointOnSurface(p_this);
	return (closestPoint - p_this).lengthSquared() <= radiusSquared_;
}

void BoundingSphere::batchTest_OBBs(BatchedIntersectionCase &td) {
	auto *shapes = td.indexedShapes->data();
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);

	REGEN_SPHERE_BATCH_DATA(t, testShape->globalBatchData_t());
	const uint32_t t_idx = testShape->globalIndex();
	const float sphereRadiusSq = t_radius[t_idx] * t_radius[t_idx];

	auto *batchData = static_cast<const BatchOfOBBs *>(td.batchData);
	const float *d_obbCenterX = batchData->centerX().data();
	const float *d_obbCenterY = batchData->centerY().data();
	const float *d_obbCenterZ = batchData->centerZ().data();
	const float *d_obbHalfSizeX = batchData->halfSizeX().data();
	const float *d_obbHalfSizeY = batchData->halfSizeY().data();
	const float *d_obbHalfSizeZ = batchData->halfSizeZ().data();
	auto d_axes = batchData->axes();

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load some of the ABB data into SIMD registers, too many registers are needed so
		// we leave out the axes and rather load them in a loop below.
		BatchOf_float closestX = BatchOf_float::loadAligned(d_obbCenterX + queuedIdx);
		BatchOf_float closestY = BatchOf_float::loadAligned(d_obbCenterY + queuedIdx);
		BatchOf_float closestZ = BatchOf_float::loadAligned(d_obbCenterZ + queuedIdx);
		const BatchOf_float obbHalfSize[3] = {
			BatchOf_float::loadAligned(d_obbHalfSizeX + queuedIdx),
			BatchOf_float::loadAligned(d_obbHalfSizeY + queuedIdx),
			BatchOf_float::loadAligned(d_obbHalfSizeZ + queuedIdx)};

		// Find closest point on OBB to sphere center, start with:
		// closest = obbCenter (we will just accumulate in td.batch_obbCenterX)
		// delta = p1 - obbCenter
		BatchOf_float deltaX = BatchOf_float::fromScalar(t_posX[t_idx]) - closestX;
		BatchOf_float deltaY = BatchOf_float::fromScalar(t_posY[t_idx]) - closestY;
		BatchOf_float deltaZ = BatchOf_float::fromScalar(t_posZ[t_idx]) - closestZ;

		// We need to project delta onto each OBB axis, clamp to halfSize, and accumulate
		for (int i = 0; i < 3; ++i) {
			// axis = obbAxes[i]
			const auto &axisBatch = d_axes[i];
			const BatchOf_float axisX = BatchOf_float::loadAligned(axisBatch.x.data() + queuedIdx);
			const BatchOf_float axisY = BatchOf_float::loadAligned(axisBatch.y.data() + queuedIdx);
			const BatchOf_float axisZ = BatchOf_float::loadAligned(axisBatch.z.data() + queuedIdx);
			// dist = delta.dot(axis)
			BatchOf_float dist = (deltaX * axisX) + (deltaY * axisY) + (deltaZ * axisZ);
			// dist = clamp(dist, -halfSize, halfSize)
			const BatchOf_float halfSizeOnAxis(obbHalfSize[i]);
			dist.c = simd::min_ps(simd::max_ps(dist.c, -halfSizeOnAxis.c), halfSizeOnAxis.c);
			// closest += axis * clamped, use fused-multiply-add
			closestX.c = simd::mul_add_ps(dist.c, axisX.c, closestX.c);
			closestY.c = simd::mul_add_ps(dist.c, axisY.c, closestY.c);
			closestZ.c = simd::mul_add_ps(dist.c, axisZ.c, closestZ.c);
		}
		// delta = closest - p1
		deltaX = closestX - BatchOf_float::fromScalar(t_posX[t_idx]);
		deltaY = closestY - BatchOf_float::fromScalar(t_posY[t_idx]);
		deltaZ = closestZ - BatchOf_float::fromScalar(t_posZ[t_idx]);
		BatchOf_float tmp = (deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
		// tmp = (delta(dot)delta) < r_sum*r_sum
		tmp = (tmp < BatchOf_float::fromScalar(sphereRadiusSq));

		// Convert lanes (1/0) to bitmask value
		uint8_t mask = tmp.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		const OBB &box = *static_cast<OBB *>(shapes[itemIdx].get());
		if (testShape->hasIntersectionWithOBB(box)) {
			td.hits->push(itemIdx);
		}
	}
}

void BoundingSphere::batchTest_Frustums(BatchedIntersectionCase &td) {
	// note: index shapes are rarely frustum, so no SIMD optimization here.
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const Frustum &frustum = *static_cast<Frustum *>(shapes[itemIdx].get());
		if (frustum.hasIntersectionWithSphere(*testShape)) {
			td.hits->push(itemIdx);
		}
	}
}
