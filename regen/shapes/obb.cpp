#include <regen/math/simd.h>
#include <regen/utility/logging.h>
#include "obb.h"
#include "aabb.h"
#include "bounding-sphere.h"
#include "frustum.h"

using namespace regen;

#define REGEN_OBB_BATCH_DATA_AXES_flat(name, batch) \
	auto* __restrict name##_ax0 = static_cast<float*>(__builtin_assume_aligned(batch.axis0X().data(), 32)); \
	auto* __restrict name##_ay0 = static_cast<float*>(__builtin_assume_aligned(batch.axis0Y().data(), 32)); \
	auto* __restrict name##_az0 = static_cast<float*>(__builtin_assume_aligned(batch.axis0Z().data(), 32)); \
	auto* __restrict name##_ax1 = static_cast<float*>(__builtin_assume_aligned(batch.axis1X().data(), 32)); \
	auto* __restrict name##_ay1 = static_cast<float*>(__builtin_assume_aligned(batch.axis1Y().data(), 32)); \
	auto* __restrict name##_az1 = static_cast<float*>(__builtin_assume_aligned(batch.axis1Z().data(), 32)); \
	auto* __restrict name##_ax2 = static_cast<float*>(__builtin_assume_aligned(batch.axis2X().data(), 32)); \
	auto* __restrict name##_ay2 = static_cast<float*>(__builtin_assume_aligned(batch.axis2Y().data(), 32)); \
	auto* __restrict name##_az2 = static_cast<float*>(__builtin_assume_aligned(batch.axis2Z().data(), 32))

#define REGEN_OBB_BATCH_DATA_AXES_array(name, batch) \
	REGEN_OBB_BATCH_DATA_AXES_flat(name, batch); \
	float* __restrict name##_ax[3] = {name##_ax0, name##_ax1, name##_ax2}; \
	float* __restrict name##_ay[3] = {name##_ay0, name##_ay1, name##_ay2}; \
	float* __restrict name##_az[3] = {name##_az0, name##_az1, name##_az2}

#define REGEN_OBB_BATCH_DATA_SIZE_flat(name, batch) \
	auto* __restrict name##_hx = static_cast<float*>(__builtin_assume_aligned(batch.halfSizeX().data(), 32)); \
	auto* __restrict name##_hy = static_cast<float*>(__builtin_assume_aligned(batch.halfSizeY().data(), 32)); \
	auto* __restrict name##_hz = static_cast<float*>(__builtin_assume_aligned(batch.halfSizeZ().data(), 32))

#define REGEN_OBB_BATCH_DATA_SIZE_array(name, batch) \
	REGEN_OBB_BATCH_DATA_SIZE_flat(name, batch); \
	float* __restrict name##_halfSize[3] = {name##_hx, name##_hy, name##_hz}

#define REGEN_OBB_BATCH_DATA_CENTER(name, batch) \
	auto* __restrict name##_cx = static_cast<float*>(__builtin_assume_aligned(batch.centerX().data(), 32)); \
	auto* __restrict name##_cy = static_cast<float*>(__builtin_assume_aligned(batch.centerY().data(), 32)); \
	auto* __restrict name##_cz = static_cast<float*>(__builtin_assume_aligned(batch.centerZ().data(), 32))

OBB::OBB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts)
		: BoundingBox(BoundingShapeType::OBB, mesh, parts) {
	updateBaseSize();
	updateOBB();
}

OBB::OBB(const Bounds<Vec3f> &bounds)
		: BoundingBox(BoundingShapeType::OBB, bounds) {
	updateBaseSize();
	updateOBB();
}

void OBB::updateBaseSize() {
	Vec3f halfBaseSize = (baseBounds_.max - baseBounds_.min) * 0.5f;
	if (transform_.get()) {
		if (transform_->hasModelMat()) {
			auto tf = transform_->modelMat()->getVertex(transformIndex_);
			halfBaseSize *= tf.r.scaling();
		}
	} else if (localStamp_ != 1) {
		halfBaseSize *= localTransform_.scaling();
	}
	globalBatchData_.halfSizeX()[globalIndex_] = halfBaseSize.x;
	globalBatchData_.halfSizeY()[globalIndex_] = halfBaseSize.y;
	globalBatchData_.halfSizeZ()[globalIndex_] = halfBaseSize.z;
}

bool OBB::updateTransform(bool forceUpdate) {
	auto stamp = tfStamp();
	if (!forceUpdate && stamp == lastTransformStamp_) {
		return false;
	} else {
		lastTransformStamp_ = stamp;
		updateOBB();
		return true;
	}
}

