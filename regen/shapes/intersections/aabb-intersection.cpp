#include "aabb-intersection.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"

void regen::shapes::flush_AABB_Spheres(BatchedIntersectionCase &td) {
	// TODO: Support SIMD on this path.
	// Vec3f closestPoint = other.closestPointOnSurface(p_this);
	// return (closestPoint - p_this).lengthSquared() <= radiusSquared_;
	auto *testShape = static_cast<const BoundingBox *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		const BoundingSphere &sphere = *static_cast<BoundingSphere *>(shapes[td.queuedIndices[i]].get());
		if (sphere.hasIntersectionWithShape(*testShape)) {
			td.callback.fun(sphere, td.callback.userData);
		}
	}
}

void regen::shapes::flush_AABB_AABBs(BatchedIntersectionCase &td) {
	// TODO: Support SIMD on this path.
	//const Vec3f &aMin = tfBounds().min;
	//const Vec3f &aMax = tfBounds().max;
	//const Vec3f &bMin = other.tfBounds().min;
	//const Vec3f &bMax = other.tfBounds().max;
	//return aMin.x < bMax.x && aMax.x > bMin.x &&
	//	   aMin.y < bMax.y && aMax.y > bMin.y &&
	//	   aMin.z < bMax.z && aMax.z > bMin.z;
	auto *testShape = static_cast<const BoundingBox *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		const BoundingBox &box = *static_cast<BoundingBox *>(shapes[td.queuedIndices[i]].get());
		if (testShape->hasIntersectionWithBox(box)) {
			td.callback.fun(box, td.callback.userData);
		}
	}
}

void regen::shapes::flush_AABB_OBBs(BatchedIntersectionCase &td) {
	auto *testShape = static_cast<const BoundingBox *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		const BoundingBox &box = *static_cast<BoundingBox *>(shapes[td.queuedIndices[i]].get());
		if (testShape->hasIntersectionWithBox(box)) {
			td.callback.fun(box, td.callback.userData);
		}
	}
}

void regen::shapes::flush_AABB_Frustums(BatchedIntersectionCase &td) {
	// note: index shapes are rarely frustum, so no SIMD optimization here.
	auto *testShape = static_cast<const BoundingBox *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		const Frustum &frustum = *static_cast<Frustum *>(shapes[td.queuedIndices[i]].get());
		if (frustum.hasIntersectionWithBox(*testShape)) {
			td.callback.fun(frustum, td.callback.userData);
		}
	}
}
