#include "aabb-intersection.h"
#include "regen/shapes/frustum.h"

void regen::shapes::flush_AABB_Spheres(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *testShape = static_cast<const AABB *>(td.testShape);
	const Vec3f &aabbMin = testShape->tfBounds().min;
	const Vec3f &aabbMax = testShape->tfBounds().max;

	auto *batchData = static_cast<BatchOfSpheres *>(td.batchData);
	const float *d_spherePosX = batchData->posX.data();
	const float *d_spherePosY = batchData->posY.data();
	const float *d_spherePosZ = batchData->posZ.data();
	const float *d_sphereRadius = batchData->radius.data();

	// load min/max aabb into SIMD registers, we need 6 registers.
	const BatchOf_float aabbMinX = BatchOf_float::fromScalar(aabbMin.x);
	const BatchOf_float aabbMinY = BatchOf_float::fromScalar(aabbMin.y);
	const BatchOf_float aabbMinZ = BatchOf_float::fromScalar(aabbMin.z);
	const BatchOf_float aabbMaxX = BatchOf_float::fromScalar(aabbMax.x);
	const BatchOf_float aabbMaxY = BatchOf_float::fromScalar(aabbMax.y);
	const BatchOf_float aabbMaxZ = BatchOf_float::fromScalar(aabbMax.z);

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the sphere data into SIMD registers, we need 4 registers.
		const BatchOf_float spherePosX = BatchOf_float::loadAligned(d_spherePosX + queuedIdx);
		const BatchOf_float spherePosY = BatchOf_float::loadAligned(d_spherePosY + queuedIdx);
		const BatchOf_float spherePosZ = BatchOf_float::loadAligned(d_spherePosZ + queuedIdx);
		const BatchOf_float sphereRadius = BatchOf_float::loadAligned(d_sphereRadius + queuedIdx);

		// Intersection test:
		//		((spherePos + sphereRadius) > aabbMin) &&
		//		((spherePos - sphereRadius) < aabbMax)
		const BatchOf_float isInside = (
			// x axis
			((spherePosX + sphereRadius) > aabbMinX) &&
			((spherePosX - sphereRadius) < aabbMaxX) &&
			// y axis
			((spherePosY + sphereRadius) > aabbMinY) &&
			((spherePosY - sphereRadius) < aabbMaxY) &&
			// z axis
			((spherePosZ + sphereRadius) > aabbMinZ) &&
			((spherePosZ - sphereRadius) < aabbMaxZ));

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

void regen::shapes::flush_AABB_AABBs(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *testShape = static_cast<const AABB *>(td.testShape);
	const Vec3f &aabbMin_0 = testShape->tfBounds().min;
	const Vec3f &aabbMax_0 = testShape->tfBounds().max;

	auto *batchData = static_cast<BatchOfAABBs *>(td.batchData);
	const float *aabbMinX_1 = batchData->minX.data();
	const float *aabbMinY_1 = batchData->minY.data();
	const float *aabbMinZ_1 = batchData->minZ.data();
	const float *aabbMaxX_1 = batchData->maxX.data();
	const float *aabbMaxY_1 = batchData->maxY.data();
	const float *aabbMaxZ_1 = batchData->maxZ.data();

	// load min/max aabb into SIMD registers, we need 6 registers.
	const BatchOf_float t_aabbMinX = BatchOf_float::fromScalar(aabbMin_0.x);
	const BatchOf_float t_aabbMinY = BatchOf_float::fromScalar(aabbMin_0.y);
	const BatchOf_float t_aabbMinZ = BatchOf_float::fromScalar(aabbMin_0.z);
	const BatchOf_float t_aabbMaxX = BatchOf_float::fromScalar(aabbMax_0.x);
	const BatchOf_float t_aabbMaxY = BatchOf_float::fromScalar(aabbMax_0.y);
	const BatchOf_float t_aabbMaxZ = BatchOf_float::fromScalar(aabbMax_0.z);

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the AABB data into SIMD registers, 6 registers.
		const BatchOf_float aabbMinX = BatchOf_float::loadAligned(aabbMinX_1 + queuedIdx);
		const BatchOf_float aabbMinY = BatchOf_float::loadAligned(aabbMinY_1 + queuedIdx);
		const BatchOf_float aabbMinZ = BatchOf_float::loadAligned(aabbMinZ_1 + queuedIdx);
		const BatchOf_float aabbMaxX = BatchOf_float::loadAligned(aabbMaxX_1 + queuedIdx);
		const BatchOf_float aabbMaxY = BatchOf_float::loadAligned(aabbMaxY_1 + queuedIdx);
		const BatchOf_float aabbMaxZ = BatchOf_float::loadAligned(aabbMaxZ_1 + queuedIdx);

		// Intersection test:
		//		(aabbA.min < aabbB.max) &&
		//		(aabbA.max > aabbB.min)
		const BatchOf_float hasIntersection =
			(t_aabbMinX < aabbMaxX) && (t_aabbMaxX > aabbMinX) &&
			(t_aabbMinY < aabbMaxY) && (t_aabbMaxY > aabbMinY) &&
			(t_aabbMinZ < aabbMaxZ) && (t_aabbMaxZ > aabbMinZ);
		// Convert lanes (1/0) to bitmask value
		uint8_t mask = hasIntersection.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		bool hasIntersection =
			aabbMin_0.x < aabbMaxX_1[queuedIdx] &&
			aabbMax_0.x > aabbMinX_1[queuedIdx] &&
			aabbMin_0.y < aabbMaxY_1[queuedIdx] &&
			aabbMax_0.y > aabbMinY_1[queuedIdx] &&
			aabbMin_0.z < aabbMaxZ_1[queuedIdx] &&
			aabbMax_0.z > aabbMinZ_1[queuedIdx];
		if (hasIntersection) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_AABB_OBBs(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *testShape = static_cast<const AABB *>(td.testShape);
	const Vec3f &aabbMin = testShape->tfBounds().min;
	const Vec3f &aabbMax = testShape->tfBounds().max;

	auto *batchData = static_cast<BatchOfOBBs *>(td.batchData);
	const float *d_obbCenterX = batchData->centerX.data();
	const float *d_obbCenterY = batchData->centerY.data();
	const float *d_obbCenterZ = batchData->centerZ.data();
	const float *d_obbHalfSizeX = batchData->halfSizeX.data();
	const float *d_obbHalfSizeY = batchData->halfSizeY.data();
	const float *d_obbHalfSizeZ = batchData->halfSizeZ.data();
	auto *d_axes = batchData->axes.data();

	const BatchOf_float aabbMinX = BatchOf_float::fromScalar(aabbMin.x);
	const BatchOf_float aabbMinY = BatchOf_float::fromScalar(aabbMin.y);
	const BatchOf_float aabbMinZ = BatchOf_float::fromScalar(aabbMin.z);
	const BatchOf_float aabbMaxX = BatchOf_float::fromScalar(aabbMax.x);
	const BatchOf_float aabbMaxY = BatchOf_float::fromScalar(aabbMax.y);
	const BatchOf_float aabbMaxZ = BatchOf_float::fromScalar(aabbMax.z);

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Accumulate projected radius from each OBB axis
		BatchOf_float obbRadiusX = BatchOf_float::allZeros();
		BatchOf_float obbRadiusY = BatchOf_float::allZeros();;
		BatchOf_float obbRadiusZ = BatchOf_float::allZeros();
		{
			BatchOf_float objHalfSize[3] = {
				BatchOf_float::loadAligned(d_obbHalfSizeX + queuedIdx),
				BatchOf_float::loadAligned(d_obbHalfSizeY + queuedIdx),
				BatchOf_float::loadAligned(d_obbHalfSizeZ + queuedIdx)};

			for (uint32_t axisIdx=0u; axisIdx < 3u; ++axisIdx) {
				// Load axis into SIMD registers
				BatchOfOBBs::AxisBatch &axisBatch = d_axes[axisIdx];
				BatchOf_float axis;
				// X axis
				axis = BatchOf_float::loadAligned(axisBatch.x.data() + queuedIdx);
				obbRadiusX += objHalfSize[axisIdx] * axis.abs();
				// Y axis
				axis = BatchOf_float::loadAligned(axisBatch.y.data() + queuedIdx);
				obbRadiusY += objHalfSize[axisIdx] * axis.abs();
				// Z axis
				axis = BatchOf_float::loadAligned(axisBatch.z.data() + queuedIdx);
				obbRadiusZ += objHalfSize[axisIdx] * axis.abs();
			}
		}

		// Intersection test:
		//		((obbCenter - r) < aabbMax) &&
		//		((obbCenter + r) > aabbMin)
		const BatchOf_float obbCenterX = BatchOf_float::loadAligned(d_obbCenterX + queuedIdx);
		const BatchOf_float obbCenterY = BatchOf_float::loadAligned(d_obbCenterY + queuedIdx);
		const BatchOf_float obbCenterZ = BatchOf_float::loadAligned(d_obbCenterZ + queuedIdx);
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
		// The OBB's projected radius
		float obbRadiusX =
			d_obbHalfSizeX[queuedIdx] * std::abs(d_axes[0].x[queuedIdx]) +
			d_obbHalfSizeY[queuedIdx] * std::abs(d_axes[1].x[queuedIdx]) +
			d_obbHalfSizeZ[queuedIdx] * std::abs(d_axes[2].x[queuedIdx]);
		float obbRadiusY =
			d_obbHalfSizeX[queuedIdx] * std::abs(d_axes[0].y[queuedIdx]) +
			d_obbHalfSizeY[queuedIdx] * std::abs(d_axes[1].y[queuedIdx]) +
			d_obbHalfSizeZ[queuedIdx] * std::abs(d_axes[2].y[queuedIdx]);
		float obbRadiusZ =
			d_obbHalfSizeX[queuedIdx] * std::abs(d_axes[0].z[queuedIdx]) +
			d_obbHalfSizeY[queuedIdx] * std::abs(d_axes[1].z[queuedIdx]) +
			d_obbHalfSizeZ[queuedIdx] * std::abs(d_axes[2].z[queuedIdx]);
		// Intersection test:
		//		((obbCenter - r) < aabbMax) &&
		//		((obbCenter + r) > aabbMin)
		if (d_obbCenterX[queuedIdx] - obbRadiusX < aabbMax.x &&
			d_obbCenterX[queuedIdx] + obbRadiusX > aabbMin.x &&
			d_obbCenterY[queuedIdx] - obbRadiusY < aabbMax.y &&
			d_obbCenterY[queuedIdx] + obbRadiusY > aabbMin.y &&
			d_obbCenterZ[queuedIdx] - obbRadiusZ < aabbMax.z &&
			d_obbCenterZ[queuedIdx] + obbRadiusZ > aabbMin.z)
		{
			auto itemIdx = td.queuedIndices[queuedIdx];
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