void OBB::updateBaseBounds(const Vec3f &min, const Vec3f &max) {
	baseBounds_.min = mesh_->minPosition();
	baseBounds_.max = mesh_->maxPosition();
	for (const auto &part : parts_) {
		baseBounds_.min.setMin(part->minPosition());
		baseBounds_.max.setMax(part->maxPosition());
	}
	baseBounds_.min += baseOffset_;
	baseBounds_.max += baseOffset_;
	basePosition_ = (baseBounds_.max + baseBounds_.min) * 0.5f;
	updateBaseSize();
	// reset TF stamp to force update
	lastTransformStamp_ = 0;
}

#define _set_axis(i, v) \
	g_ax[i][globalIndex_] = v.x; \
	g_ay[i][globalIndex_] = v.y; \
	g_az[i][globalIndex_] = v.z

void OBB::applyTransform(const Mat4f &tf) {
	REGEN_OBB_BATCH_DATA_AXES_array(g, globalBatchData_);
	tfOrigin_ = (tf ^ Vec4f::create(Vec3f::right(), 0.0f)).xyz(); _set_axis(0, tfOrigin_);
	tfOrigin_ = (tf ^ Vec4f::create(Vec3f::up(),    0.0f)).xyz(); _set_axis(1, tfOrigin_);
	tfOrigin_ = (tf ^ Vec4f::create(Vec3f::front(), 0.0f)).xyz(); _set_axis(2, tfOrigin_);
	tfOrigin_ = (tf ^ Vec4f::create(basePosition_, 1.0f)).xyz();
}

void OBB::updateOBB() {
	REGEN_OBB_BATCH_DATA_CENTER(g, globalBatchData_);
	REGEN_OBB_BATCH_DATA_SIZE_flat(g, globalBatchData_);
	REGEN_OBB_BATCH_DATA_AXES_array(g, globalBatchData_);

	Vec3f scaling = Vec3f::one();
	// compute axes of the OBB based on the model transformation
	if (transform_.get()) {
		if (transform_->hasModelMat()) {
			auto tf = transform_->modelMat()->getVertex(transformIndex_);
			applyTransform(tf.r);
			scaling = tf.r.scaling();
		} else {
			tfOrigin_ = basePosition_;
		}
		if (transform_->hasModelOffset()) {
			tfOrigin_ += transform_->modelOffset()->getVertexClamped(transformIndex_).r.xyz();
		}
	} else if (localStamp_ != 1) {
		// use local transform if no model transformation is set.
		// note: stamp!=1 indicates we do not have an identity transform.
		applyTransform(localTransform_);
		scaling = localTransform_.scaling();
	} else {
		_set_axis(0, Vec3f::right());
		_set_axis(1, Vec3f::up());
		_set_axis(2, Vec3f::front());
		tfOrigin_ = basePosition_;
	}
	g_cx[globalIndex_] = tfOrigin_.x;
	g_cy[globalIndex_] = tfOrigin_.y;
	g_cz[globalIndex_] = tfOrigin_.z;

	if (scaling != Vec3f::one()) {
		// normalize axes if scaling is applied
		float len;
		#define _normalizeAxis(i) \
			len = sqrt(g_ax[i][globalIndex_] * g_ax[i][globalIndex_] + \
					   g_ay[i][globalIndex_] * g_ay[i][globalIndex_] + \
					   g_az[i][globalIndex_] * g_az[i][globalIndex_]); \
			g_ax[i][globalIndex_] /= len; \
			g_ay[i][globalIndex_] /= len; \
			g_az[i][globalIndex_] /= len
		_normalizeAxis(0);
		_normalizeAxis(1);
		_normalizeAxis(2);
		#undef _normalizeAxis
	}

	// compute vertices of the OBB
	Vec3f x, y, z;
	// x = axis0 * halfSizeX
	x.x = g_ax[0][globalIndex_] * g_hx[globalIndex_];
	x.y = g_ay[0][globalIndex_] * g_hx[globalIndex_];
	x.z = g_az[0][globalIndex_] * g_hx[globalIndex_];
	// y = axis1 * halfSizeY
	y.x = g_ax[1][globalIndex_] * g_hy[globalIndex_];
	y.y = g_ay[1][globalIndex_] * g_hy[globalIndex_];
	y.z = g_az[1][globalIndex_] * g_hy[globalIndex_];
	// z = axis2 * halfSizeZ
	z.x = g_ax[2][globalIndex_] * g_hz[globalIndex_];
	z.y = g_ay[2][globalIndex_] * g_hz[globalIndex_];
	z.z = g_az[2][globalIndex_] * g_hz[globalIndex_];
	vertices_[0] = tfOrigin_ - x - y - z;
	vertices_[1] = tfOrigin_ - x - y + z;
	vertices_[2] = tfOrigin_ - x + y - z;
	vertices_[3] = tfOrigin_ - x + y + z;
	vertices_[4] = tfOrigin_ + x - y - z;
	vertices_[5] = tfOrigin_ + x - y + z;
	vertices_[6] = tfOrigin_ + x + y - z;
	vertices_[7] = tfOrigin_ + x + y + z;
}
#undef _set_axis

