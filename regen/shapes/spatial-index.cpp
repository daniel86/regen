#include <regen/utility/logging.h>

#include "spatial-index.h"

using namespace regen;

void SpatialIndex::addToIndex(const ref_ptr<BoundingShape> &shape) {
	shapes_.push_back(shape);
}

void SpatialIndex::removeFromIndex(const ref_ptr<BoundingShape> &shape) {
	auto it = std::find(shapes_.begin(), shapes_.end(), shape);
	if (it != shapes_.end()) {
		shapes_.erase(it);
	}
}

void SpatialIndex::addCamera(const ref_ptr<Camera> &camera) {
	auto omniCamera = dynamic_cast<OmniDirectionalCamera *>(camera.get());
	if (omniCamera) {
		omniCameras_.push_back(camera);
	} else {
		cameras_.push_back(camera);
	}
}

bool SpatialIndex::isVisible(const Camera &camera, std::string_view shapeID) {
	auto it = visibleShapes_.find(&camera);
	if (it == visibleShapes_.end()) {
		return true;
	}
	return it->second.count(shapeID) > 0;
}

const std::vector<unsigned int> &
SpatialIndex::getVisibleInstances(const Camera &camera, std::string_view shapeID) {
	static std::vector<unsigned int> empty;
	auto it = visibleInstances_.find(&camera);
	if (it == visibleInstances_.end()) {
		return empty;
	}
	auto it2 = it->second.find(shapeID);
	if (it2 == it->second.end()) {
		return empty;
	}
	return it2->second;
}

GLuint SpatialIndex::numInstances(std::string_view shapeID) const {
	for (auto &shape: shapes_) {
		if (shape->name() == shapeID) {
			return shape->numInstances();
		}
	}
	return 0u;
}

ref_ptr<Mesh> SpatialIndex::getMeshOfShape(std::string_view shapeID) const {
	for (auto &shape: shapes_) {
		if (shape->name() == shapeID) {
			return shape->mesh();
		}
	}
	return {};
}

ref_ptr<BoundingShape> SpatialIndex::getShape(std::string_view shapeID) const {
	for (auto &shape: shapes_) {
		if (shape->name() == shapeID) {
			return shape;
		}
	}
	return {};
}

void SpatialIndex::updateVisibility(const ref_ptr<Camera> &camera, const BoundingShape &a_shape) {
	auto &shapes = visibleShapes_[camera.get()];
	auto &instances = visibleInstances_[camera.get()];
	shapes.clear();
	instances.clear();
	foreachIntersection(a_shape, [&](const BoundingShape &b_shape) {
		shapes.insert(b_shape.name());
		if (b_shape.numInstances() > 1) {
			auto &visibleInstances = instances[b_shape.name()];
			visibleInstances.push_back(b_shape.instanceID());
		}
	});
}

void SpatialIndex::updateVisibility() {
	for (auto &camera: cameras_) {
		// spot camera -> intersection test with view frustum
		auto &frustumShape = camera->frustum();
		// update visible shapes
		updateVisibility(camera, frustumShape);
	}
	for (auto &camera: omniCameras_) {
		// omni camera -> intersection test with bounding sphere
		// TODO: Support half-spheres for culling
		BoundingSphere sphereShape(Vec3f::zero(), camera->far()->getVertex(0));
		sphereShape.setTransform(camera->position());
		sphereShape.updateTransform(true);
		// update visible shapes
		updateVisibility(camera, sphereShape);
	}
}
