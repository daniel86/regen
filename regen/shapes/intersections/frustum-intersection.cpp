#include "frustum-intersection.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"
#include "regen/utility/logging.h"

namespace regen {
	static constexpr int NUM_FRUSTUM_PLANES = 6;
}

void regen::shapes::flush_Frustum_Spheres(BatchedIntersectionCase &tid) {
	// Process numQueuedItems_ nodes from the queue, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	auto &td = *static_cast<BatchIntersection_Frustum_Spheres *>(&tid);
	auto *shapeData = static_cast<IntersectionData_Frustum *>(tid.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
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
		// We'll accumulate a boolean vector "survived" as mask; init to true
		BatchOf_float survived = BatchOf_float::all_ones();

		for (int p = 0; p < NUM_FRUSTUM_PLANES; ++p) {
			// tmp = n(dot)p + r
			BatchOf_float tmp =
				(td.batch_spherePosX * shapeData->planes[p].x) +
				(td.batch_spherePosY * shapeData->planes[p].y) +
				(td.batch_spherePosZ * shapeData->planes[p].z) +
				td.batch_sphereRadius;
			// survived if: (n(dot)p - d) >= -r  -> (n(dot)p + r) >= d
			survived = survived && (tmp > shapeData->planes[p].w);
			// early break if all lanes are dead
			if (survived.isZeroMask()) break;
		}

		// Convert survived lanes to bitmask value
		uint8_t mask = survived.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		bool inside = true;
		for (int p = 0; p < NUM_FRUSTUM_PLANES; ++p) {
			// tmp = n(dot)p + r
			float tmp =
				(d_spherePosX[queuedIdx] * shapeData->planes[p].x) +
				(d_spherePosY[queuedIdx] * shapeData->planes[p].y) +
				(d_spherePosZ[queuedIdx] * shapeData->planes[p].z) +
				d_sphereRadius[queuedIdx];
			// survived if: (n(dot)p - d) >= -r  -> (n(dot)p + r) >= d
			if (tmp < shapeData->planes[p].w) {
				inside = false;
				break;
			}
		}
		if (inside) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Frustum_AABBs(BatchedIntersectionCase &tid) {
	auto &td = *static_cast<BatchIntersection_Frustum_AABBs *>(&tid);
	auto *shapeData = static_cast<IntersectionData_Frustum *>(tid.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	int32_t queuedIdx = 0;

	auto *batchData = static_cast<BatchOfAABBs *>(tid.batchData);
	auto *d_aabbMinX = batchData->minX.data();
	auto *d_aabbMinY = batchData->minY.data();
	auto *d_aabbMinZ = batchData->minZ.data();
	auto *d_aabbMaxX = batchData->maxX.data();
	auto *d_aabbMaxY = batchData->maxY.data();
	auto *d_aabbMaxZ = batchData->maxZ.data();

	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load the AABB data into SIMD registers
		td.batch_aabbMinX.load_aligned(d_aabbMinX + queuedIdx);
		td.batch_aabbMinY.load_aligned(d_aabbMinY + queuedIdx);
		td.batch_aabbMinZ.load_aligned(d_aabbMinZ + queuedIdx);
		td.batch_aabbMaxX.load_aligned(d_aabbMaxX + queuedIdx);
		td.batch_aabbMaxY.load_aligned(d_aabbMaxY + queuedIdx);
		td.batch_aabbMaxZ.load_aligned(d_aabbMaxZ + queuedIdx);
		// We'll accumulate a boolean vector "survived" as mask; init to true
		BatchOf_float survived = BatchOf_float::all_ones();

		for (int p = 0; p < NUM_FRUSTUM_PLANES; ++p) {
			// Select vertex farthest from the plane in direction of the plane normal.
			// If this point is behind the plane, the AABB must be outside the frustum.
			BatchOf_float px = shapeData->planes[p].x < 0.0f ? td.batch_aabbMinX : td.batch_aabbMaxX;
			BatchOf_float py = shapeData->planes[p].y < 0.0f ? td.batch_aabbMinY : td.batch_aabbMaxY;
			BatchOf_float pz = shapeData->planes[p].z < 0.0f ? td.batch_aabbMinZ : td.batch_aabbMaxZ;
			// tmp = n(dot)p
			BatchOf_float tmp =
				(px * shapeData->planes[p].x) +
				(py * shapeData->planes[p].y) +
				(pz * shapeData->planes[p].z);
			survived = survived && (tmp > -shapeData->planes[p].w);
			// early break if all lanes are dead
			if (survived.isZeroMask()) break;
		}

		// Convert survived lanes to bitmask value
		uint8_t mask = survived.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];
		bool inside = true;
		for (unsigned int p = 0u; p < NUM_FRUSTUM_PLANES; ++p) {
			// Select vertex farthest from the plane in direction of the plane normal.
			// If this point is behind the plane, the AABB must be outside of the frustum.
			Vec3f farthest(
				shapeData->planes[p].x < 0.0 ? d_aabbMinX[queuedIdx] : d_aabbMaxX[queuedIdx],
				shapeData->planes[p].y < 0.0 ? d_aabbMinY[queuedIdx] : d_aabbMaxY[queuedIdx],
				shapeData->planes[p].z < 0.0 ? d_aabbMinZ[queuedIdx] : d_aabbMaxZ[queuedIdx]);
			float nDotFarthest = (
				shapeData->planes[p].x * farthest.x +
				shapeData->planes[p].y * farthest.y +
				shapeData->planes[p].z * farthest.z);
			if (nDotFarthest < -shapeData->planes[p].w) {
				// AABB is outside the frustum
				inside = false;
				break;
			}
		}
		if (inside) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Frustum_OBBs(BatchedIntersectionCase &tid) {
	auto &td = *static_cast<BatchIntersection_Frustum_OBBs *>(&tid);
	auto *shapes = td.indexedShapes->data();
	auto *frustum = static_cast<IntersectionData_Frustum *>(tid.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	int32_t queuedIdx = 0;

	auto *batchData = static_cast<BatchOfOBBs *>(tid.batchData);
	auto *d_obbCenterX = batchData->centerX.data();
	auto *d_obbCenterY = batchData->centerY.data();
	auto *d_obbCenterZ = batchData->centerZ.data();
	auto *d_obbHalfSizeX = batchData->halfSizeX.data();
	auto *d_obbHalfSizeY = batchData->halfSizeY.data();
	auto *d_obbHalfSizeZ = batchData->halfSizeZ.data();
	auto *d_axes = batchData->axes.data();

	for (; queuedIdx + simd::RegisterWidth <= numQueued; queuedIdx += simd::RegisterWidth) {
		// Load some of the ABB data into SIMD registers, too many registers are needed so
		// we leave out the axes and rather load them in a loop below.
		td.batch_obbCenterX.load_aligned(d_obbCenterX + queuedIdx);
		td.batch_obbCenterY.load_aligned(d_obbCenterY + queuedIdx);
		td.batch_obbCenterZ.load_aligned(d_obbCenterZ + queuedIdx);
		td.batch_obbHalfSize[0].load_aligned(d_obbHalfSizeX + queuedIdx);
		td.batch_obbHalfSize[1].load_aligned(d_obbHalfSizeY + queuedIdx);
		td.batch_obbHalfSize[2].load_aligned(d_obbHalfSizeZ + queuedIdx);
		// We'll accumulate a boolean vector "survived" as mask; init to true
		BatchOf_float survived = BatchOf_float::all_ones();

		for (unsigned int planeIdx = 0u; planeIdx < NUM_FRUSTUM_PLANES; ++planeIdx) {
			const auto &plane = frustum->planes[planeIdx];
			// center-to-plane distance + projected radius, radius is initially 0
			BatchOf_float dr =
				(td.batch_obbCenterX * plane.x) +
				(td.batch_obbCenterY * plane.y) +
				(td.batch_obbCenterZ * plane.z) - BatchOf_float(plane.w);
			// Accumulate projected radius from each OBB axis
			for (uint32_t axisIdx=0u; axisIdx < 3u; ++axisIdx) {
				// Load axis into SIMD registers
				BatchOfOBBs::AxisBatch &axisBatch = d_axes[axisIdx];
				BatchOf_float axisX; axisX.load_aligned(axisBatch.x.data() + queuedIdx);
				BatchOf_float axisY; axisY.load_aligned(axisBatch.y.data() + queuedIdx);
				BatchOf_float axisZ; axisZ.load_aligned(axisBatch.z.data() + queuedIdx);
				// Projected radius
				dr += td.batch_obbHalfSize[axisIdx] * (
					(axisX * BatchOf_float(plane.x)).abs() +
					(axisY * BatchOf_float(plane.y)).abs() +
					(axisZ * BatchOf_float(plane.z)).abs());
			}
			survived = survived && dr.isPositive();
			// early break if all lanes are dead
			if (survived.isZeroMask()) break;
		}

		// Convert survived lanes to bitmask value
		uint8_t mask = survived.toBitmask8();
		while (mask) {
			int bitIndex = simd::nextBitIndex<uint8_t>(mask);
			td.hits->push(td.queuedIndices[queuedIdx + bitIndex]);
		}
	}

	// Scalar fallback for remaining items
	for (; queuedIdx < numQueued; queuedIdx++) {
		auto itemIdx = td.queuedIndices[queuedIdx];

		const BoundingBox &obb = *static_cast<OBB *>(shapes[itemIdx].get());
		auto *obbAxes = obb.boxAxes();
		auto &obbCenter = obb.tfOrigin();
		Vec3f obbHalfSize = {
			d_obbHalfSizeX[queuedIdx],
			d_obbHalfSizeY[queuedIdx],
			d_obbHalfSizeZ[queuedIdx]};

		bool isOutside = false;
		for (unsigned int planeIdx = 0u; planeIdx < NUM_FRUSTUM_PLANES; ++planeIdx) {
			const auto &plane = frustum->planes[planeIdx];
			// center-to-plane distance + projected radius
			float dr = obbCenter.dot(plane.xyz_()) - plane.w +
				obbHalfSize.x * std::abs(obbAxes[0].dot(plane.xyz_())) +
				obbHalfSize.y * std::abs(obbAxes[1].dot(plane.xyz_())) +
				obbHalfSize.z * std::abs(obbAxes[2].dot(plane.xyz_()));
			if (dr < 0.0f) { // completely outside
				isOutside = true;
				break;
			}
		}

		if (isOutside) {
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