Vec3f OBB::closestPointOnSurface(const Vec3f &point) const {
	REGEN_OBB_BATCH_DATA_SIZE_array(g, globalBatchData_);
	REGEN_OBB_BATCH_DATA_AXES_array(g, globalBatchData_);
	const Vec3f &obbCenter = tfOrigin();
	Vec3f d = point - obbCenter;

	Vec3f tf_closest = obbCenter;
	for (int i = 0; i < 3; ++i) {
		float halfSize = g_halfSize[i][globalIndex_];
		Vec3f axis(
			g_ax[i][globalIndex_],
			g_ay[i][globalIndex_],
			g_az[i][globalIndex_]);
		tf_closest = tf_closest + axis * math::clamp(d.dot(axis), -halfSize, halfSize);
	}
	return tf_closest;
}

template <typename T>
inline std::pair<float, float> project(const T &a, const Vec3f &axis) {
	const Vec3f *v = a.boxVertices();
	float min = v[0].dot(axis);
	float max = min;
	for (int i = 1; i < 8; ++i) {
		float projection = v[i].dot(axis);
		if (projection < min) min = projection;
		if (projection > max) max = projection;
	}
	return std::make_pair(min, max);
}

#define _axis_struct(box, i) \
	Vec3f( \
		(box).globalBatchData_.axes()[i].x[(box).globalIndex_], \
		(box).globalBatchData_.axes()[i].y[(box).globalIndex_], \
		(box).globalBatchData_.axes()[i].z[(box).globalIndex_])

bool OBB::hasIntersectionWithOBB(const OBB &other) const {
	static constexpr int NUM_OBB_TEST_AXES = 15;
	// get the axes of both OBBs
	const Vec3f axes_a[3] = {
		_axis_struct(*this, 0),
		_axis_struct(*this, 1),
		_axis_struct(*this, 2)};
	const Vec3f axes_b[3] = {
		_axis_struct(other, 0),
		_axis_struct(other, 1),
		_axis_struct(other, 2)};

	std::array<Vec3f, NUM_OBB_TEST_AXES> axes = {
		axes_a[0], axes_a[1], axes_a[2],
		axes_b[0], axes_b[1], axes_b[2],
		//
		axes_a[0].cross(axes_b[0]),
		axes_a[0].cross(axes_b[1]),
		axes_a[0].cross(axes_b[2]),
		//
		axes_a[1].cross(axes_b[0]),
		axes_a[1].cross(axes_b[1]),
		axes_a[1].cross(axes_b[2]),
		//
		axes_a[2].cross(axes_b[0]),
		axes_a[2].cross(axes_b[1]),
		axes_a[2].cross(axes_b[2]) };

	bool isOutside = false;

	for (uint32_t i = 0; i < NUM_OBB_TEST_AXES && !isOutside; ++i) {
		auto &axis = axes[i];
		if (axis.x == 0 && axis.y == 0 && axis.z == 0) continue; // Skip zero-length axes

		auto [minA, maxA] = project(*this, axis);
		auto [minB, maxB] = project(other, axis);

		isOutside = (maxA < minB || maxB < minA);
	}

	return !isOutside;
}

#undef _axis_struct

bool OBB::hasIntersectionWithAABB(const AABB &other) const {
	auto &c = tfOrigin();
	const uint32_t g_idx =globalIndex();
	REGEN_OBB_BATCH_DATA_SIZE_flat(g, globalBatchData_t());
	REGEN_OBB_BATCH_DATA_AXES_flat(g, globalBatchData_t());

	auto &aabbBatch = other.globalBatchData_t();
	const uint32_t aabbIdx = other.globalIndex();

	float r;
	r = g_hx[g_idx] * std::abs(g_ax0[g_idx]) +
		g_hy[g_idx] * std::abs(g_ax1[g_idx]) +
		g_hz[g_idx] * std::abs(g_ax2[g_idx]);
	if (c.x - r > aabbBatch.maxX()[aabbIdx] || c.x + r < aabbBatch.minX()[aabbIdx]) return false;

	r = g_hx[g_idx] * std::abs(g_ay0[g_idx]) +
		g_hy[g_idx] * std::abs(g_ay1[g_idx]) +
		g_hz[g_idx] * std::abs(g_ay2[g_idx]);
	if (c.y - r > aabbBatch.maxY()[aabbIdx] || c.y + r < aabbBatch.minY()[aabbIdx]) return false;

	r = g_hx[g_idx] * std::abs(g_az0[g_idx]) +
		g_hy[g_idx] * std::abs(g_az1[g_idx]) +
		g_hz[g_idx] * std::abs(g_az2[g_idx]);
	if (c.z - r > aabbBatch.maxZ()[aabbIdx] || c.z + r < aabbBatch.minZ()[aabbIdx]) return false;

	return true;
}

