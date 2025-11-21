#include <regen/math/simd.h>
#include "aabb.h"
#include "bounding-sphere.h"
#include "frustum.h"

using namespace regen;

#define REGEN_AABB_BATCH_DATA(name, batch) \
	auto* __restrict name##_minX = static_cast<float*>(__builtin_assume_aligned(batch.soaData_[0].data(), 32)); \
	auto* __restrict name##_minY = static_cast<float*>(__builtin_assume_aligned(batch.soaData_[1].data(), 32)); \
	auto* __restrict name##_minZ = static_cast<float*>(__builtin_assume_aligned(batch.soaData_[2].data(), 32)); \
	auto* __restrict name##_maxX = static_cast<float*>(__builtin_assume_aligned(batch.soaData_[3].data(), 32)); \
	auto* __restrict name##_maxY = static_cast<float*>(__builtin_assume_aligned(batch.soaData_[4].data(), 32)); \
	auto* __restrict name##_maxZ = static_cast<float*>(__builtin_assume_aligned(batch.soaData_[5].data(), 32))

AABB::AABB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts)
		: BoundingBox(BoundingShapeType::AABB, mesh, parts) {
	updateAABB();
}

AABB::AABB(const Bounds<Vec3f> &bounds)
		: BoundingBox(BoundingShapeType::AABB, bounds) {
	updateAABB();
}

bool AABB::updateTransform(bool forceUpdate) {
	const uint32_t stamp = tfStamp();
	if (!forceUpdate && stamp == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = stamp;
		updateAABB();
		return true;
	}
}

void AABB::updateBaseBounds(const Vec3f &min, const Vec3f &max) {
	baseBounds_.min = mesh_->minPosition();
	baseBounds_.max = mesh_->maxPosition();
	for (const auto &part : parts_) {
		baseBounds_.min.setMin(part->minPosition());
		baseBounds_.max.setMax(part->maxPosition());
	}
	baseBounds_.min += baseOffset_;
	baseBounds_.max += baseOffset_;
	basePosition_ = (baseBounds_.max + baseBounds_.min) * 0.5f;
	// reset TF stamp to force update
	lastTransformStamp_ = 0;
}

void AABB::updateAABB() {
	// Compiler hints: assume arrays do not alias, aligned to 32 bytes
	REGEN_AABB_BATCH_DATA(g, globalBatchData_);

#define _set_min(v) g_minX[globalIndex_] = v.x; g_minY[globalIndex_] = v.y; g_minZ[globalIndex_] = v.z
#define _set_max(v) g_maxX[globalIndex_] = v.x; g_maxY[globalIndex_] = v.y; g_maxZ[globalIndex_] = v.z
#define _set_minmax(v) \
	transformed = (tf.r ^ v).xyz(); \
	g_minX[globalIndex_] = std::min(g_minX[globalIndex_], transformed.x); \
	g_minY[globalIndex_] = std::min(g_minY[globalIndex_], transformed.y); \
	g_minZ[globalIndex_] = std::min(g_minZ[globalIndex_], transformed.z); \
	g_maxX[globalIndex_] = std::max(g_maxX[globalIndex_], transformed.x); \
	g_maxY[globalIndex_] = std::max(g_maxY[globalIndex_], transformed.y); \
	g_maxZ[globalIndex_] = std::max(g_maxZ[globalIndex_], transformed.z)

	// apply transform
	if (transform_.get()) {
		if (transform_->hasModelMat()) {
			auto tf = transform_->modelMat()->getVertexClamped(transformIndex_);
			// compute transformed bounds
			Vec3f transformed;
			_set_min(Vec3f::posMax());
			_set_max(Vec3f::negMax());
			// min = min(all 8 transformed vertices)
			// max = max(all 8 transformed vertices)
			_set_minmax(Vec3f(baseBounds_.min.x, baseBounds_.min.y, baseBounds_.min.z));
			_set_minmax(Vec3f(baseBounds_.min.x, baseBounds_.min.y, baseBounds_.max.z));
			_set_minmax(Vec3f(baseBounds_.min.x, baseBounds_.max.y, baseBounds_.min.z));
			_set_minmax(Vec3f(baseBounds_.min.x, baseBounds_.max.y, baseBounds_.max.z));
			_set_minmax(Vec3f(baseBounds_.max.x, baseBounds_.min.y, baseBounds_.min.z));
			_set_minmax(Vec3f(baseBounds_.max.x, baseBounds_.min.y, baseBounds_.max.z));
			_set_minmax(Vec3f(baseBounds_.max.x, baseBounds_.max.y, baseBounds_.min.z));
			_set_minmax(Vec3f(baseBounds_.max.x, baseBounds_.max.y, baseBounds_.max.z));
			// tfOrigin = (tfBounds.min + tfBounds.max) * 0.5f;
			tfOrigin_.x = (g_minX[globalIndex_] + g_maxX[globalIndex_]) * 0.5f;
			tfOrigin_.y = (g_minY[globalIndex_] + g_maxY[globalIndex_]) * 0.5f;
			tfOrigin_.z = (g_minZ[globalIndex_] + g_maxZ[globalIndex_]) * 0.5f;
		} else {
			_set_min(baseBounds_.min);
			_set_max(baseBounds_.max);
			tfOrigin_ = basePosition_;
		}

		// apply model offset if available
		if (transform_->hasModelOffset()) {
			auto &modelOffset = transform_->modelOffset();
			auto offset = modelOffset->getVertexClamped(transformIndex_);
			const Vec3f &o = offset.r.xyz();
			// tfBounds += o;
			g_minX[globalIndex_] += o.x;
			g_minY[globalIndex_] += o.y;
			g_minZ[globalIndex_] += o.z;
			g_maxX[globalIndex_] += o.x;
			g_maxY[globalIndex_] += o.y;
			g_maxZ[globalIndex_] += o.z;
			tfOrigin_ += o;
		}
	} else {
		_set_min(baseBounds_.min);
		_set_max(baseBounds_.max);
		tfOrigin_ = basePosition_;
	}
#undef _set_minmax
#undef _set_min
#undef _set_max
}

