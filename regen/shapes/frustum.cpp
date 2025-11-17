#include "frustum.h"
#include <regen/math/simd.h>

using namespace regen;

namespace regen {
	static constexpr int NUM_FRUSTUM_PLANES = 6;
}

Frustum::Frustum() :
		BoundingShape(BoundingShapeType::FRUSTUM),
		orthoBounds(Bounds<Vec2f>::create(Vec2f(0), Vec2f(0))) {
	direction_ = ref_ptr<ShaderInput3f>::alloc("frustumDirection");
	direction_->setUniformData(Vec3f::front());
}

void Frustum::setPerspective(double _aspect, double _fov, double _near, double _far) {
	fov = _fov;
	aspect = _aspect;
	near = _near;
	far = _far;
	double fovR = _fov / 57.2957795;
	nearPlaneHalfSize.y = tan(fovR * 0.5) * near;
	nearPlaneHalfSize.x = nearPlaneHalfSize.y * aspect;
	farPlaneHalfSize.y = tan(fovR * 0.5) * far;
	farPlaneHalfSize.x = farPlaneHalfSize.y * aspect;
}

void Frustum::setOrtho(double left, double right, double bottom, double top, double _near, double _far) {
	fov = 0.0;
	aspect = abs((right - left) / (top - bottom));
	near = _near;
	far = _far;
	nearPlaneHalfSize.x = abs(right-left)*0.5;
	nearPlaneHalfSize.y = abs(top-bottom)*0.5;
	farPlaneHalfSize = nearPlaneHalfSize;
	orthoBounds.min.x = left;
	orthoBounds.min.y = bottom;
	orthoBounds.max.x = right;
	orthoBounds.max.y = top;
}

bool Frustum::updateTransform(bool forceUpdate) {
	auto stamp0 = tfStamp();
	auto stamp1 = directionStamp();
	if (!forceUpdate && lastTransformStamp_ == stamp0 && lastDirectionStamp_ == stamp1) {
		return false;
	}
	lastTransformStamp_ = stamp0;
	lastDirectionStamp_ = stamp1;
	return true;
}

unsigned int Frustum::directionStamp() const {
	if (direction_.get()) {
		return direction_->stampOfReadData();
	} else {
		return 0;
	}
}

void Frustum::update(const Vec3f &pos, const Vec3f &dir) {
	Vec3f d = dir;
	d.normalize();
	localTransform_.setPosition(pos);
	direction_->setVertex(0, d);
	tfOrigin_ = pos;

	if (fov > 0.0) {
		updatePointsPerspective(pos, d);
	} else {
		updatePointsOrthogonal(pos, d);
	}

	// Top:		nTr - nTl - fTl
	planes[0].set(points[2], points[1], points[5]);
	// Bottom:	nBl - nBr - fBr
	planes[1].set(points[3], points[0], points[4]);
	// Left:	ntL - nbL - fbL
	planes[2].set(points[1], points[3], points[7]);
	// Right:	nbR - ntR - fbR
	planes[3].set(points[0], points[2], points[4]);
	// Near:	nTl - nTr - nBr
	planes[4].set(points[1], points[2], points[0]);
	// Far:		fTr - fTl - fBl
	planes[5].set(points[6], points[5], points[7]);
}

void Frustum::updatePointsPerspective(const Vec3f &pos, const Vec3f &dir) {
	auto v = dir.cross(Vec3f::up());
	const float vLength = v.length();
	if (vLength < 1e-6f) {
		// direction is parallel to up vector
		v = dir.cross(Vec3f::right());
		v.normalize();
	} else {
		v /= vLength;
	}
	auto u = v.cross(dir);
	u.normalize();
	// near plane points
	auto nc = pos + dir * near;
	auto rw = v * nearPlaneHalfSize.x;
	auto uh = u * nearPlaneHalfSize.y;
	points[0] = nc - uh + rw; // bottom right
	points[1] = nc + uh - rw; // top left
	points[2] = nc + uh + rw; // top right
	points[3] = nc - uh - rw; // bottom left
	// far plane points
	auto fc = pos + dir * far;
	rw = v * farPlaneHalfSize.x;
	uh = u * farPlaneHalfSize.y;
	points[4] = fc - uh + rw; // bottom right
	points[5] = fc + uh - rw; // top left
	points[6] = fc + uh + rw; // top right
	points[7] = fc - uh - rw; // bottom left
}

