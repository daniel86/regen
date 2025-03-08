#include <regen/utility/logging.h>
#include <algorithm>

#include "spatial-index.h"
#include "quad-tree.h"

using namespace regen;

SpatialIndex::SpatialIndex()
		: threadPool_(std::max(2u, std::thread::hardware_concurrency()) - 2u) {
}

void SpatialIndex::addToIndex(const ref_ptr<BoundingShape> &shape) {
	shapes_[shape->name()].push_back(shape);
	for (auto &ic: cameras_) {
		createIndexShape(ic.second, shape);
	}
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
	auto &data = cameras_[camera.get()];
	data.camera = camera;
	data.sortInstances = sortInstances;
	for (auto &pair: shapes_) {
		for (auto &shape: pair.second) {
			createIndexShape(data, shape);
		}
	}
}

bool SpatialIndex::hasCamera(const Camera &camera) const {
	return cameras_.find(&camera) != cameras_.end();
}

std::vector<const Camera *> SpatialIndex::cameras() const {
	std::vector<const Camera *> result;
	for (auto &pair: cameras_) {
		result.push_back(pair.first);
	}
	return result;
}

ref_ptr<IndexedShape> SpatialIndex::getIndexedShape(const ref_ptr<Camera> &camera, std::string_view shapeName) {
	auto it = cameras_.find(camera.get());
	if (it == cameras_.end()) {
		return {};
	}
	return it->second.shapes[shapeName];
}

bool SpatialIndex::isVisible(const Camera &camera, std::string_view shapeID) {
	auto it = cameras_.find(&camera);
	if (it == cameras_.end()) {
		return true;
	}
	auto it2 = it->second.shapes.find(shapeID);
	if (it2 == it->second.shapes.end()) {
		return true;
	}
	return it2->second->isVisible();
}

GLuint SpatialIndex::numInstances(std::string_view shapeID) const {
	auto shape = getShape(shapeID);
	if (!shape.get()) {
		return 0u;
	}
	return shape->numInstances();
}

ref_ptr<BoundingShape> SpatialIndex::getShape(std::string_view shapeID) const {
	auto it = shapes_.find(shapeID);
	if (it != shapes_.end() && !it->second.empty()) {
		return it->second.front();
	}
	return {};
}

void SpatialIndex::updateVisibility(IndexCamera &ic, const BoundingShape &camera_shape, bool isMultiShape) {
	if (ic.sortInstances) {
		for (auto &pair: ic.shapes) {
			pair.second->instanceDistances_.clear();
		}

		auto camPos = ic.camera->position()->getVertex(0);
		foreachIntersection(camera_shape, [&](const BoundingShape &b_shape) {
			//std::lock_guard<std::mutex> lock(mutex_);
			auto &index_shape = ic.shapes[b_shape.name()];
			index_shape->u_visible_ = true;

			if (b_shape.numInstances() > 1) {
				if (isMultiShape) {
					// make sure we don't add the same instance twice
					auto [_, inserted] = index_shape->u_visibleSet_.insert(b_shape.instanceID());
					if (!inserted) return;
				}
				float d = (b_shape.getCenterPosition() - camPos.r).length();
				index_shape->instanceDistances_.push_back({&b_shape, d});
			}
		});
		for (auto &pair: ic.shapes) {
			auto &index_shape = pair.second;
			auto &distances = index_shape->instanceDistances_;
			if (distances.empty()) {
				continue;
			}
			std::sort(distances.begin(), distances.end(),
					  [](const IndexedShape::ShapeDistance &a, const IndexedShape::ShapeDistance &b) {
						  return a.distance < b.distance;
					  });
			auto mapped_data = index_shape->mappedInstanceIDs();
			for (auto &distance: distances) {
				index_shape->u_instanceCount_ += 1;
				mapped_data[index_shape->u_instanceCount_] = distance.shape->instanceID();
				mapped_data[0] = index_shape->u_instanceCount_;
			}
		}
	} else {
		foreachIntersection(camera_shape, [&](const BoundingShape &b_shape) {
			//std::lock_guard<std::mutex> lock(mutex_);
			auto &index_shape = ic.shapes[b_shape.name()];
			auto mapped_data = index_shape->mappedInstanceIDs();
			index_shape->u_visible_ = true;

			if (b_shape.numInstances() > 1) {
				if (isMultiShape) {
					// make sure we don't add the same instance twice
					auto [_, inserted] = index_shape->u_visibleSet_.insert(b_shape.instanceID());
					if (!inserted) return;
				}
				index_shape->u_instanceCount_ += 1;
				mapped_data[index_shape->u_instanceCount_] = b_shape.instanceID();
				mapped_data[0] = index_shape->u_instanceCount_;
			}
		});
	}
}

void SpatialIndex::updateVisibility() {
	for (auto &ic: cameras_) {
		for (auto &pair: ic.second.shapes) {
			// keep instance IDs mapped for writing during visibility update
			pair.second->mapInstanceIDs_internal();
			// note: first element is the number of visible instances
			pair.second->mappedInstanceIDs()[0] = 0;
			pair.second->u_instanceCount_ = 0;
			pair.second->u_visible_ = false;
			pair.second->u_visibleSet_.clear();
		}

		if (ic.second.camera->isOmni()) {
			// omni camera -> intersection test with bounding sphere
			BoundingSphere sphereShape(Vec3f::zero(), ic.first->far()->getVertex(0).r);
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

		for (auto &pair: ic.second.shapes) {
			pair.second->unmapInstanceIDs_internal();
			pair.second->visible_ = pair.second->u_visible_;
			pair.second->instanceCount_ = pair.second->u_instanceCount_;
		}
	}
}

void SpatialIndex::createIndexShape(IndexCamera &ic, const ref_ptr<BoundingShape> &shape) {
	auto is = ref_ptr<IndexedShape>::alloc(ic.camera, shape);
	is->visibleVec_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDs", 1);
	is->visibleVec_->setInstanceData(shape->numInstances() + 1, 1, nullptr);
	auto mapped = is->visibleVec_->mapClientData<unsigned int>(ShaderData::WRITE);
	for (unsigned int i = 0; i < shape->numInstances(); ++i) {
		mapped.w[i + 1] = i;
	}
	mapped.w[0] = shape->numInstances();
	ic.shapes[shape->name()] = is;
}

ref_ptr<SpatialIndex> SpatialIndex::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto indexType = input.getValue<std::string>("type", "quadtree");
	ref_ptr<SpatialIndex> index;

	if (indexType == "quadtree") {
		auto quadTree = ref_ptr<QuadTree>::alloc();
		//quadTree->setMaxObjectsPerNode(input.getValue<GLuint>("max-objects-per-node", 4u));
		quadTree->setMinNodeSize(input.getValue<float>("min-node-size", 0.1f));
		index = quadTree;
	}

	return index;
}
