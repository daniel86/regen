#include "obb-intersection.h"
#include "regen/shapes/frustum.h"

using namespace regen;

void regen::shapes::flush_OBB_Spheres(BatchedIntersectionCase &tid) {
	auto &td = *static_cast<BatchIntersection_OBB_Spheres *>(&tid);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto &testShape = *static_cast<const OBB *>(td.testShape);
	int32_t queuedIdx = 0;

	// bounding box in base pose
	auto &obbBaseMin = testShape.baseBounds().min;
	auto &obbBaseMax = testShape.baseBounds().max;
	auto &obbCenter = testShape.tfOrigin();
	auto halfSize = (obbBaseMax - obbBaseMin) * 0.5f;
	// transformed axes of the OBB
	auto *obbAxes = testShape.boxAxes();

	auto *batchData = static_cast<BatchOfSpheres *>(tid.batchData);
	auto *d_spherePosX = batchData->posX.data();
	auto *d_spherePosY = batchData->posY.data();
	auto *d_spherePosZ = batchData->posZ.data();
	auto *d_sphereRadius = batchData->radius.data();

	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the sphere data into SIMD registers, we need 4 registers.
		td.batch_spherePosX.load_aligned(d_spherePosX + queuedIdx);
		td.batch_spherePosY.load_aligned(d_spherePosY + queuedIdx);
		td.batch_spherePosZ.load_aligned(d_spherePosZ + queuedIdx);
		td.batch_sphereRadius.load_aligned(d_sphereRadius + queuedIdx);
		// r = r * r
		td.batch_sphereRadius *= td.batch_sphereRadius;

		// Find closest point on OBB to sphere center, start with:
		// closest = obbCenter
		BatchOf_float closestX(obbCenter.x);
		BatchOf_float closestY(obbCenter.y);
		BatchOf_float closestZ(obbCenter.z);
		// delta = p1 - obbCenter
		BatchOf_float deltaX = td.batch_spherePosX - closestX;
		BatchOf_float deltaY = td.batch_spherePosY - closestY;
		BatchOf_float deltaZ = td.batch_spherePosZ - closestZ;

		// We need to project delta onto each OBB axis, clamp to halfSize, and accumulate
		for (int i = 0; i < 3; ++i) {
			// axis = obbAxes[i]
			const Vec3f &axis = obbAxes[i];
			BatchOf_float halfSizeOnAxis(halfSize[i]);
			// dist = delta.dot(axis)
			BatchOf_float dist =
				(deltaX * axis.x) +
				(deltaY * axis.y) +
				(deltaZ * axis.z);
			// dist = clamp(dist, -halfSize, halfSize)
			dist.c = simd::min_ps(simd::max_ps(dist.c, -halfSizeOnAxis.c), halfSizeOnAxis.c);
			// closest += axis * clamped, use fused-multiply-add
			closestX.c = simd::mul_add_ps(dist.c, simd::set1_ps(axis.x), closestX.c);
			closestY.c = simd::mul_add_ps(dist.c, simd::set1_ps(axis.y), closestY.c);
			closestZ.c = simd::mul_add_ps(dist.c, simd::set1_ps(axis.z), closestZ.c);
		}
		// delta = closest - p1
		deltaX = closestX - td.batch_spherePosX;
		deltaY = closestY - td.batch_spherePosY;
		deltaZ = closestZ - td.batch_spherePosZ;
		BatchOf_float tmp = (deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
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
		float radiusSq = d_sphereRadius[queuedIdx] * d_sphereRadius[queuedIdx];
		// compute the closest point on OBB to sphere center
		Vec3f closest = testShape.closestPointOnSurface(p1);
		if ((closest - p1).lengthSquared() <= radiusSq) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_OBB_Boxes(BatchedIntersectionCase &td) {
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

void regen::shapes::flush_OBB_Frustums(BatchedIntersectionCase &td) {
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
