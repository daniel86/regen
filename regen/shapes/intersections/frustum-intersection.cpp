#include "frustum-intersection.h"
#include "regen/shapes/frustum.h"

namespace regen {
	static constexpr int NUM_FRUSTUM_PLANES = 6;
}

void regen::shapes::flush_Frustum_Spheres(BatchedIntersectionCase &td) {
	// Process numQueuedItems_ nodes from the queue, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	auto *shapeData = static_cast<IntersectionData_Frustum *>(td.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *batchData = static_cast<BatchOfSpheres *>(td.batchData);
	const float *d_spherePosX = batchData->posX.data();
	const float *d_spherePosY = batchData->posY.data();
	const float *d_spherePosZ = batchData->posZ.data();
	const float *d_sphereRadius = batchData->radius.data();

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the sphere data into SIMD registers
		const BatchOf_float spherePosX   = BatchOf_float::loadAligned(d_spherePosX + queuedIdx);
		const BatchOf_float spherePosY   = BatchOf_float::loadAligned(d_spherePosY + queuedIdx);
		const BatchOf_float spherePosZ   = BatchOf_float::loadAligned(d_spherePosZ + queuedIdx);
		const BatchOf_float sphereRadius = BatchOf_float::loadAligned(d_sphereRadius + queuedIdx);
		// We'll accumulate a boolean vector as mask; init to true
		BatchOf_float hasIntersection = BatchOf_float::allOnes();

		for (int p = 0; p < NUM_FRUSTUM_PLANES; ++p) {
			// tmp = n(dot)p + r
			const BatchOf_float n_dot_p =
				(spherePosX * shapeData->planes[p].x) +
				(spherePosY * shapeData->planes[p].y) +
				(spherePosZ * shapeData->planes[p].z) +
				sphereRadius;
			// (partially) inside if: (n_dot_p + r - w) > 0
			hasIntersection &= (n_dot_p > shapeData->planes[p].w);
			// early break if all lanes are dead
			if (hasIntersection.isAllZero()) break;
		}

		// Convert survived lanes to bitmask value
		uint8_t mask = hasIntersection.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		bool isOutside = false;
		for (int p = 0; p < NUM_FRUSTUM_PLANES; ++p) {
			// tmp = n(dot)p + r
			float n_dot_p =
				(d_spherePosX[queuedIdx] * shapeData->planes[p].x) +
				(d_spherePosY[queuedIdx] * shapeData->planes[p].y) +
				(d_spherePosZ[queuedIdx] * shapeData->planes[p].z) +
				d_sphereRadius[queuedIdx];
			// fully outside if: (n_dot_p + r - w) < 0
			if (n_dot_p < shapeData->planes[p].w) {
				isOutside = true; // Completely outside
				break;
			}
		}
		if (!isOutside) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Frustum_AABBs(BatchedIntersectionCase &td) {
	auto *shapeData = static_cast<IntersectionData_Frustum *>(td.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *batchData = static_cast<BatchOfAABBs *>(td.batchData);
	const float *d_aabbMinX = batchData->minX.data();
	const float *d_aabbMinY = batchData->minY.data();
	const float *d_aabbMinZ = batchData->minZ.data();
	const float *d_aabbMaxX = batchData->maxX.data();
	const float *d_aabbMaxY = batchData->maxY.data();
	const float *d_aabbMaxZ = batchData->maxZ.data();

	int32_t queuedIdx = 0;
	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the AABB data into SIMD registers
		const BatchOf_float aabbMinX = BatchOf_float::loadAligned(d_aabbMinX + queuedIdx);
		const BatchOf_float aabbMinY = BatchOf_float::loadAligned(d_aabbMinY + queuedIdx);
		const BatchOf_float aabbMinZ = BatchOf_float::loadAligned(d_aabbMinZ + queuedIdx);
		const BatchOf_float aabbMaxX = BatchOf_float::loadAligned(d_aabbMaxX + queuedIdx);
		const BatchOf_float aabbMaxY = BatchOf_float::loadAligned(d_aabbMaxY + queuedIdx);
		const BatchOf_float aabbMaxZ = BatchOf_float::loadAligned(d_aabbMaxZ + queuedIdx);
		// We'll accumulate a boolean vector as mask; init to true
		BatchOf_float hasIntersection = BatchOf_float::allOnes();

		for (int p = 0; p < NUM_FRUSTUM_PLANES; ++p) {
			// Select vertex farthest from the plane in direction of the plane normal.
			// If this point is behind the plane, the AABB must be outside the frustum.
			const BatchOf_float px = shapeData->planes[p].x < 0.0f ? aabbMinX : aabbMaxX;
			const BatchOf_float py = shapeData->planes[p].y < 0.0f ? aabbMinY : aabbMaxY;
			const BatchOf_float pz = shapeData->planes[p].z < 0.0f ? aabbMinZ : aabbMaxZ;
			// tmp = n(dot)p
			const BatchOf_float n_dot_p =
				(px * shapeData->planes[p].x) +
				(py * shapeData->planes[p].y) +
				(pz * shapeData->planes[p].z);
			hasIntersection &= (n_dot_p > shapeData->planes[p].w);
			// early break if all lanes are dead
			if (hasIntersection.isAllZero()) break;
		}

		// Convert survived lanes to bitmask value
		uint8_t mask = hasIntersection.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		bool isOutside = false;
		for (unsigned int p = 0u; p < NUM_FRUSTUM_PLANES; ++p) {
			// Select vertex farthest from the plane in direction of the plane normal.
			// If this point is behind the plane, the AABB must be outside of the frustum.
			const Vec3f farthest(
				shapeData->planes[p].x < 0.0 ? d_aabbMinX[queuedIdx] : d_aabbMaxX[queuedIdx],
				shapeData->planes[p].y < 0.0 ? d_aabbMinY[queuedIdx] : d_aabbMaxY[queuedIdx],
				shapeData->planes[p].z < 0.0 ? d_aabbMinZ[queuedIdx] : d_aabbMaxZ[queuedIdx]);
			const float n_dot_p = (
				shapeData->planes[p].x * farthest.x +
				shapeData->planes[p].y * farthest.y +
				shapeData->planes[p].z * farthest.z);
			if (n_dot_p < shapeData->planes[p].w) {
				isOutside = true; // Completely outside
				break;
			}
		}
		if (!isOutside) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Frustum_OBBs(BatchedIntersectionCase &td) {
	auto *shapes = td.indexedShapes->data();
	auto *frustum = static_cast<IntersectionData_Frustum *>(td.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);

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
		const BatchOf_float obbCenterX = BatchOf_float::loadAligned(d_obbCenterX + queuedIdx);
		const BatchOf_float obbCenterY = BatchOf_float::loadAligned(d_obbCenterY + queuedIdx);
		const BatchOf_float obbCenterZ = BatchOf_float::loadAligned(d_obbCenterZ + queuedIdx);
		const BatchOf_float obbHalfSize[3] = {
			BatchOf_float::loadAligned(d_obbHalfSizeX + queuedIdx),
			BatchOf_float::loadAligned(d_obbHalfSizeY + queuedIdx),
			BatchOf_float::loadAligned(d_obbHalfSizeZ + queuedIdx)};
		// We'll accumulate a boolean vector as mask; init to true
		BatchOf_float hasIntersection = BatchOf_float::allOnes();

		for (unsigned int planeIdx = 0u; planeIdx < NUM_FRUSTUM_PLANES; ++planeIdx) {
			const auto &plane = frustum->planes[planeIdx];
			// center-to-plane distance + projected radius, radius is initially 0
			BatchOf_float dr =
				(obbCenterX * plane.x) +
				(obbCenterY * plane.y) +
				(obbCenterZ * plane.z) -
				BatchOf_float::fromScalar(plane.w);
			// Accumulate projected radius from each OBB axis
			for (uint32_t axisIdx=0u; axisIdx < 3u; ++axisIdx) {
				// Load axis into SIMD registers
				const BatchOfOBBs::AxisBatch &axisBatch = d_axes[axisIdx];
				const BatchOf_float axisX = BatchOf_float::loadAligned(axisBatch.x.data() + queuedIdx);
				const BatchOf_float axisY = BatchOf_float::loadAligned(axisBatch.y.data() + queuedIdx);
				const BatchOf_float axisZ = BatchOf_float::loadAligned(axisBatch.z.data() + queuedIdx);
				// Projected radius
				dr += obbHalfSize[axisIdx] *
					(axisX*plane.x + axisY*plane.y + axisZ*plane.z).abs();
			}
			hasIntersection &= dr.isPositive();
			// early break if all lanes are dead
			if (hasIntersection.isAllZero()) break;
		}

		// Convert survived lanes to bitmask value
		uint8_t mask = hasIntersection.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];

		const BoundingBox &obb = *static_cast<OBB *>(shapes[itemIdx].get());
		const Vec3f *obbAxes = obb.boxAxes();
		const Vec3f &obbCenter = obb.tfOrigin();
		const Vec3f obbHalfSize = {
			d_obbHalfSizeX[queuedIdx],
			d_obbHalfSizeY[queuedIdx],
			d_obbHalfSizeZ[queuedIdx]};

		bool isOutside = false;
		for (unsigned int planeIdx = 0u; planeIdx < NUM_FRUSTUM_PLANES; ++planeIdx) {
			const auto &plane = frustum->planes[planeIdx];
			const Vec3f &n = plane.xyz_();
			// center-to-plane distance + projected radius
			float dr = n.dot(obbCenter) - plane.w +
				obbHalfSize.x * std::abs(obbAxes[0].dot(n)) +
				obbHalfSize.y * std::abs(obbAxes[1].dot(n)) +
				obbHalfSize.z * std::abs(obbAxes[2].dot(n));
			if (dr < 0.0f) { // completely outside
				isOutside = true;
				break;
			}
		}
		if (!isOutside) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Frustum_Frustums(BatchedIntersectionCase &td) {
	// note: index shapes are rarely frustum, so no SIMD optimization here.
	auto *testShape = static_cast<const Frustum *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const Frustum &frustum = *static_cast<Frustum *>(shapes[itemIdx].get());
		if (testShape->hasIntersectionWithFrustum(frustum)) {
			td.hits->push(itemIdx);
		}
	}
}
