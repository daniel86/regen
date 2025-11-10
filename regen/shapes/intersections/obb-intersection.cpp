#include "obb-intersection.h"

#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"

void regen::shapes::flush_OBB_Spheres(BatchedIntersectionCase &td) {
	// TODO Can be done well with SIMD:
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

void regen::shapes::flush_OBB_Boxes(BatchedIntersectionCase &td) {
	auto *testShape = static_cast<const BoundingBox *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		const BoundingBox &box = *static_cast<BoundingBox *>(shapes[td.queuedIndices[i]].get());
		if (testShape->hasIntersectionWithBox(box)) {
			td.callback.fun(box, td.callback.userData);
		}
	}
}

void regen::shapes::flush_OBB_Frustums(BatchedIntersectionCase &td) {
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
