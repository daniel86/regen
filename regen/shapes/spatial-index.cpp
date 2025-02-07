#include <regen/utility/logging.h>
#include <algorithm>

#include "spatial-index.h"

using namespace regen;

SpatialIndex::SpatialIndex()
	: threadPool_(std::max(2u,std::thread::hardware_concurrency()) - 2u) {
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
	cameras_.emplace(camera.get(),
					 IndexCamera{camera, {}, {}, {}, sortInstances});
}

bool SpatialIndex::hasCamera(const Camera &camera) const {
	return cameras_.find(&camera) != cameras_.end();
}

std::vector<const Camera*> SpatialIndex::cameras() const {
	std::vector<const Camera*> result;
	for (auto &pair: cameras_) {
		result.push_back(pair.first);
	}
	return result;
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
	auto it2 = it->second.visibleInstances_vec.find(shapeID);
	if (it2 == it->second.visibleInstances_vec.end()) {
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

void SpatialIndex::updateVisibility(IndexCamera &ic, const BoundingShape &a_shape, bool isMultiShape) {
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
				if (isMultiShape) {
					// make sure we don't add the same instance twice
					auto &visibleInstances = ic.visibleInstances[b_shape.name()];
					auto [_,inserted] = visibleInstances.insert(b_shape.instanceID());
					if (!inserted) return;
				}
				float d = (b_shape.getCenterPosition() - camPos).length();
				instanceDistances[b_shape.name()].push_back({&b_shape, d});
			}
		});
		for (auto &pair: instanceDistances) {
			auto &distances = pair.second;
			std::sort(distances.begin(), distances.end(), [](const ShapeDistance &a, const ShapeDistance &b) {
				return a.distance < b.distance;
			});
			auto &visibleInstances = ic.visibleInstances_vec[pair.first];
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
				if (isMultiShape) {
					// make sure we don't add the same instance twice
					auto &visibleInstances = ic.visibleInstances[b_shape.name()];
					auto [_,inserted] = visibleInstances.insert(b_shape.instanceID());
					if (!inserted) return;
				}
				auto &visibleInstances = ic.visibleInstances_vec[b_shape.name()];
				visibleInstances.push_back(b_shape.instanceID());
			}
		});
	}
}

void SpatialIndex::updateVisibility() {
	for (auto &ic: cameras_) {
		ic.second.visibleShapes.clear();
		ic.second.visibleInstances.clear();
		ic.second.visibleInstances_vec.clear();

		if (ic.second.camera->isOmni()) {
			// omni camera -> intersection test with bounding sphere
			BoundingSphere sphereShape(Vec3f::zero(), ic.first->far()->getVertex(0));
			sphereShape.setTransform(ic.first->position());
			sphereShape.updateTransform(true);
			updateVisibility(ic.second, sphereShape, false);
		}
		//else if (ic.second.camera->isSemiOmni()) {
		//	// TODO: Support half-spheres for culling
		//}
		else {
			// spot camera -> intersection test with view frustum
			auto &frustumShapes = ic.first->frustum();
			for (auto &frustumShape: frustumShapes) {
				updateVisibility(ic.second, frustumShape, frustumShapes.size() > 1);
			}
		}
	}
}
