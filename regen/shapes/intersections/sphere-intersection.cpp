#include "sphere-intersection.h"
#include "regen/shapes/bounding-box.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"

void regen::shapes::flush_Sphere_Spheres(BatchedIntersectionCase &tid) {
	auto &td = *static_cast<BatchIntersection_Sphere_Spheres *>(&tid);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	const Vec3f &p0 = testShape->tfOrigin();
	int32_t queuedIdx = 0;

	auto *batchData = static_cast<BatchOfSpheres *>(tid.batchData);
	auto *d_spherePosX = batchData->posX.data();
	auto *d_spherePosY = batchData->posY.data();
	auto *d_spherePosZ = batchData->posZ.data();
	auto *d_sphereRadius = batchData->radius.data();

	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the sphere data into SIMD registers
		td.batch_spherePosX.load_aligned(d_spherePosX + queuedIdx);
		td.batch_spherePosY.load_aligned(d_spherePosY + queuedIdx);
		td.batch_spherePosZ.load_aligned(d_spherePosZ + queuedIdx);
		td.batch_sphereRadius.load_aligned(d_sphereRadius + queuedIdx);
		// r_sum = r1 + r2
		td.batch_sphereRadius += testShape->radius();
		// r_sum = r_sum * r_sum
		td.batch_sphereRadius *= td.batch_sphereRadius;
		// delta = p0 - p1
		td.batch_spherePosX -= p0.x;
		td.batch_spherePosY -= p0.y;
		td.batch_spherePosZ -= p0.z;
		// tmp = delta(dot)delta = lengthSquared(delta)
		BatchOf_float tmp = (
			(td.batch_spherePosX * td.batch_spherePosX) +
			(td.batch_spherePosY * td.batch_spherePosY) +
			(td.batch_spherePosZ * td.batch_spherePosZ));
		// tmp = (delta(dot)delta) < r_sum*r_sum
		tmp = (tmp < td.batch_sphereRadius);
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
		Vec3f p1(
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

void regen::shapes::flush_Sphere_AABBs(BatchedIntersectionCase &tid) {
	auto &td = *static_cast<BatchIntersection_Sphere_AABBs *>(&tid);
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	int32_t queuedIdx = 0;

	auto *batchData = static_cast<BatchOfAABBs *>(tid.batchData);
	auto *d_aabbMinX = batchData->minX.data();
	auto *d_aabbMinY = batchData->minY.data();
	auto *d_aabbMinZ = batchData->minZ.data();
	auto *d_aabbMaxX = batchData->maxX.data();
	auto *d_aabbMaxY = batchData->maxY.data();
	auto *d_aabbMaxZ = batchData->maxZ.data();

	auto &spherePos = testShape->tfOrigin();
	const float sphereRadius = testShape->radius();

	{
		// Load sphere data into SIMD registers, we need 4 registers.
		BatchOf_float pX(spherePos.x);
		BatchOf_float pY(spherePos.y);
		BatchOf_float pZ(spherePos.z);
		BatchOf_float r(sphereRadius);
		for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
			// Load the AABB data into SIMD registers, 6 registers.
			td.batch_aabbMinX.load_aligned(d_aabbMinX + queuedIdx);
			td.batch_aabbMinY.load_aligned(d_aabbMinY + queuedIdx);
			td.batch_aabbMinZ.load_aligned(d_aabbMinZ + queuedIdx);
			td.batch_aabbMaxX.load_aligned(d_aabbMaxX + queuedIdx);
			td.batch_aabbMaxY.load_aligned(d_aabbMaxY + queuedIdx);
			td.batch_aabbMaxZ.load_aligned(d_aabbMaxZ + queuedIdx);
			// Intersection test:
			//		((spherePos + sphereRadius) > aabbMin) &&
			//		((spherePos - sphereRadius) < aabbMax)
			BatchOf_float isInside = (
				// x axis
				((pX + r) > td.batch_aabbMinX) &&
				((pX - r) < td.batch_aabbMaxX) &&
				// y axis
				((pY + r) > td.batch_aabbMinY) &&
				((pY - r) < td.batch_aabbMaxY) &&
				// z axis
				((pZ + r) > td.batch_aabbMinZ) &&
				((pZ - r) < td.batch_aabbMaxZ));
			// Convert lanes (1/0) to bitmask value
			uint8_t mask = isInside.toBitmask8();
			while (mask) {
				int bitIndex = simd::nextBitIndex<uint8_t>(mask);
				td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
			}
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
	// TODO: Support SIMD on this path.
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
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