void OBB::batchTest_Spheres(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto &testShape = *static_cast<const OBB *>(td.testShape);

	// bounding box in base pose
	const Vec3f &obbCenter = testShape.tfOrigin();
	const uint32_t g_idx = testShape.globalIndex();
	// transformed axes of the OBB
	REGEN_OBB_BATCH_DATA_SIZE_array(g, testShape.globalBatchData_t());
	REGEN_OBB_BATCH_DATA_AXES_array(g, testShape.globalBatchData_t());

	auto *batchData = static_cast<const BatchOfSpheres *>(td.batchData);
	const float *d_spherePosX = batchData->posX().data();
	const float *d_spherePosY = batchData->posY().data();
	const float *d_spherePosZ = batchData->posZ().data();
	const float *d_sphereRadius = batchData->radius().data();

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
			BatchOf_float halfSizeOnAxis = BatchOf_float::fromScalar(g_halfSize[i][g_idx]);
			// dist = delta.dot(axis)
			BatchOf_float dist =
				(deltaX * g_ax[i][g_idx]) +
				(deltaY * g_ay[i][g_idx]) +
				(deltaZ * g_az[i][g_idx]);
			// dist = clamp(dist, -halfSize, halfSize)
			dist.c = simd::min_ps(simd::max_ps(dist.c, -halfSizeOnAxis.c), halfSizeOnAxis.c);
			// closest += axis * clamped, use fused-multiply-add
			closestX.c = simd::mul_add_ps(dist.c, simd::set1_ps(g_ax[i][g_idx]), closestX.c);
			closestY.c = simd::mul_add_ps(dist.c, simd::set1_ps(g_ay[i][g_idx]), closestY.c);
			closestZ.c = simd::mul_add_ps(dist.c, simd::set1_ps(g_az[i][g_idx]), closestZ.c);
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

void OBB::batchTest_AABBs(BatchedIntersectionCase &td) {
	const auto numQueued = static_cast<int32_t>(td.numQueued);
	auto &testShape = *static_cast<const OBB *>(td.testShape);

	// bounding box in base pose
	auto &obbCenter = testShape.tfOrigin();
	const uint32_t g_idx = testShape.globalIndex();
	REGEN_OBB_BATCH_DATA_SIZE_flat(g, testShape.globalBatchData_t());
	REGEN_OBB_BATCH_DATA_AXES_flat(g, testShape.globalBatchData_t());
	// The OBB's projected radius
	const std::array<float,3> obbRadius = {
		// R = hx*|ax0| + hy*|ax1| + hz*|ax2|
		g_hx[g_idx] * std::abs(g_ax0[g_idx]) +
		g_hy[g_idx] * std::abs(g_ax1[g_idx]) +
		g_hz[g_idx] * std::abs(g_ax2[g_idx]),
		// R = hx*|ay0| + hy*|ay1| + hz*|ay2|
		g_hx[g_idx] * std::abs(g_ay0[g_idx]) +
		g_hy[g_idx] * std::abs(g_ay1[g_idx]) +
		g_hz[g_idx] * std::abs(g_ay2[g_idx]),
		// R = hx*|az0| + hy*|az1| + hz*|az2|
		g_hx[g_idx] * std::abs(g_az0[g_idx]) +
		g_hy[g_idx] * std::abs(g_az1[g_idx]) +
		g_hz[g_idx] * std::abs(g_az2[g_idx])};

	auto *batchData = static_cast<const BatchOfAABBs *>(td.batchData);
	const float *d_aabbMinX = batchData->minX().data();
	const float *d_aabbMinY = batchData->minY().data();
	const float *d_aabbMinZ = batchData->minZ().data();
	const float *d_aabbMaxX = batchData->maxX().data();
	const float *d_aabbMaxY = batchData->maxY().data();
	const float *d_aabbMaxZ = batchData->maxZ().data();

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

void OBB::batchTest_OBBs(BatchedIntersectionCase &td) {
	// note: This case cannot be handled well with AVX and its limited number
	// of registers, so we do a scalar implementation here.
	auto &testShape = *static_cast<const OBB *>(td.testShape);
	auto *shapes = td.indexedShapes->data();

	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const OBB &box = *static_cast<OBB *>(shapes[itemIdx].get());
		if (testShape.hasIntersectionWithOBB(box)) {
			td.hits->push(itemIdx);
		}
	}
}

void OBB::batchTest_Frustums(BatchedIntersectionCase &td) {
	// note: index shapes are rarely frustum, so no SIMD optimization here.
	auto *testShape = static_cast<const OBB *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const Frustum &frustum = *static_cast<Frustum *>(shapes[itemIdx].get());
		if (frustum.hasIntersectionWithOBB(*testShape)) {
			td.hits->push(itemIdx);
		}
	}
}
