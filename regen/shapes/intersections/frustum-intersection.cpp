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
	auto *testShape = static_cast<const Frustum *>(td.testShape);
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
			// condition for *outside*: plane.distance(p) = (n(dot)p) - d < -r
			//     *survived* if: (n(dot)p - d) >= -r  -> (n(dot)p + r) >= d
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
		Vec3f center(
			d_spherePosX[queuedIdx],
			d_spherePosY[queuedIdx],
			d_spherePosZ[queuedIdx]);
		float radius = d_sphereRadius[queuedIdx];
		bool inside = true;
		for (const auto &plane: testShape->planes) {
			if (plane.distance(center) < -radius) {
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
			BatchOf_float &px = shapeData->planes[p].x < 0.0f ? td.batch_aabbMinX : td.batch_aabbMaxX;
			BatchOf_float &py = shapeData->planes[p].y < 0.0f ? td.batch_aabbMinY : td.batch_aabbMaxY;
			BatchOf_float &pz = shapeData->planes[p].z < 0.0f ? td.batch_aabbMinZ : td.batch_aabbMaxZ;
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
			Vec3f n(
				shapeData->planes[p].x,
				shapeData->planes[p].y,
				shapeData->planes[p].z);
			Vec3f farthest(
				n.x < 0.0 ? d_aabbMinX[queuedIdx] : d_aabbMaxX[queuedIdx],
				n.y < 0.0 ? d_aabbMinY[queuedIdx] : d_aabbMaxY[queuedIdx],
				n.z < 0.0 ? d_aabbMinZ[queuedIdx] : d_aabbMaxZ[queuedIdx]);
			if (n.dot(farthest) < -shapeData->planes[p].w) {
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

void regen::shapes::flush_Frustum_OBBs(BatchedIntersectionCase &td) {
	// TODO: Support SIMD on this path.
	//       It uses `if (planes[i].distance(points[j]) >= 0.0) then inside`
	//       So would "only" need point data (8 verts per shape)
	auto *testShape = static_cast<const Frustum *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const BoundingBox &box = *static_cast<BoundingBox *>(shapes[itemIdx].get());
		if (testShape->hasIntersectionWithBox(box)) {
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
