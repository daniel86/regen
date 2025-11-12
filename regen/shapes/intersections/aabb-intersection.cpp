#include "aabb-intersection.h"
#include "regen/shapes/frustum.h"

void regen::shapes::flush_AABB_Spheres(BatchedIntersectionCase &tid) {
	auto &td = *static_cast<BatchIntersection_AABB_Spheres *>(&tid);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	int32_t queuedIdx = 0;

	auto *testShape = static_cast<const AABB *>(td.testShape);
	auto &aabbMin = testShape->tfBounds().min;
	auto &aabbMax = testShape->tfBounds().max;

	auto *batchData = static_cast<BatchOfSpheres *>(tid.batchData);
	auto *d_spherePosX = batchData->posX.data();
	auto *d_spherePosY = batchData->posY.data();
	auto *d_spherePosZ = batchData->posZ.data();
	auto *d_sphereRadius = batchData->radius.data();

	{
		// load min/max aabb into SIMD registers, we need 6 registers.
		BatchOf_float aabbMinX(aabbMin.x);
		BatchOf_float aabbMinY(aabbMin.y);
		BatchOf_float aabbMinZ(aabbMin.z);
		BatchOf_float aabbMaxX(aabbMax.x);
		BatchOf_float aabbMaxY(aabbMax.y);
		BatchOf_float aabbMaxZ(aabbMax.z);

		for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
			// Load the sphere data into SIMD registers, we need 4 registers.
			td.batch_spherePosX.load_aligned(d_spherePosX + queuedIdx);
			td.batch_spherePosY.load_aligned(d_spherePosY + queuedIdx);
			td.batch_spherePosZ.load_aligned(d_spherePosZ + queuedIdx);
			td.batch_sphereRadius.load_aligned(d_sphereRadius + queuedIdx);
			// Intersection test:
			//		((spherePos + sphereRadius) > aabbMin) &&
			//		((spherePos - sphereRadius) < aabbMax)
			BatchOf_float isInside = (
				// x axis
				((td.batch_spherePosX + td.batch_sphereRadius) > aabbMinX) &&
				((td.batch_spherePosX - td.batch_sphereRadius) < aabbMaxX) &&
				// y axis
				((td.batch_spherePosY + td.batch_sphereRadius) > aabbMinY) &&
				((td.batch_spherePosY - td.batch_sphereRadius) < aabbMaxY) &&
				// z axis
				((td.batch_spherePosZ + td.batch_sphereRadius) > aabbMinZ) &&
				((td.batch_spherePosZ - td.batch_sphereRadius) < aabbMaxZ));
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
			d_spherePosX[queuedIdx] + d_sphereRadius[queuedIdx] < aabbMin.x ||
			d_spherePosX[queuedIdx] - d_sphereRadius[queuedIdx] > aabbMax.x ||
			d_spherePosY[queuedIdx] + d_sphereRadius[queuedIdx] < aabbMin.y ||
			d_spherePosY[queuedIdx] - d_sphereRadius[queuedIdx] > aabbMax.y ||
			d_spherePosZ[queuedIdx] + d_sphereRadius[queuedIdx] < aabbMin.z ||
			d_spherePosZ[queuedIdx] - d_sphereRadius[queuedIdx] > aabbMax.z;
		if (!isOutside) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_AABB_AABBs(BatchedIntersectionCase &tid) {
	auto &td = *static_cast<BatchIntersection_AABB_AABBs *>(&tid);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	int32_t queuedIdx = 0;

	auto *testShape = static_cast<const AABB *>(td.testShape);
	auto &v_test_aabbMin = testShape->tfBounds().min;
	auto &v_test_aabbMax = testShape->tfBounds().max;

	auto *batchData = static_cast<BatchOfAABBs *>(tid.batchData);
	auto *v_batch_aabbMinX = batchData->minX.data();
	auto *v_batch_aabbMinY = batchData->minY.data();
	auto *v_batch_aabbMinZ = batchData->minZ.data();
	auto *v_batch_aabbMaxX = batchData->maxX.data();
	auto *v_batch_aabbMaxY = batchData->maxY.data();
	auto *v_batch_aabbMaxZ = batchData->maxZ.data();

	{	// SIMD scope
		// load min/max aabb into SIMD registers, we need 6 registers.
		BatchOf_float t_aabbMinX(v_test_aabbMin.x);
		BatchOf_float t_aabbMinY(v_test_aabbMin.y);
		BatchOf_float t_aabbMinZ(v_test_aabbMin.z);
		BatchOf_float t_aabbMaxX(v_test_aabbMax.x);
		BatchOf_float t_aabbMaxY(v_test_aabbMax.y);
		BatchOf_float t_aabbMaxZ(v_test_aabbMax.z);

		for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
			// Load the AABB data into SIMD registers, 6 registers.
			td.batch_aabbMinX.load_aligned(v_batch_aabbMinX + queuedIdx);
			td.batch_aabbMinY.load_aligned(v_batch_aabbMinY + queuedIdx);
			td.batch_aabbMinZ.load_aligned(v_batch_aabbMinZ + queuedIdx);
			td.batch_aabbMaxX.load_aligned(v_batch_aabbMaxX + queuedIdx);
			td.batch_aabbMaxY.load_aligned(v_batch_aabbMaxY + queuedIdx);
			td.batch_aabbMaxZ.load_aligned(v_batch_aabbMaxZ + queuedIdx);
			// Intersection test:
			//		(aabbA.min < aabbB.max) &&
			//		(aabbA.max > aabbB.min)
			BatchOf_float hasIntersection =
				(t_aabbMinX < td.batch_aabbMaxX) &&
				(t_aabbMaxX > td.batch_aabbMinX) &&
				(t_aabbMinY < td.batch_aabbMaxY) &&
				(t_aabbMaxY > td.batch_aabbMinY) &&
				(t_aabbMinZ < td.batch_aabbMaxZ) &&
				(t_aabbMaxZ > td.batch_aabbMinZ);
			// Convert lanes (1/0) to bitmask value
			uint8_t mask = hasIntersection.toBitmask8();
			while (mask) {
				int bitIndex = simd::nextBitIndex<uint8_t>(mask);
				td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
			}
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		bool hasIntersection =
			v_test_aabbMin.x < v_batch_aabbMaxX[queuedIdx] &&
			v_test_aabbMax.x > v_batch_aabbMinX[queuedIdx] &&
			v_test_aabbMin.y < v_batch_aabbMaxY[queuedIdx] &&
			v_test_aabbMax.y > v_batch_aabbMinY[queuedIdx] &&
			v_test_aabbMin.z < v_batch_aabbMaxZ[queuedIdx] &&
			v_test_aabbMax.z > v_batch_aabbMinZ[queuedIdx];
		if (hasIntersection) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_AABB_OBBs(BatchedIntersectionCase &td) {
	auto *testShape = static_cast<const BoundingBox *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const BoundingBox &box = *static_cast<BoundingBox *>(shapes[itemIdx].get());
		if (testShape->hasIntersectionWithBox(box)) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_AABB_Frustums(BatchedIntersectionCase &td) {
	// note: index shapes are rarely frustum, so no SIMD optimization here.
	auto *testShape = static_cast<const BoundingBox *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const Frustum &frustum = *static_cast<Frustum *>(shapes[itemIdx].get());
		if (frustum.hasIntersectionWithBox(*testShape)) {
			td.hits->push(itemIdx);
		}
	}
}
