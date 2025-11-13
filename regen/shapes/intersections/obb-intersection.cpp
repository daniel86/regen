#include "obb-intersection.h"
#include "regen/shapes/frustum.h"

using namespace regen;

void regen::shapes::flush_OBB_Spheres(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto &testShape = *static_cast<const OBB *>(td.testShape);

	// bounding box in base pose
	const Vec3f &obbCenter = testShape.tfOrigin();
	const Vec3f halfSize = (testShape.baseBounds().max - testShape.baseBounds().min) * 0.5f;
	// transformed axes of the OBB
	const Vec3f *obbAxes = testShape.boxAxes();

	auto *batchData = static_cast<BatchOfSpheres *>(td.batchData);
	const float *d_spherePosX = batchData->posX.data();
	const float *d_spherePosY = batchData->posY.data();
	const float *d_spherePosZ = batchData->posZ.data();
	const float *d_sphereRadius = batchData->radius.data();

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the sphere data into SIMD registers, we need 4 registers.
		const BatchOf_float spherePosX = BatchOf_float::loadAligned(d_spherePosX + queuedIdx);
		const BatchOf_float spherePosY = BatchOf_float::loadAligned(d_spherePosY + queuedIdx);
		const BatchOf_float spherePosZ = BatchOf_float::loadAligned(d_spherePosZ + queuedIdx);
		BatchOf_float sphereRadius = BatchOf_float::loadAligned(d_sphereRadius + queuedIdx);
		// r = r * r
		sphereRadius *= sphereRadius;

		// Find closest point on OBB to sphere center, start with:
		// closest = obbCenter
		BatchOf_float closestX = BatchOf_float::fromScalar(obbCenter.x);
		BatchOf_float closestY = BatchOf_float::fromScalar(obbCenter.y);
		BatchOf_float closestZ = BatchOf_float::fromScalar(obbCenter.z);
		// delta = p1 - obbCenter
		BatchOf_float deltaX = spherePosX - closestX;
		BatchOf_float deltaY = spherePosY - closestY;
		BatchOf_float deltaZ = spherePosZ - closestZ;

		// We need to project delta onto each OBB axis, clamp to halfSize, and accumulate
		for (int i = 0; i < 3; ++i) {
			// axis = obbAxes[i]
			const Vec3f &axis = obbAxes[i];
			BatchOf_float halfSizeOnAxis = BatchOf_float::fromScalar(halfSize[i]);
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
		deltaX = closestX - spherePosX;
		deltaY = closestY - spherePosY;
		deltaZ = closestZ - spherePosZ;
		BatchOf_float tmp = (deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
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
		float radiusSq = d_sphereRadius[queuedIdx] * d_sphereRadius[queuedIdx];
		// compute the closest point on OBB to sphere center
		Vec3f closest = testShape.closestPointOnSurface(p1);
		if ((closest - p1).lengthSquared() <= radiusSq) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_OBB_AABBs(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto &testShape = *static_cast<const OBB *>(td.testShape);

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

	auto *batchData = static_cast<BatchOfAABBs *>(td.batchData);
	const float *d_aabbMinX = batchData->minX.data();
	const float *d_aabbMinY = batchData->minY.data();
	const float *d_aabbMinZ = batchData->minZ.data();
	const float *d_aabbMaxX = batchData->maxX.data();
	const float *d_aabbMaxY = batchData->maxY.data();
	const float *d_aabbMaxZ = batchData->maxZ.data();

	const BatchOf_float obbCenterX = BatchOf_float::fromScalar(obbCenter.x);
	const BatchOf_float obbCenterY = BatchOf_float::fromScalar(obbCenter.y);
	const BatchOf_float obbCenterZ = BatchOf_float::fromScalar(obbCenter.z);
	const BatchOf_float obbRadiusX = BatchOf_float::fromScalar(obbRadius[0]);
	const BatchOf_float obbRadiusY = BatchOf_float::fromScalar(obbRadius[1]);
	const BatchOf_float obbRadiusZ = BatchOf_float::fromScalar(obbRadius[2]);

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
		//		((obbCenter - r) < aabbMax) &&
		//		((obbCenter + r) > aabbMin)
		const BatchOf_float hasOverlap =
				// X axis overlap test
			(obbCenterX - obbRadiusX < aabbMaxX) &&
			(obbCenterX + obbRadiusX > aabbMinX) &&
				// Y axis overlap test
			(obbCenterY - obbRadiusY < aabbMaxY) &&
			(obbCenterY + obbRadiusY > aabbMinY) &&
				// Z axis overlap test
			(obbCenterZ - obbRadiusZ < aabbMaxZ) &&
			(obbCenterZ + obbRadiusZ > aabbMinZ);

		// Convert lanes (1/0) to bitmask value
		uint8_t mask = hasOverlap.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
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
	// TODO: Consider adding an early-out by bounding sphere
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