void Frustum::updatePointsOrthogonal(const Vec3f &pos, const Vec3f &dir) {
	auto v = dir.cross(Vec3f::up());
	v.normalize();
	auto u = v.cross(dir);
	u.normalize();
	// do not assume that the frustum is centered at the far/near plane centroids!
	// could be that left/right/top/bottom are not symmetric
	auto vl = v * orthoBounds.min.x; // left
	auto vr = v * orthoBounds.max.x; // right
	auto ub = u * orthoBounds.min.y; // bottom
	auto ut = u * orthoBounds.max.y; // top
	// near plane points
	auto nc = pos + dir * near;
	points[0] = nc + vr + ub; // bottom right
	points[1] = nc + vl + ut; // top left
	points[2] = nc + vr + ut; // top right
	points[3] = nc + vl + ub; // bottom left
	// far plane points
	auto fc = pos + dir * far;
	points[4] = fc + vr + ub; // bottom right
	points[5] = fc + vl + ut; // top left
	points[6] = fc + vr + ut; // top right
	points[7] = fc + vl + ub; // bottom left
}

void Frustum::split(double splitWeight, std::vector<Frustum> &frustumSplit) const {
	const auto &n = near;
	const auto &f = far;
	const auto count = frustumSplit.size();
	auto ratio = f / n;
	double si, lastn, currf, currn;

	lastn = n;
	for (uint32_t i = 1; i < count; ++i) {
		si = i / (double) count;

		// C_i = \lambda * C_i^{log} + (1-\lambda) * C_i^{uni}
		currn = splitWeight * (n * (pow(ratio, si))) +
				(1 - splitWeight) * (n + (f - n) * si);
		currf = currn * 1.005;

		frustumSplit[i - 1].setPerspective(aspect, fov, lastn, currf);

		lastn = currn;
	}
	frustumSplit[count - 1].setPerspective(aspect, fov, lastn, f);
}

Vec3f Frustum::closestPointOnSurface(const Vec3f &point) const {
	Vec3f closestPoint;
	float minDistanceSqr = std::numeric_limits<float>::max();

	for (const auto &plane: planes) {
		Vec3f planePoint = plane.closestPoint(point);
		float distanceSqr = (planePoint - point).lengthSquared();
		if (distanceSqr < minDistanceSqr) {
			minDistanceSqr = distanceSqr;
			closestPoint = planePoint;
		}
	}

	return closestPoint;
}

bool Frustum::hasIntersectionWithSphere(const BoundingSphere &sphere) const {
	auto &center = sphere.tfOrigin();
	auto radius = sphere.radius();
	for (const auto &plane: planes) {
		if (plane.distance(center) < -radius) {
			return false;
		}
	}
	return true;
}

void Frustum::batchTest_Spheres(BatchedIntersectionCase &td) {
	// Process numQueuedItems_ nodes from the queue, performing an intersection test with the shape's projection;
	// and also writing nodeIdx to successor array if the test succeeds.
	auto *shapeData = static_cast<IntersectionData_Frustum *>(td.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *batchData = static_cast<const BatchOfSpheres *>(td.batchData);
	const float *d_spherePosX = batchData->posX().data();
	const float *d_spherePosY = batchData->posY().data();
	const float *d_spherePosZ = batchData->posZ().data();
	const float *d_sphereRadius = batchData->radius().data();

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
		bool isOutside = false;
		for (int p = 0; p < NUM_FRUSTUM_PLANES && !isOutside; ++p) {
			// tmp = n(dot)p + r
			float n_dot_p =
				(d_spherePosX[queuedIdx] * shapeData->planes[p].x) +
				(d_spherePosY[queuedIdx] * shapeData->planes[p].y) +
				(d_spherePosZ[queuedIdx] * shapeData->planes[p].z) +
				d_sphereRadius[queuedIdx];
			// fully outside if: (n_dot_p + r - w) < 0
			isOutside = (n_dot_p < shapeData->planes[p].w);
		}
		if (!isOutside) {
			td.hits->push(td.queuedIndices[queuedIdx]);
		}
	}
}

bool Frustum::hasIntersectionWithAABB(const AABB &box) const {
	auto &boxBatchData = box.globalBatchData_t();
	auto boxIdx = box.globalIndex();
	const float *boxMinX = boxBatchData.minX().data() + boxIdx;
	const float *boxMinY = boxBatchData.minY().data() + boxIdx;
	const float *boxMinZ = boxBatchData.minZ().data() + boxIdx;
	const float *boxMaxX = boxBatchData.maxX().data() + boxIdx;
	const float *boxMaxY = boxBatchData.maxY().data() + boxIdx;
	const float *boxMaxZ = boxBatchData.maxZ().data() + boxIdx;

	for (unsigned int i = 0u; i < 6u; ++i) {
		// Select vertex farthest from the plane in direction of the plane normal.
		// If this point is behind the plane, the AABB must be outside of the frustum.
		auto &plane = planes[i];
		auto &n = plane.normal();
		const Vec3f p(
			n.x > 0.0 ? *boxMinX : *boxMaxX,
			n.y > 0.0 ? *boxMinY : *boxMaxY,
			n.z > 0.0 ? *boxMinZ : *boxMaxZ);
		if (n.dot(p) - plane.coefficients.w < 0.0) {
			// AABB is outside the frustum
			return false;
		}
	}
	return true;
}