Vec3f AABB::closestPointOnSurface(const Vec3f &point) const {
	// Compiler hints: assume arrays do not alias, aligned to 32 bytes
	REGEN_AABB_BATCH_DATA(g, globalBatchData_);
	Vec3f closestPoint;
	closestPoint.x = point.x < g_minX[globalIndex_] ? g_minX[globalIndex_] :
		(point.x > g_maxX[globalIndex_] ? g_maxX[globalIndex_] : point.x);
	closestPoint.y = point.y < g_minY[globalIndex_] ? g_minY[globalIndex_] :
		(point.y > g_maxY[globalIndex_] ? g_maxY[globalIndex_] : point.y);
	closestPoint.z = point.z < g_minZ[globalIndex_] ? g_minZ[globalIndex_] :
		(point.z > g_maxZ[globalIndex_] ? g_maxZ[globalIndex_] : point.z);
	return closestPoint;
}

void AABB::batchTest_Spheres(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto *testShape = static_cast<const AABB *>(td.testShape);

	REGEN_AABB_BATCH_DATA(global, testShape->globalBatchData());
	const uint32_t globalIdx = testShape->globalIndex();

	auto *batchData = static_cast<const BatchOfSpheres *>(td.batchData);
	const float *d_spherePosX = batchData->posX().data();
	const float *d_spherePosY = batchData->posY().data();
	const float *d_spherePosZ = batchData->posZ().data();
	const float *d_sphereRadius = batchData->radius().data();

	// load min/max aabb into SIMD registers, we need 6 registers.
	const BatchOf_float aabbMinX = BatchOf_float::fromScalar(global_minX[globalIdx]);
	const BatchOf_float aabbMinY = BatchOf_float::fromScalar(global_minY[globalIdx]);
	const BatchOf_float aabbMinZ = BatchOf_float::fromScalar(global_minZ[globalIdx]);
	const BatchOf_float aabbMaxX = BatchOf_float::fromScalar(global_maxX[globalIdx]);
	const BatchOf_float aabbMaxY = BatchOf_float::fromScalar(global_maxY[globalIdx]);
	const BatchOf_float aabbMaxZ = BatchOf_float::fromScalar(global_maxZ[globalIdx]);

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
			d_spherePosX[queuedIdx] + d_sphereRadius[queuedIdx] < global_minX[globalIdx] ||
			d_spherePosX[queuedIdx] - d_sphereRadius[queuedIdx] > global_maxX[globalIdx] ||
			d_spherePosY[queuedIdx] + d_sphereRadius[queuedIdx] < global_minY[globalIdx] ||
			d_spherePosY[queuedIdx] - d_sphereRadius[queuedIdx] > global_maxY[globalIdx] ||
			d_spherePosZ[queuedIdx] + d_sphereRadius[queuedIdx] < global_minZ[globalIdx] ||
			d_spherePosZ[queuedIdx] - d_sphereRadius[queuedIdx] > global_maxZ[globalIdx];
		if (!isOutside) {
			td.hits->push(itemIdx);
		}
	}
}

bool AABB::hasIntersectionWithAABB(const AABB &other) const {
	// Compiler hints: assume arrays do not alias, aligned to 32 bytes
	REGEN_AABB_BATCH_DATA(g, globalBatchData_);
	return g_minX[globalIndex_] < g_maxX[other.globalIndex()] &&
		   g_maxX[globalIndex_] > g_minX[other.globalIndex()] &&
		   g_minY[globalIndex_] < g_maxY[other.globalIndex()] &&
		   g_maxY[globalIndex_] > g_minY[other.globalIndex()] &&
		   g_minZ[globalIndex_] < g_maxZ[other.globalIndex()] &&
		   g_maxZ[globalIndex_] > g_minZ[other.globalIndex()];
}

