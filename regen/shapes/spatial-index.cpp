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

void SpatialIndex::addCamera(
		const ref_ptr<Camera> &cullCamera,
		const ref_ptr<Camera> &sortCamera,
		SortMode sortMode,
		Vec4i lodShift) {
	auto &data = cameras_[cullCamera.get()];
	data.cullCamera = cullCamera;
	data.sortCamera = sortCamera;
	data.sortMode = sortMode;
	data.lodShift = lodShift;
	for (auto &pair: nameToShape_) {
		for (auto &shape: pair.second) {
			createIndexShape(data, shape);
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
	const uint32_t numLayer = ic.cullCamera->numLayer();
	const uint32_t numInstances = shape->numInstances();
	const uint32_t numIndices = numInstances * numLayer;

	auto is = ref_ptr<IndexedShape>::alloc(ic.cullCamera, ic.sortCamera, shape);
	const uint32_t numLOD = is->numLODs();
	is->idVec_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDs", 1);
	is->idVec_->setInstanceData(numIndices, 1, nullptr);
	is->countVec_ = ref_ptr<ShaderInput1ui>::alloc("instanceCounts", numLayer * numLOD);
	is->countVec_->setInstanceData(1, 1, nullptr);
	is->baseVec_ = ref_ptr<ShaderInput1ui>::alloc("baseInstances", numLayer * numLOD);
	is->baseVec_->setInstanceData(1, 1, nullptr);
	is->setLODShift(ic.lodShift);

	auto mapped_ids = is->idVec_->mapClientData<uint32_t>(BUFFER_GPU_WRITE);
	auto mapped_count = is->countVec_->mapClientData<uint32_t>(BUFFER_GPU_WRITE);
	auto mapped_base = is->baseVec_->mapClientData<uint32_t>(BUFFER_GPU_WRITE);
	for (unsigned int instanceIdx = 0; instanceIdx < numInstances; ++instanceIdx) {
		// write instance data for every layer
		for (unsigned int layerIdx = 0; layerIdx < numLayer; ++layerIdx) {
			mapped_ids.w[instanceIdx + layerIdx * numInstances] = instanceIdx;
		}
	}
	for (unsigned int layerIdx = 0; layerIdx < numLayer; ++layerIdx) {
		for (unsigned int lodLevel = 0; lodLevel < numLOD; ++lodLevel) {
			uint32_t binIdx = lodLevel * numLayer + layerIdx;
			if (lodLevel==0) {
				mapped_count.w[binIdx] = numInstances;
				mapped_base.w[binIdx] = layerIdx * numInstances;
			} else {
				mapped_count.w[binIdx] = 0;
				mapped_base.w[binIdx] = (layerIdx+1) * numInstances;
			}
		}
	}
	mapped_base.unmap();
	mapped_count.unmap();
	mapped_ids.unmap();

	ic.nameToShape_[shape->name()] = is;
	ic.indexShapes_.push_back(is.get());
	is->boundingShapes_.push_back(shape);
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

bool SpatialIndex::isVisible(const Camera &camera, uint32_t layerIdx, std::string_view shapeID) {
	auto it = cameras_.find(&camera);
	if (it == cameras_.end()) {
		return true;
	}
	auto it2 = it->second.nameToShape_.find(shapeID);
	if (it2 == it->second.nameToShape_.end()) {
		return true;
	}
	return it2->second->isVisibleInLayer(layerIdx);
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

static inline uint32_t getLODLevel(
			const BoundingShape &b_shape,
			const IndexedShape *i_shape,
			float lodDistance) {
	return b_shape.baseMesh()->getLODLevel(lodDistance, i_shape->lodShift());
}

void SpatialIndex::handleIntersection(const BoundingShape& b_shape, void* userData) {
    auto* data = (SpatialIndex::TraversalData*)(userData);
    auto* i_shape = (IndexedShape*) b_shape.spatialIndexData_;
    const uint32_t L = i_shape->camera()->numLayer();
	const uint32_t l = data->layerIdx;

    // toggle visibility for this layer
    i_shape->tmp_layerVisibility_[l] = true;

    // compute LOD level for this shape by distance to camera.
    // each mesh may have its own thresholds for switching LOD levels, so we need
    // to let the (base) mesh decide which LOD level to use.
	const float lodDistance = (b_shape.getShapeOrigin() - *data->camPos).lengthSquared();
	const uint32_t k = getLODLevel(b_shape, i_shape, lodDistance);

	// Finally bin the shape into the (lod, layer) bin
	const uint32_t b = IndexedShape::binIdx(k, l, L);
	i_shape->tmp_layerShapes_[l].push_back({
		b_shape.instanceID(),
		lodDistance });
	i_shape->tmp_binCounts_[b] += 1;
}

void SpatialIndex::updateLayerVisibility(
		IndexCamera &ic, uint32_t layerIdx,
		const BoundingShape &camera_shape) {
	// Collect all intersections for this layer into (lod,layer) bins
	TraversalData traversalData{ this, nullptr, layerIdx };
	if (ic.sortCamera->position().size()>1) {
		traversalData.camPos = &ic.sortCamera->position(layerIdx);
	} else {
		traversalData.camPos = &ic.sortCamera->position(0);
	}
	foreachIntersection(camera_shape, handleIntersection, &traversalData);
}

void SpatialIndex::updateVisibility() {
	for (auto &ic: cameras_) {
		auto &indexCamera = ic.second;
		const uint32_t L = indexCamera.cullCamera->numLayer();

		for (auto &indexShape: ic.second.indexShapes_) {
			const uint32_t K = indexShape->numLODs();
			const uint32_t B = K * L;
			indexShape->tmp_layerVisibility_.assign(L, false);
			// Reset the per-bin counts, we accumulate them during traversal, so they
			// need to be reset first.
			std::memset(indexShape->tmp_binCounts_.data(), 0, sizeof(uint32_t) * B);
			// Clear the per-layer bins.
			for (uint32_t l=0; l<L; ++l) {
				indexShape->tmp_layerShapes_[l].clear();
			}
			// Remember the index shape to bounding shape mapping such that we can
			// obtain index shape from bounding shape directly (else a hash lookup would be required).
			for (auto &bs: indexShape->boundingShapes_) {
				bs->spatialIndexData_ = indexShape;
			}
		}

		auto &frustumShapes = ic.first->frustum();
		for (uint32_t layerIdx = 0; layerIdx < frustumShapes.size(); ++layerIdx) {
			updateLayerVisibility(ic.second, layerIdx, frustumShapes[layerIdx]);
		}
		for (auto &indexShape: ic.second.indexShapes_) {
			updateLOD_Major(indexCamera, indexShape);
		}
	}
}

void SpatialIndex::updateLOD_Major(IndexCamera &indexCamera, IndexedShape *indexShape) {
    const uint32_t L = indexCamera.cullCamera->numLayer();
    const uint32_t K = indexShape->numLODs();
    const uint32_t B = K * L;

    // Compute base offsets for all bins in LOD-major order
    uint32_t runningBase = 0;
    for (uint32_t b = 0; b < B; ++b) {
		indexShape->tmp_binBase_[b] = runningBase;
		runningBase += indexShape->tmp_binCounts_[b];
	}

	indexShape->mapInstanceData_internal();
    auto mapped_ids = indexShape->mappedInstanceIDs();
    auto mapped_counts = indexShape->mappedInstanceCounts();
    auto mapped_base = indexShape->mappedBaseInstance();

	// Copy over the visibility flags from tmp_layerVisibility_ into visible_
    bool isVisible = false;
	for (uint32_t layer = 0; layer < L; ++layer) {
		indexShape->visible_[layer] = indexShape->tmp_layerVisibility_[layer];
		if (indexShape->visible_[layer]) { isVisible = true; }
	}
	indexShape->isVisibleInAnyLayer_ = isVisible;

	// Copy over the counts and base offsets from tmp_ arrays into the mapped arrays
	std::memcpy(mapped_counts, indexShape->tmp_binCounts_.data(), sizeof(uint32_t) * B);
	std::memcpy(mapped_base, indexShape->tmp_binBase_.data(), sizeof(uint32_t) * B);

    // Sort inside each bin and write IDs to mapped buffer
    for (uint32_t l = 0; l < L; ++l) {
    	auto& vec = indexShape->tmp_layerShapes_[l];
    	if (vec.empty()) continue;

		if (indexCamera.sortMode == SortMode::FRONT_TO_BACK) {
			std::ranges::sort(vec, std::ranges::less(), &IndexedShape::ShapeDistance::distance);
		} else if (indexCamera.sortMode == SortMode::BACK_TO_FRONT) {
			std::ranges::sort(vec, std::ranges::greater(), &IndexedShape::ShapeDistance::distance);
		}
		uint32_t layerBase = 0;
		for (uint32_t k = 0; k < K; ++k) {
			const uint32_t b = IndexedShape::binIdx(k, l, L);
			const uint32_t base = indexShape->tmp_binBase_[b];
			const uint32_t cnt = indexShape->tmp_binCounts_[b];
			if (cnt == 0) continue;

			std::transform(
				vec.begin() + layerBase,
				vec.begin() + layerBase + cnt,
				mapped_ids + base,
				[](const IndexedShape::ShapeDistance &sd) { return sd.instanceID; });
			layerBase += cnt;
		}
	}

	indexShape->unmapInstanceData_internal();
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
