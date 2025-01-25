#include <regen/utility/logging.h>
#include <algorithm>

#include "spatial-index.h"
#include "regen/camera/cube-camera.h"
#include "regen/camera/paraboloid-camera.h"

using namespace regen;

SpatialIndex::SpatialIndex()
	: threadPool_(std::max(2u, std::max(2u,std::thread::hardware_concurrency()) - 2u)) {
}

void SpatialIndex::addToIndex(const ref_ptr<BoundingShape> &shape) {
	shapes_[shape->name()].push_back(shape);
}

void SpatialIndex::removeFromIndex(const ref_ptr<BoundingShape> &shape) {
	auto it = shapes_.find(shape->name());
	if (it != shapes_.end()) {
		auto jt = std::find(it->second.begin(), it->second.end(), shape);
		if (jt != it->second.end()) {
			it->second.erase(jt);
		}
	}
}

void SpatialIndex::addCamera(const ref_ptr<Camera> &camera, bool sortInstances) {
	bool isOmniCamera = dynamic_cast<OmniDirectionalCamera *>(camera.get()) != nullptr;
	cameras_.emplace(camera.get(),
					 IndexCamera{camera, {}, {}, sortInstances, isOmniCamera});
}

bool SpatialIndex::hasCamera(const Camera &camera) const {
	return cameras_.find(&camera) != cameras_.end();
}

bool SpatialIndex::isVisible(const Camera &camera, std::string_view shapeID) {
	auto it = cameras_.find(&camera);
	if (it == cameras_.end()) {
		return true;
	}
	return it->second.visibleShapes.count(shapeID) > 0;
}

const std::vector<unsigned int> &
SpatialIndex::getVisibleInstances(const Camera &camera, std::string_view shapeID) const {
	static std::vector<unsigned int> empty;
	auto it = cameras_.find(&camera);
	if (it == cameras_.end()) {
		return empty;
	}
	auto it2 = it->second.visibleInstances.find(shapeID);
	if (it2 == it->second.visibleInstances.end()) {
		return empty;
	}
	return it2->second;
}

GLuint SpatialIndex::numInstances(std::string_view shapeID) const {
	auto shape = getShape(shapeID);
	if (!shape.get()) {
		return 0u;
	}
	return shape->numInstances();
}

ref_ptr<Mesh> SpatialIndex::getMeshOfShape(std::string_view shapeID) const {
	auto shape = getShape(shapeID);
	if (!shape.get()) {
		return {};
	}
	return shape->mesh();
}

ref_ptr<BoundingShape> SpatialIndex::getShape(std::string_view shapeID) const {
	auto it = shapes_.find(shapeID);
	if (it != shapes_.end() && !it->second.empty()) {
		return it->second.front();
	}
	return {};
}

void SpatialIndex::updateVisibility(IndexCamera &ic, const BoundingShape &a_shape) {
	ic.visibleShapes.clear();
	ic.visibleInstances.clear();

	if (ic.sortInstances) {
		auto &camPos = ic.camera->position()->getVertex(0);
		struct ShapeDistance {
			const BoundingShape *shape;
			float distance;
		};
		std::map<std::string_view, std::vector<ShapeDistance>> instanceDistances;
		foreachIntersection(a_shape, [&](const BoundingShape &b_shape) {
			std::lock_guard<std::mutex> lock(mutex_);
			ic.visibleShapes.insert(b_shape.name());
			if (b_shape.numInstances() > 1) {
				float d = (b_shape.getCenterPosition() - camPos).length();
				instanceDistances[b_shape.name()].push_back({&b_shape, d});
			}
		});
		for (auto &pair: instanceDistances) {
			auto &distances = pair.second;
			std::sort(distances.begin(), distances.end(), [](const ShapeDistance &a, const ShapeDistance &b) {
				return a.distance < b.distance;
			});
			auto &visibleInstances = ic.visibleInstances[pair.first];
			visibleInstances.reserve(distances.size());
			for (auto &distance: distances) {
				visibleInstances.push_back(distance.shape->instanceID());
			}
		}
	} else {
		foreachIntersection(a_shape, [&](const BoundingShape &b_shape) {
			std::lock_guard<std::mutex> lock(mutex_);
			ic.visibleShapes.insert(b_shape.name());
			if (b_shape.numInstances() > 1) {
				auto &visibleInstances = ic.visibleInstances[b_shape.name()];
				visibleInstances.push_back(b_shape.instanceID());
			}
		});
	}
}

void SpatialIndex::updateVisibility() {
	for (auto &ic: cameras_) {
		if (ic.second.isOmni) {
			// omni camera -> intersection test with bounding sphere
			// TODO: Support half-spheres for culling
			BoundingSphere sphereShape(Vec3f::zero(), ic.first->far()->getVertex(0));
			sphereShape.setTransform(ic.first->position());
			sphereShape.updateTransform(true);
			// update visible shapes
			updateVisibility(ic.second, sphereShape);
		} else {
			// spot camera -> intersection test with view frustum
			auto &frustumShape = ic.first->frustum();
			// update visible shapes
			updateVisibility(ic.second, frustumShape);
		}
	}
}