void AABB::batchTest_AABBs(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto *testShape = static_cast<const AABB *>(td.testShape);

	REGEN_AABB_BATCH_DATA(global, testShape->globalBatchData());
	const uint32_t globalIdx = testShape->globalIndex();

	auto *batchData = static_cast<const BatchOfAABBs *>(td.batchData);
	const float *aabbMinX_1 = batchData->minX().data();
	const float *aabbMinY_1 = batchData->minY().data();
	const float *aabbMinZ_1 = batchData->minZ().data();
	const float *aabbMaxX_1 = batchData->maxX().data();
	const float *aabbMaxY_1 = batchData->maxY().data();
	const float *aabbMaxZ_1 = batchData->maxZ().data();

	// load min/max aabb into SIMD registers, we need 6 registers.
	const BatchOf_float t_aabbMinX = BatchOf_float::fromScalar(global_minX[globalIdx]);
	const BatchOf_float t_aabbMinY = BatchOf_float::fromScalar(global_minY[globalIdx]);
	const BatchOf_float t_aabbMinZ = BatchOf_float::fromScalar(global_minZ[globalIdx]);
	const BatchOf_float t_aabbMaxX = BatchOf_float::fromScalar(global_maxX[globalIdx]);
	const BatchOf_float t_aabbMaxY = BatchOf_float::fromScalar(global_maxY[globalIdx]);
	const BatchOf_float t_aabbMaxZ = BatchOf_float::fromScalar(global_maxZ[globalIdx]);

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
			global_minX[globalIdx] < aabbMaxX_1[queuedIdx] &&
			global_maxX[globalIdx] > aabbMinX_1[queuedIdx] &&
			global_minY[globalIdx] < aabbMaxY_1[queuedIdx] &&
			global_maxY[globalIdx] > aabbMinY_1[queuedIdx] &&
			global_minZ[globalIdx] < aabbMaxZ_1[queuedIdx] &&
			global_maxZ[globalIdx] > aabbMinZ_1[queuedIdx];
		if (hasIntersection) {
			td.hits->push(itemIdx);
		}
	}
}

void AABB::batchTest_OBBs(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto *testShape = static_cast<const AABB *>(td.testShape);

	REGEN_AABB_BATCH_DATA(global, testShape->globalBatchData());
	const uint32_t globalIdx = testShape->globalIndex();

	auto *batchData = static_cast<const BatchOfOBBs *>(td.batchData);
	const float *d_obbCenterX = batchData->centerX().data();
	const float *d_obbCenterY = batchData->centerY().data();
	const float *d_obbCenterZ = batchData->centerZ().data();
	const float *d_obbHalfSizeX = batchData->halfSizeX().data();
	const float *d_obbHalfSizeY = batchData->halfSizeY().data();
	const float *d_obbHalfSizeZ = batchData->halfSizeZ().data();
	auto d_axes = batchData->axes();

	const BatchOf_float aabbMinX = BatchOf_float::fromScalar(global_minX[globalIdx]);
	const BatchOf_float aabbMinY = BatchOf_float::fromScalar(global_minY[globalIdx]);
	const BatchOf_float aabbMinZ = BatchOf_float::fromScalar(global_minZ[globalIdx]);
	const BatchOf_float aabbMaxX = BatchOf_float::fromScalar(global_maxX[globalIdx]);
	const BatchOf_float aabbMaxY = BatchOf_float::fromScalar(global_maxY[globalIdx]);
	const BatchOf_float aabbMaxZ = BatchOf_float::fromScalar(global_maxZ[globalIdx]);

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
				auto &axisBatch = d_axes[axisIdx];
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
		if (d_obbCenterX[queuedIdx] - obbRadiusX < global_maxX[globalIdx] &&
			d_obbCenterX[queuedIdx] + obbRadiusX > global_minX[globalIdx] &&
			d_obbCenterY[queuedIdx] - obbRadiusY < global_maxY[globalIdx] &&
			d_obbCenterY[queuedIdx] + obbRadiusY > global_minY[globalIdx] &&
			d_obbCenterZ[queuedIdx] - obbRadiusZ < global_maxZ[globalIdx] &&
			d_obbCenterZ[queuedIdx] + obbRadiusZ > global_minZ[globalIdx])
		{
			auto itemIdx = td.queuedIndices[queuedIdx];
			td.hits->push(itemIdx);
		}
	}
}

void AABB::batchTest_Frustums(BatchedIntersectionCase &td) {
	// note: index shapes are rarely frustum, so no SIMD optimization here.
	auto *testShape = static_cast<const AABB *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const Frustum &frustum = *static_cast<Frustum *>(shapes[itemIdx].get());
		if (frustum.hasIntersectionWithAABB(*testShape)) {
			td.hits->push(itemIdx);
		}
	}
}
