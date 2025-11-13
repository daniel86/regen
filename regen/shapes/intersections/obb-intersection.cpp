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

void regen::shapes::flush_OBB_AABBs(BatchedIntersectionCase &tid) {
	auto &td = *static_cast<BatchIntersection_OBB_AABBs *>(&tid);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto &testShape = *static_cast<const OBB *>(td.testShape);
	int32_t queuedIdx = 0;

	// bounding box in base pose
	auto &obbCenter = testShape.tfOrigin();
	auto obbHalfSize = (testShape.baseBounds().max - testShape.baseBounds().min) * 0.5f;
	// Transformed axes of the OBB
	auto *obbAxes = testShape.boxAxes();
	const std::array<Vec3f,3> obbAbsAxes = {obbAxes[0].abs(), obbAxes[1].abs(), obbAxes[2].abs()};
	// The OBB's projected radius
	const std::array<float,3> obbRadius = {
		obbHalfSize.x * obbAbsAxes[0][0] + obbHalfSize.y * obbAbsAxes[1][0] + obbHalfSize.z * obbAbsAxes[2][0],
		obbHalfSize.x * obbAbsAxes[0][1] + obbHalfSize.y * obbAbsAxes[1][1] + obbHalfSize.z * obbAbsAxes[2][1],
		obbHalfSize.x * obbAbsAxes[0][2] + obbHalfSize.y * obbAbsAxes[1][2] + obbHalfSize.z * obbAbsAxes[2][2]};

	auto *batchData = static_cast<BatchOfAABBs *>(tid.batchData);
	auto *d_aabbMinX = batchData->minX.data();
	auto *d_aabbMinY = batchData->minY.data();
	auto *d_aabbMinZ = batchData->minZ.data();
	auto *d_aabbMaxX = batchData->maxX.data();
	auto *d_aabbMaxY = batchData->maxY.data();
	auto *d_aabbMaxZ = batchData->maxZ.data();

	{
		const BatchOf_float obbCenterX(obbCenter.x);
		const BatchOf_float obbCenterY(obbCenter.y);
		const BatchOf_float obbCenterZ(obbCenter.z);
		const BatchOf_float obbRadiusX(obbRadius[0]);
		const BatchOf_float obbRadiusY(obbRadius[1]);
		const BatchOf_float obbRadiusZ(obbRadius[2]);

		for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
			// Load the AABB data into SIMD registers, 6 registers.
			td.batch_aabbMinX.load_aligned(d_aabbMinX + queuedIdx);
			td.batch_aabbMinY.load_aligned(d_aabbMinY + queuedIdx);
			td.batch_aabbMinZ.load_aligned(d_aabbMinZ + queuedIdx);
			td.batch_aabbMaxX.load_aligned(d_aabbMaxX + queuedIdx);
			td.batch_aabbMaxY.load_aligned(d_aabbMaxY + queuedIdx);
			td.batch_aabbMaxZ.load_aligned(d_aabbMaxZ + queuedIdx);

			// Intersection test:
			//		((obbCenter - r) < aabbMax) &&
			//		((obbCenter + r) > aabbMin)
			const BatchOf_float hasOverlap =
					// X axis overlap test
				(obbCenterX - obbRadiusX < td.batch_aabbMaxX) &&
				(obbCenterX + obbRadiusX > td.batch_aabbMinX) &&
					// Y axis overlap test
				(obbCenterY - obbRadiusY < td.batch_aabbMaxY) &&
				(obbCenterY + obbRadiusY > td.batch_aabbMinY) &&
					// Z axis overlap test
				(obbCenterZ - obbRadiusZ < td.batch_aabbMaxZ) &&
				(obbCenterZ + obbRadiusZ > td.batch_aabbMinZ);

			// Convert lanes (1/0) to bitmask value
			uint8_t mask = hasOverlap.toBitmask8();
			while (mask) {
				int bitIndex = simd::nextBitIndex<uint8_t>(mask);
				td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
			}
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		if (obbCenter.x - obbRadius[0] < d_aabbMaxX[queuedIdx] &&
			obbCenter.x + obbRadius[0] > d_aabbMinX[queuedIdx] &&
			obbCenter.y - obbRadius[1] < d_aabbMaxY[queuedIdx] &&
			obbCenter.y + obbRadius[1] > d_aabbMinY[queuedIdx] &&
			obbCenter.z - obbRadius[2] < d_aabbMaxZ[queuedIdx] &&
			obbCenter.z + obbRadius[2] > d_aabbMinZ[queuedIdx])
		{
			auto itemIdx = td.queuedIndices[queuedIdx];
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_OBB_OBBs(BatchedIntersectionCase &td) {
	// note: This case cannot be handled well with AVX and its limited number
	// of registers, so we do a scalar implementation here.
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