void Frustum::batchTest_AABBs(BatchedIntersectionCase &td) {
	auto *shapeData = static_cast<IntersectionData_Frustum *>(td.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *batchData = static_cast<const BatchOfAABBs *>(td.batchData);
	const float *d_aabbMinX = batchData->minX().data();
	const float *d_aabbMinY = batchData->minY().data();
	const float *d_aabbMinZ = batchData->minZ().data();
	const float *d_aabbMaxX = batchData->maxX().data();
	const float *d_aabbMaxY = batchData->maxY().data();
	const float *d_aabbMaxZ = batchData->maxZ().data();

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
		bool isOutside = false;
		for (unsigned int p = 0u; p < NUM_FRUSTUM_PLANES && !isOutside; ++p) {
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
			// Completely outside
			isOutside = (n_dot_p < shapeData->planes[p].w);
		}
		if (!isOutside) {
			td.hits->push(td.queuedIndices[queuedIdx]);
		}
	}
}

bool Frustum::hasIntersectionWithOBB(const OBB &box) const {
	auto *boxPoints = box.boxVertices();
	for (unsigned int i = 0u; i < 6u; ++i) {
		bool allOutside = true;
		for (unsigned int j = 0u; j < 8; ++j) {
			if (planes[i].distance(boxPoints[j]) >= 0.0) {
				allOutside = false;
				break;
			}
		}
		if (allOutside) {
			return false;
		}
	}
	return true;
}

void Frustum::batchTest_OBBs(BatchedIntersectionCase &td) {
	auto *shapes = td.indexedShapes->data();
	auto *frustum = static_cast<IntersectionData_Frustum *>(td.shapeData);
	const auto numQueued = static_cast<int32_t>(td.numQueued);

	auto *batchData = static_cast<const BatchOfOBBs *>(td.batchData);
	const float *d_obbCenterX = batchData->centerX().data();
	const float *d_obbCenterY = batchData->centerY().data();
	const float *d_obbCenterZ = batchData->centerZ().data();
	const float *d_obbHalfSizeX = batchData->halfSizeX().data();
	const float *d_obbHalfSizeY = batchData->halfSizeY().data();
	const float *d_obbHalfSizeZ = batchData->halfSizeZ().data();
	auto d_axes = batchData->axes();

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
				const auto &axisBatch = d_axes[axisIdx];
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

		const float *axis0X = d_axes[0].x.data() + queuedIdx;
		const float *axis0Y = d_axes[0].y.data() + queuedIdx;
		const float *axis0Z = d_axes[0].z.data() + queuedIdx;
		const float *axis1X = d_axes[1].x.data() + queuedIdx;
		const float *axis1Y = d_axes[1].y.data() + queuedIdx;
		const float *axis1Z = d_axes[1].z.data() + queuedIdx;
		const float *axis2X = d_axes[2].x.data() + queuedIdx;
		const float *axis2Y = d_axes[2].y.data() + queuedIdx;
		const float *axis2Z = d_axes[2].z.data() + queuedIdx;

		const OBB &obb = *static_cast<OBB *>(shapes[itemIdx].get());
		const Vec3f &obbCenter = obb.tfOrigin();

		bool isOutside = false;
		for (unsigned int planeIdx = 0u; planeIdx < NUM_FRUSTUM_PLANES && !isOutside; ++planeIdx) {
			const auto &plane = frustum->planes[planeIdx];
			const Vec3f &n = plane.xyz_();
			// center-to-plane distance + projected radius
			float dr = n.dot(obbCenter) - plane.w +
				d_obbHalfSizeX[queuedIdx] * std::abs(n.dot(axis0X, axis0Y, axis0Z)) +
				d_obbHalfSizeY[queuedIdx] * std::abs(n.dot(axis1X, axis1Y, axis1Z)) +
				d_obbHalfSizeZ[queuedIdx] * std::abs(n.dot(axis2X, axis2Y, axis2Z));
			// completely outside
			isOutside = (dr < 0.0f);
		}
		if (!isOutside) {
			td.hits->push(itemIdx);
		}
	}
}

bool Frustum::hasIntersectionWithFrustum(const Frustum &other) const {
	for (const auto & plane : other.planes) {
		if (plane.distance(points[0]) < 0 &&
			plane.distance(points[1]) < 0 &&
			plane.distance(points[2]) < 0 &&
			plane.distance(points[3]) < 0 &&
			plane.distance(points[4]) < 0 &&
			plane.distance(points[5]) < 0 &&
			plane.distance(points[6]) < 0 &&
			plane.distance(points[7]) < 0) {
			return false;
			}
	}
	return true;
}

void Frustum::batchTest_Frustums(BatchedIntersectionCase &td) {
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

