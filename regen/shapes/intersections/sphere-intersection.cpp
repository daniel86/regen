#include "sphere-intersection.h"
#include "regen/shapes/bounding-box.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"

void regen::shapes::flush_Sphere_Spheres(BatchedIntersectionCase &td) {
	// TODO: Support SIMD on this path.
	// const float r_sum = radiusSquared_ + other.radiusSquared_;
	// return (p_this - p_other).lengthSquared() <= (r_sum * r_sum);
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const BoundingSphere &sphere = *static_cast<BoundingSphere *>(shapes[itemIdx].get());
		if (testShape->hasIntersectionWithSphere(sphere)) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Sphere_Boxes(BatchedIntersectionCase &td) {
	// TODO: Support SIMD on this path.
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const BoundingBox &box = *static_cast<BoundingBox *>(shapes[itemIdx].get());
		if (testShape->hasIntersectionWithShape(box)) {
			td.hits->push(itemIdx);
		}
	}
}

void regen::shapes::flush_Sphere_Frustums(BatchedIntersectionCase &td) {
	// note: index shapes are rarely frustum, so no SIMD optimization here.
	auto *testShape = static_cast<const BoundingSphere *>(td.testShape);
	auto *shapes = td.indexedShapes->data();
	for (uint32_t i = 0; i < td.numQueued; ++i) {
		auto itemIdx = td.queuedIndices[i];
		const Frustum &frustum = *static_cast<Frustum *>(shapes[itemIdx].get());
		if (frustum.hasIntersectionWithSphere(*testShape)) {
			td.hits->push(itemIdx);
		}
	}
}
