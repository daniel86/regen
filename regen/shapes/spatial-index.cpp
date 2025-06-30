#include <regen/utility/logging.h>
#include <algorithm>

#include "spatial-index.h"
#include "quad-tree.h"

// NOTE: this piece of code is performance critical! For many execution paths:
// - avoid the use of std::set, std::unordered_set, std::map, std::unordered_map, etc. here
// 		- also iteration over these containers is expensive!
// - avoid lambda functions
// - avoid alloc/free

using namespace regen;

SpatialIndex::SpatialIndex() {
}

void SpatialIndex::addToIndex(const ref_ptr<BoundingShape> &shape) {
	nameToShape_[shape->name()].push_back(shape);
	for (auto &ic: cameras_) {
		createIndexShape(ic.second, shape);
	}
}

void SpatialIndex::removeFromIndex(const ref_ptr<BoundingShape> &shape) {
	auto it = nameToShape_.find(shape->name());
	if (it != nameToShape_.end()) {
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
	for (auto &pair: nameToShape_) {
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
	return it->second.nameToShape_[shapeName];
}

bool SpatialIndex::isVisible(const Camera &camera, std::string_view shapeID) {
	auto it = cameras_.find(&camera);
	if (it == cameras_.end()) {
		return true;
	}
	auto it2 = it->second.nameToShape_.find(shapeID);
	if (it2 == it->second.nameToShape_.end()) {
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
	auto it = nameToShape_.find(shapeID);
	if (it != nameToShape_.end() && !it->second.empty()) {
		return it->second.front();
	}
	return {};
}

void SpatialIndex::handleIntersection_sorted(const BoundingShape &b_shape, void *userData) {
	auto *data = (SpatialIndex::TraversalData*)(userData);
	auto *index_shape = (IndexedShape*) b_shape.spatialIndexData_;
	index_shape->u_visible_ = true;

	if (b_shape.numInstances() > 1) {
		// Skip if we already added this instance
		if (data->isMultiShape && b_shape.spatialIndexVisible_) { return; }
		b_shape.spatialIndexVisible_ = true;
		float d = (b_shape.getShapeOrigin() - data->camPos->xyz_()).lengthSquared();
		index_shape->instanceDistances_.push_back({&b_shape, d});
	}
}

void SpatialIndex::handleIntersection_unsorted(const BoundingShape &b_shape, void *userData) {
	auto *data = (SpatialIndex::TraversalData*)(userData);
	auto *index_shape = (IndexedShape*) b_shape.spatialIndexData_;
	auto mapped_data = index_shape->mappedInstanceIDs();
	index_shape->u_visible_ = true;
	if (b_shape.numInstances() > 1) {
		// Skip if we already added this instance
		if (data->isMultiShape && b_shape.spatialIndexVisible_) { return; }
		b_shape.spatialIndexVisible_ = true;
		index_shape->u_instanceCount_ += 1;
		mapped_data[index_shape->u_instanceCount_] = b_shape.instanceID();
		mapped_data[0] = index_shape->u_instanceCount_;
	}
}

void SpatialIndex::updateVisibilityWithCamera(IndexCamera &ic, const BoundingShape &camera_shape, bool isMultiShape) {
	TraversalData traversalData{this, nullptr, isMultiShape};

	if (ic.sortInstances) {
		for (auto &indexShape: ic.indexShapes_) {
			indexShape->instanceDistances_.clear();
		}

		auto camPos = ic.camera->position()->getVertex(0);
		traversalData.camPos = &camPos.r;

		foreachIntersection(camera_shape, SpatialIndex::handleIntersection_sorted, &traversalData);
		for (auto &indexShape: ic.indexShapes_) {
			auto &distances = indexShape->instanceDistances_;
			if (distances.empty()) { continue; }
			std::ranges::sort(distances, {}, &IndexedShape::ShapeDistance::distance);

			uint32_t startIdx = indexShape->u_instanceCount_;
			indexShape->u_instanceCount_ = startIdx + static_cast<unsigned int>(distances.size());
			auto mapped_data = indexShape->mappedInstanceIDs();
			// convention: first element is the number of visible instances
			mapped_data[0] = indexShape->u_instanceCount_;
			std::transform(
				distances.begin(), distances.end(),
				mapped_data + startIdx + 1,
				[](const auto &d) { return d.shape->instanceID(); });
		}
	} else {
		foreachIntersection(camera_shape, handleIntersection_unsorted, &traversalData);
	}
}

void SpatialIndex::updateVisibility() {
	for (auto &ic: cameras_) {
		for (auto &indexShape: ic.second.indexShapes_) {
			// keep instance IDs mapped for writing during visibility update
			indexShape->mapInstanceIDs_internal();
			// note: first element is the number of visible instances
			indexShape->mappedInstanceIDs()[0] = 0;
			indexShape->u_instanceCount_ = 0;
			indexShape->u_visible_ = false;
			for (auto &bs: indexShape->boundingShapes_) {
				// Remember the index shape to bounding shape mapping such that we can
				// obtain index shape from bounding shape directly (else a hash lookup would be required).
				bs->spatialIndexData_ = indexShape;
				bs->spatialIndexVisible_ = false;
			}
		}

		if (ic.second.camera->isOmni()) {
			// omni camera -> intersection test with bounding sphere
			auto projParams = ic.first->projParams()->getVertex(0);
			BoundingSphere sphereShape(Vec3f::zero(), projParams.r.y);
			sphereShape.setTransform(ref_ptr<ModelTransformation>::alloc(ic.first->position()));
			sphereShape.updateTransform(true);
			updateVisibilityWithCamera(ic.second, sphereShape, false);
		}
			//else if (ic.second.camera->isSemiOmni()) {
			//	// TODO: Support half-spheres for culling
			//}
		else {
			// spot camera -> intersection test with view frustum
			// TODO: In case of cascaded frustums, rather find enclosing frustum and only
			//       do one intersection test. would probably give us more false positives, but
			//       might still be faster.
			auto &frustumShapes = ic.first->frustum();
			for (auto &frustumShape: frustumShapes) {
				updateVisibilityWithCamera(ic.second, frustumShape, frustumShapes.size() > 1);
			}
		}

		for (auto &indexShape: ic.second.indexShapes_) {
			indexShape->unmapInstanceIDs_internal();
			indexShape->visible_ = indexShape->u_visible_;
			indexShape->instanceCount_ = indexShape->u_instanceCount_;
		}
	}
}

void SpatialIndex::createIndexShape(IndexCamera &ic, const ref_ptr<BoundingShape> &shape) {
	auto needle = ic.nameToShape_.find(shape->name());
	if (needle != ic.nameToShape_.end()) {
		// already created
		needle->second->boundingShapes_.push_back(shape);
		return;
	}
	auto is = ref_ptr<IndexedShape>::alloc(ic.camera, shape);
	is->visibleVec_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDs", 1);
	is->visibleVec_->setInstanceData(shape->numInstances() + 1, 1, nullptr);
	auto mapped = is->visibleVec_->mapClientData<unsigned int>(ShaderData::WRITE);
	for (unsigned int i = 0; i < shape->numInstances(); ++i) {
		mapped.w[i + 1] = i;
	}
	mapped.w[0] = shape->numInstances();
	ic.nameToShape_[shape->name()] = is;
	ic.indexShapes_.push_back(is.get());
	is->boundingShapes_.push_back(shape);
}

ref_ptr<SpatialIndex> SpatialIndex::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto indexType = input.getValue<std::string>("type", "quadtree");
	ref_ptr<SpatialIndex> index;

	if (indexType == "quadtree") {
		auto quadTree = ref_ptr<QuadTree>::alloc();
		//quadTree->setMaxObjectsPerNode(input.getValue<GLuint>("max-objects-per-node", 4u));
		quadTree->setMinNodeSize(input.getValue<float>("min-node-size", 0.1f));

		if (input.hasAttribute("test-mode-3d")) {
			auto testMode = input.getValue("test-mode-3d");
			if (testMode == "closest") {
				quadTree->setTestMode3D(QuadTree::QUAD_TREE_3D_TEST_CLOSEST);
			} else if (testMode == "all") {
				quadTree->setTestMode3D(QuadTree::QUAD_TREE_3D_TEST_ALL);
			} else {
				quadTree->setTestMode3D(QuadTree::QUAD_TREE_3D_TEST_NONE);
			}
		}
		if (input.hasAttribute("close-distance")) {
			quadTree->setCloseDistanceSquared(input.getValue<float>("close-distance", 20.0f));
		}

		index = quadTree;
	}

	return index;
}
