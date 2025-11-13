#include "sphere-intersection.h"
#include "regen/shapes/bounding-box.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"

void regen::shapes::flush_Sphere_Spheres(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	const Vec3f &p0 = testShape->tfOrigin();

	auto *batchData = static_cast<BatchOfSpheres *>(td.batchData);
	const float *d_spherePosX = batchData->posX.data();
	const float *d_spherePosY = batchData->posY.data();
	const float *d_spherePosZ = batchData->posZ.data();
	const float *d_sphereRadius = batchData->radius.data();

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the sphere data into SIMD registers
		BatchOf_float spherePosX   = BatchOf_float::loadAligned(d_spherePosX + queuedIdx);
		BatchOf_float spherePosY   = BatchOf_float::loadAligned(d_spherePosY + queuedIdx);
		BatchOf_float spherePosZ   = BatchOf_float::loadAligned(d_spherePosZ + queuedIdx);
		BatchOf_float sphereRadius = BatchOf_float::loadAligned(d_sphereRadius + queuedIdx);
		// r_sum = r1 + r2
		sphereRadius += testShape->radius();
		// r_sum = r_sum * r_sum
		sphereRadius *= sphereRadius;
		// delta = p0 - p1
		spherePosX -= p0.x;
		spherePosY -= p0.y;
		spherePosZ -= p0.z;
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
		const Vec3f p1(
			d_spherePosX[queuedIdx],
			d_spherePosY[queuedIdx],
			d_spherePosZ[queuedIdx]);
		// r_sum = r1 + r2
		const float r_sum = testShape->radius() + d_sphereRadius[queuedIdx];
		// test if squared distance between centers <= squared sum of radii
		if ((p0-p1).lengthSquared() <= (r_sum * r_sum)) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Sphere_AABBs(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	auto &spherePos = testShape->tfOrigin();
	const float sphereRadius = testShape->radius();

	auto *batchData = static_cast<BatchOfAABBs *>(td.batchData);
	const float *d_aabbMinX = batchData->minX.data();
	const float *d_aabbMinY = batchData->minY.data();
	const float *d_aabbMinZ = batchData->minZ.data();
	const float *d_aabbMaxX = batchData->maxX.data();
	const float *d_aabbMaxY = batchData->maxY.data();
	const float *d_aabbMaxZ = batchData->maxZ.data();

	// Load sphere data into SIMD registers, we need 4 registers.
	const BatchOf_float pX = BatchOf_float::fromScalar(spherePos.x);
	const BatchOf_float pY = BatchOf_float::fromScalar(spherePos.y);
	const BatchOf_float pZ = BatchOf_float::fromScalar(spherePos.z);
	const BatchOf_float r  = BatchOf_float::fromScalar(sphereRadius);

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
			spherePos.x + sphereRadius < d_aabbMinX[queuedIdx] ||
			spherePos.x - sphereRadius > d_aabbMaxX[queuedIdx] ||
			spherePos.y + sphereRadius < d_aabbMinY[queuedIdx] ||
			spherePos.y - sphereRadius > d_aabbMaxY[queuedIdx] ||
			spherePos.z + sphereRadius < d_aabbMinZ[queuedIdx] ||
			spherePos.z - sphereRadius > d_aabbMaxZ[queuedIdx];
		if (!isOutside) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Sphere_OBBs(BatchedIntersectionCase &td) {
	auto *shapes = td.indexedShapes->data();
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	auto &spherePos = testShape->tfOrigin();
	const float sphereRadiusSq = testShape->radiusSquared();

	auto *batchData = static_cast<BatchOfOBBs *>(td.batchData);
	const float *d_obbCenterX = batchData->centerX.data();
	const float *d_obbCenterY = batchData->centerY.data();
	const float *d_obbCenterZ = batchData->centerZ.data();
	const float *d_obbHalfSizeX = batchData->halfSizeX.data();
	const float *d_obbHalfSizeY = batchData->halfSizeY.data();
	const float *d_obbHalfSizeZ = batchData->halfSizeZ.data();
	auto *d_axes = batchData->axes.data();

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
		BatchOf_float deltaX = BatchOf_float::fromScalar(spherePos.x);
		deltaX -= closestX;
		BatchOf_float deltaY = BatchOf_float::fromScalar(spherePos.y);
		deltaY -= closestY;
		BatchOf_float deltaZ = BatchOf_float::fromScalar(spherePos.z);
		deltaZ -= closestZ;

		// We need to project delta onto each OBB axis, clamp to halfSize, and accumulate
		for (int i = 0; i < 3; ++i) {
			// axis = obbAxes[i]
			const BatchOfOBBs::AxisBatch &axisBatch = d_axes[i];
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
		deltaX = closestX - BatchOf_float::fromScalar(spherePos.x);
		deltaY = closestY - BatchOf_float::fromScalar(spherePos.y);
		deltaZ = closestZ - BatchOf_float::fromScalar(spherePos.z);
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
		const BoundingBox &box = *static_cast<BoundingBox *>(shapes[itemIdx].get());
		if (testShape->hasIntersectionWithShape(box)) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Sphere_Frustums(BatchedIntersectionCase &td) {
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
