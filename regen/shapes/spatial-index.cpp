#include <regen/utility/logging.h>
#include <algorithm>

#include "spatial-index.h"
#include "quad-tree.h"
#include "cull-shape.h"
#include "spatial-index-debug.h"
#include "regen/utility/conversion.h"

using namespace regen;

#define REGEN_FORCE_INLINE __attribute__((always_inline)) inline

namespace regen {
	static constexpr bool SPATIAL_INDEX_USE_MULTITHREADING = true;
}

SpatialIndex::SpatialIndex() {
}

void SpatialIndex::addToIndex(const ref_ptr<BoundingShape> &shape) {
	auto it = nameToShape_.find(shape->name());
	if (it == nameToShape_.end()) {
		auto shapeVector = ref_ptr<std::vector<ref_ptr<BoundingShape>>>::alloc();
		shapeVector->push_back(shape);
		nameToShape_[shape->name()] = shapeVector;
	} else {
		it->second->push_back(shape);
	}
	itemBoundingShapes_.push_back(shape);
	for (auto &ic: indexCameras_) {
		createIndexShape(ic, shape);
	}
}

void SpatialIndex::removeFromIndex(const ref_ptr<BoundingShape> &shape) {
	auto it = nameToShape_.find(shape->name());
	if (it != nameToShape_.end()) {
		auto jt = std::find(it->second->begin(), it->second->end(), shape);
		if (jt != it->second->end()) {
			it->second->erase(jt);
		}
	}
}

void SpatialIndex::addCamera(
		const ref_ptr<Camera> &cullCamera,
		const ref_ptr<Camera> &sortCamera,
		SortMode sortMode,
		const Vec4i &lodShift) {
	cameraToIndexCamera_[cullCamera.get()] = indexCameras_.size();
	auto &data = indexCameras_.emplace_back();
	data.cullCamera = cullCamera;
	data.sortCamera = sortCamera;
	data.sortMode = sortMode;
	data.lodShift = lodShift;
	data.index = this;
	data.camIdx = static_cast<uint32_t>(indexCameras_.size() - 1);
	for (auto &shape : itemBoundingShapes_) {
		createIndexShape(data, shape);
	}
}

void SpatialIndex::createIndexShape(IndexCamera &ic, const ref_ptr<BoundingShape> &shape) {
	auto needle = ic.nameToShape_.find(shape->name());
	if (needle != ic.nameToShape_.end()) {
		// already created
		needle->second->boundingShapes_.push_back(shape);
		// Remember bounding-shape to indexed-shape mapping
		// Note: important to insert this exactly in item-order
		ic.itemToIndexedShape_.push_back(needle->second->shapeIdx_);
		return;
	}
	const uint32_t numLayer = ic.cullCamera->numLayer();
	const uint32_t numInstances = shape->numInstances();
	const uint32_t numIndices = numInstances * numLayer;

	auto is = ref_ptr<IndexedShape>::alloc(ic.cullCamera, ic.sortCamera, ic.lodShift, shape);
	const uint32_t numLOD = std::max(1u, is->numLODs());
	is->idVec_ = ref_ptr<ShaderInput1ui>::alloc("instanceIDs", 1);
	is->idVec_->setInstanceData(numIndices, 1, nullptr);
	is->countVec_ = ref_ptr<ShaderInput1ui>::alloc("instanceCounts", numLayer * numLOD);
	is->countVec_->setInstanceData(1, 1, nullptr);
	is->baseVec_ = ref_ptr<ShaderInput1ui>::alloc("baseInstances", numLayer * numLOD);
	is->baseVec_->setInstanceData(1, 1, nullptr);

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

	is->shapeIdx_ = static_cast<uint16_t>(ic.indexShapes_.size());
	ic.nameToShape_[shape->name()] = is;
	ic.indexShapes_.push_back(is.get());
	ic.itemToIndexedShape_.push_back(is->shapeIdx_);
	is->boundingShapes_.push_back(shape);
	// mark camera as dirty as the camera buffers must be resized.
	// We do this lazy to avoid multiple resizes when adding many shapes.
	ic.isDirty = true;
}

bool SpatialIndex::hasCamera(const Camera &camera) const {
	return cameraToIndexCamera_.find(&camera) != cameraToIndexCamera_.end();
}

std::vector<const Camera *> SpatialIndex::cameras() const {
	std::vector<const Camera *> result;
	for (auto &ic: indexCameras_) {
		result.push_back(ic.cullCamera.get());
	}
	return result;
}

ref_ptr<IndexedShape> SpatialIndex::getIndexedShape(const ref_ptr<Camera> &camera, std::string_view shapeName) {
	auto it = cameraToIndexCamera_.find(camera.get());
	if (it == cameraToIndexCamera_.end()) {
		return {};
	}
	return indexCameras_[it->second].nameToShape_[shapeName];
}

ref_ptr<std::vector<ref_ptr<BoundingShape>>> SpatialIndex::getShapes(std::string_view shapeName) const {
	auto it = nameToShape_.find(shapeName);
	if (it != nameToShape_.end()) {
		return it->second;
	}
	return {};
}

bool SpatialIndex::isVisible(const Camera &camera, uint32_t layerIdx, std::string_view shapeID) {
	auto it = cameraToIndexCamera_.find(&camera);
	if (it == cameraToIndexCamera_.end()) {
		return true;
	}
	const IndexCamera &ic = indexCameras_[it->second];
	auto it2 = ic.nameToShape_.find(shapeID);
	if (it2 == ic.nameToShape_.end()) {
		return true;
	}
	return it2->second->isVisibleInLayer(layerIdx);
}

uint32_t SpatialIndex::numInstances(std::string_view shapeID) const {
	auto shape = getShape(shapeID);
	if (!shape.get()) {
		return 0u;
	}
	return shape->numInstances();
}

ref_ptr<BoundingShape> SpatialIndex::getShape(std::string_view shapeID) const {
	auto it = nameToShape_.find(shapeID);
	if (it != nameToShape_.end() && !it->second->empty()) {
		return it->second->front();
	}
	return {};
}

ref_ptr<BoundingShape> SpatialIndex::getShape(std::string_view shapeID, uint32_t instance) const {
	auto it = nameToShape_.find(shapeID);
	if (it != nameToShape_.end() && !it->second->empty()) {
		for (auto &shape: *it->second.get()) {
			if (instance == shape->instanceID()) {
				return shape;
			}
		}
	}
	return {};
}

namespace regen {
	struct VisibleHitSOA {
		VisibleHitSOA() = default;
		~VisibleHitSOA() = default;
		VisibleHitSOA(const VisibleHitSOA&) = delete;
		VisibleHitSOA& operator=(const VisibleHitSOA&) = delete;

		HitBuffer *hits = nullptr;
		AlignedArray<float> ox, oy, oz;   // origins
		AlignedArray<float> t0, t1, t2;   // lod thresholds
		AlignedArray<uint32_t> globalID;
		AlignedArray<uint32_t> shapeIdx;
		// output arrays
		AlignedArray<float> lodDistance;
		AlignedArray<uint32_t> binIdx;

		void resize(uint32_t size) {
			if (ox.size() < size) {
				uint32_t nextSize = size + (size / 2u) + 256u; // grow by 50% + 256
				ox.resize(nextSize);
				oy.resize(nextSize);
				oz.resize(nextSize);
				t0.resize(nextSize);
				t1.resize(nextSize);
				t2.resize(nextSize);
				globalID.resize(nextSize);
				shapeIdx.resize(nextSize);
				lodDistance.resize(nextSize);
				binIdx.resize(nextSize);
			}
		}
	};
}

static void gatherVisibleHitSOA(
			IndexCamera &ic,
			VisibleHitSOA &hitSoA,
			uint32_t layerIdx,
			const ref_ptr<BoundingShape> *itemShapes) {
	const std::vector<uint32_t> &globalInstanceIds = ic.globalInstanceIDs_[layerIdx];
	const HitBuffer &hits = *hitSoA.hits;

	for (uint32_t hitIdx=0; hitIdx < hits.count; ++hitIdx) {
		// Gather data for this shape
		const uint32_t itemIdx  = hits.data[hitIdx];
		const uint32_t shapeIdx = ic.itemToIndexedShape_[itemIdx];
		const Vec3f &shapeOrigin   = itemShapes[itemIdx]->tfOrigin();
		const Vec3f &lodThresholds = ic.indexShapes_[shapeIdx]->lodThresholds();
		// Store in SOA
		hitSoA.ox[hitIdx] = shapeOrigin.x;
		hitSoA.oy[hitIdx] = shapeOrigin.y;;
		hitSoA.oz[hitIdx] = shapeOrigin.z;
		hitSoA.t0[hitIdx] = lodThresholds.x;
		hitSoA.t1[hitIdx] = lodThresholds.y;
		hitSoA.t2[hitIdx] = lodThresholds.z;
		hitSoA.globalID[hitIdx] = globalInstanceIds[itemIdx];
		hitSoA.shapeIdx[hitIdx] = shapeIdx;
	}
}

template<SpatialIndex::DistanceKeySize DistanceType>
static uint32_t computeDistanceKey(float lodDistance, const uint32_t flip) {
	//const uint32_t xf = conversion::floatBitsToUint(lodDistance);
	const uint32_t xf = std::bit_cast<uint32_t>(lodDistance);
	if constexpr (DistanceType == SpatialIndex::DISTANCE_KEY_16) {
		const uint16_t q = ((xf>>16)&0x8000)               // sign bit
			| ((((xf&0x7f800000)-0x38000000)>>13)&0x7c00)  // exponent bits
			| ((xf>>13)&0x03ff);                           // mantissa bits
		return (q ^ (flip & 0xFFFF));
	} else if constexpr (DistanceType == SpatialIndex::DISTANCE_KEY_24) {
		const uint32_t q = ((xf >> 8) & 0x800000)                   // sign bit
			| ((((xf & 0x7f800000) - 0x3f800000) >> 7) & 0x7f0000)  // exponent bits
			| ((xf >> 8) & 0x00ffff);                               // mantissa bits
		return (q ^ (flip & 0xFFFFFF));
	} else {
		return (xf ^ flip);
	}
}

template <typename KeyType, SpatialIndex::DistanceKeySize DistanceType, bool UseSoA>
static void pushVisibleShapes(IndexCamera &ic, uint32_t layerIdx, HitBuffer &hitBuffer,
		const ref_ptr<BoundingShape> *itemShapes) {
	static constexpr int SortKeyBits = (sizeof(KeyType) * 8);
	static constexpr uint8_t bitOffset_layer = static_cast<uint8_t>(DistanceType);

	const uint32_t numLayer = ic.cullCamera->numLayer();
	const uint32_t numHits = hitBuffer.count;
	const uint32_t flip = -(ic.sortMode == BACK_TO_FRONT);
	const uint8_t bitOffset_shape = bitOffset_layer + ic.layerBits;

	const Vec3f *camPosPtr;
	if (ic.sortCamera->position().size()>1) {
		camPosPtr = &ic.sortCamera->position(layerIdx);
	} else {
		camPosPtr = &ic.sortCamera->position(0);
	}
	const Vec3f &camPos = *camPosPtr;

	// Compute sort keys, a key is composed of:
	//		[ shapeIdx | layerIdx | distance ]
	// where the number of bits for each field is determined by:
	//		shapeIdx: determined by number of shapes in the camera (shapeBits)
	//		layerIdx: determined by number of layers in the camera (layerBits)
	//		distance: remaining bits (distanceBits)
	KeyType *sortKeys;
	if constexpr (SortKeyBits == 32) {
		sortKeys = ic.tmp_sortKeys32_.data();
	} else {
		sortKeys = ic.tmp_sortKeys64_.data();
	}

	if constexpr (UseSoA) {
		thread_local VisibleHitSOA hitSoA;
		uint32_t hitIdx = 0;
		hitSoA.hits = &hitBuffer;
		hitSoA.resize(hitBuffer.count);

		const float *d_ox = hitSoA.ox.data();
		const float *d_oy = hitSoA.oy.data();
		const float *d_oz = hitSoA.oz.data();
		const float *d_t0 = hitSoA.t0.data();
		const float *d_t1 = hitSoA.t1.data();
		const float *d_t2 = hitSoA.t2.data();

		float *d_lodDistance = hitSoA.lodDistance.data();
		uint32_t *d_binIdx = hitSoA.binIdx.data();

		// First pass: gather all data into SOA arrays
		gatherVisibleHitSOA(ic, hitSoA, layerIdx, itemShapes);

		// Second pass: process all hits, write output: lodDistance & binIdx
		{
			const BatchOf_float cx = BatchOf_float::fromScalar(camPos.x);
			const BatchOf_float cy = BatchOf_float::fromScalar(camPos.y);
			const BatchOf_float cz = BatchOf_float::fromScalar(camPos.z);
			const BatchOf_int32 nl = BatchOf_int32::fromScalar(static_cast<int32_t>(numLayer));
			const BatchOf_int32 layer = BatchOf_int32::fromScalar(static_cast<int32_t>(layerIdx));
			const BatchOf_int32 one = BatchOf_int32::fromScalar(1);

			for (; hitIdx + simd::RegisterWidth <= numHits; hitIdx += simd::RegisterWidth) {
				// delta = shape-pos - cam-pos
				const BatchOf_float dx = BatchOf_float::loadAligned(d_ox + hitIdx) - cx;
				const BatchOf_float dy = BatchOf_float::loadAligned(d_oy + hitIdx) - cy;
				const BatchOf_float dz = BatchOf_float::loadAligned(d_oz + hitIdx) - cz;
				const BatchOf_float lodDistance = (dx*dx) + (dy*dy) + (dz*dz);

				// comparisons -> mask (true -> 0xFFFFFFFF)
				const BatchOf_float c0 = (lodDistance >= BatchOf_float::loadAligned(d_t0 + hitIdx));
				const BatchOf_float c1 = (lodDistance >= BatchOf_float::loadAligned(d_t1 + hitIdx));
				const BatchOf_float c2 = (lodDistance >= BatchOf_float::loadAligned(d_t2 + hitIdx));

				// convert masks to integer 0/1 per lane, and sum to get LOD level
				const BatchOf_int32 lodLevel =
					(BatchOf_int32::castFloatBatch(c0) & one) +
					(BatchOf_int32::castFloatBatch(c1) & one) +
					(BatchOf_int32::castFloatBatch(c2) & one);

				// compute binIdx = lodLevel * numLayer + layerIdx
				const BatchOf_int32 binIdx = (lodLevel * nl) + layer;

				// Finally store results
				binIdx.storeAligned(d_binIdx + hitIdx);
				lodDistance.storeAligned(d_lodDistance + hitIdx);
			}

			// process remaining hits scalar-wise
			for (; hitIdx < numHits; ++hitIdx) {
				const float dx = d_ox[hitIdx] - camPos.x;
				const float dy = d_oy[hitIdx] - camPos.y;
				const float dz = d_oz[hitIdx] - camPos.z;
				// Compute squared distance from shape to camera
				const float lodDistance = (dx * dx) + (dy * dy) + (dz * dz);
				// Compute LOD level for this shape by distance to camera.
				const uint32_t lodLevel =
					(lodDistance >= d_t0[hitIdx]) +
					(lodDistance >= d_t1[hitIdx]) +
					(lodDistance >= d_t2[hitIdx]);
				// compute bin and item indices
				d_lodDistance[hitIdx] = lodDistance;
				d_binIdx[hitIdx] = lodLevel * numLayer + layerIdx;
			}
		}

		// Third pass: compute sort keys and scatter
		for (hitIdx=0; hitIdx < numHits; ++hitIdx) {
			const uint32_t globalID = hitSoA.globalID[hitIdx];
			const uint32_t shapeIdx = hitSoA.shapeIdx[hitIdx];
			const float lodDistance = d_lodDistance[hitIdx];
			const uint32_t binIdx = d_binIdx[hitIdx];

			// Compute sort key for this shape.
			const KeyType sortKey =
				// pack distance (lower bits)
				computeDistanceKey<DistanceType>(lodDistance, flip) |
				// pack layer
				((static_cast<KeyType>(layerIdx) & ic.layerMask) << bitOffset_layer) |
				// pack shape (upper bits)
				((static_cast<KeyType>(shapeIdx) & ic.shapeMask) << bitOffset_shape);

			// Add sort key for this instance
			sortKeys[globalID] = sortKey;
			// Store instance ID for this instance
			ic.tmp_globalIDQueue_.push_back(globalID);
			// Scatter into per indexed shape arrays
			IndexedShape &is = *ic.indexShapes_[shapeIdx];
			is.addVisibleInstance(layerIdx, binIdx);
		}
	} else {
		const uint32_t* globalInstanceIDs = ic.globalInstanceIDs_[layerIdx].data();

		for (uint32_t hitIdx=0; hitIdx < hitBuffer.count; ++hitIdx) {
			// Gather shape data for this hit.
			const uint32_t itemIdx = hitBuffer.data[hitIdx];
			const uint32_t globalID = globalInstanceIDs[itemIdx];
			const uint32_t shapeIdx = ic.itemToIndexedShape_[itemIdx];

			BoundingShape &bs = *itemShapes[itemIdx].get();
			IndexedShape &is = *ic.indexShapes_[shapeIdx];
			const Vec3f &shapeOrigin = bs.tfOrigin();
			const Vec3f &lodThresholds = is.lodThresholds();

			// Compute squared distance from shape to camera
			const float lodDistance = (shapeOrigin - camPos).lengthSquared();

			// Compute LOD level for this shape by distance to camera.
			const uint32_t lodLevel =
				(lodDistance >= lodThresholds.x) +
				(lodDistance >= lodThresholds.y) +
				(lodDistance >= lodThresholds.z);
			// compute bin and item indices
			const uint32_t binIdx = lodLevel * numLayer + layerIdx;

			// Compute sort key for this shape.
			KeyType sortKey =
				// pack distance (lower bits)
				computeDistanceKey<DistanceType>(lodDistance, flip) |
				// pack layer
				((static_cast<KeyType>(layerIdx) & ic.layerMask) << bitOffset_layer) |
				// pack shape (upper bits)
				((static_cast<KeyType>(shapeIdx) & ic.shapeMask) << bitOffset_shape);

			// Add sort key for this instance
			sortKeys[globalID] = sortKey;
			// Store instance ID for this instance
			ic.tmp_globalIDQueue_.push_back(globalID);
			// Scatter into per indexed shape arrays
			is.addVisibleInstance(layerIdx, binIdx);
		}
	}
}

void SpatialIndex::updateLayerVisibility(
		IndexCamera &ic, uint32_t layerIdx,
		const BoundingShape &camera_shape) {
	static constexpr bool UseSoA = true;
	// Collect all intersections for this layer into (lod,layer) bins
	auto &hitBuffer = foreachIntersection(camera_shape, ic.traversalMask);

	#define _push(KT,DT) pushVisibleShapes<KT,DT,UseSoA>(ic, layerIdx, hitBuffer, itemBoundingShapes_.data())
	if (ic.keyBits <= 32) {
		using KeyType = uint32_t;
		if (ic.index->distanceBits_ == DISTANCE_KEY_16) {
			_push(KeyType, DISTANCE_KEY_16);
		} else {
			// note: for 32-bit keys we can only have 24-bit distance
			_push(KeyType, DISTANCE_KEY_24);
		}
	} else {
		using KeyType = uint64_t;
		if (ic.index->distanceBits_ == DISTANCE_KEY_16) {
			_push(KeyType, DISTANCE_KEY_16);
		} else if (ic.index->distanceBits_ == DISTANCE_KEY_24) {
			_push(KeyType, DISTANCE_KEY_24);
		} else {
			_push(KeyType, DISTANCE_KEY_32);
		}
	}
	#undef _push
}

static void visibilityJobFunc(void *arg) {
	VisibilityJob::run(arg);
}

REGEN_FORCE_INLINE uint8_t getMinBits(uint32_t numValues) {
	if (numValues <= 1) return 1;
	// ceil(log2(numValues))
	return 32 - __builtin_clz(numValues - 1);
}

template <typename RadixType, typename KeyType>
static void radixSortFun(void *radixSort, void *sortKeys, std::vector<uint32_t> &values) {
	auto *keys = static_cast<std::vector<KeyType> *>(sortKeys);
	static_cast<RadixType*>(radixSort)->sort(values, *keys);
}

template <typename KeyType>
static void smallSortFun(void*, void *sortKeys, std::vector<uint32_t> &values) {
	auto &keys = *static_cast<std::vector<KeyType>*>(sortKeys);
	std::sort(values.begin(), values.end(),
		[&keys](uint32_t a, uint32_t b) { return keys[a] < keys[b]; });
}

void SpatialIndex::resetCamera(IndexCamera *indexCamera, DistanceKeySize distanceBits, uint32_t traversalMask) {
	const uint32_t numLayer = indexCamera->cullCamera->numLayer();

	if (indexCamera->isDirty) {
		// Compute the number of keys which is the sum of L * I for all shapes
		// assigned to this camera.
		indexCamera->numKeys = 0;

		uint32_t shapeBase = 0;
		for (auto &indexShape: indexCamera->indexShapes_) {
			const uint32_t numInstances = indexShape->shape().numInstances();
			indexCamera->numKeys += numInstances * numLayer;
			indexShape->globalBase_ = shapeBase;
			shapeBase += indexShape->shape().numInstances() * numLayer;
		}
		indexCamera->tmp_globalIDQueue_.reserve(indexCamera->numKeys);
		indexCamera->globalToInstanceIdx_.resize(indexCamera->numKeys, 0);

		// resize indexCamera->globalInstanceIDs_ arrays
		indexCamera->globalInstanceIDs_.resize(numLayer);
		for (auto &layerVec: indexCamera->globalInstanceIDs_) {
			layerVec.resize(indexCamera->numKeys, 0);
		}

		// Compute the global index for all shapes assigned to this camera.
		for (uint32_t itemIdx=0; itemIdx<itemBoundingShapes_.size(); ++itemIdx) {
			BoundingShape &bs = *itemBoundingShapes_[itemIdx].get();
			IndexedShape &is = *indexCamera->indexShapes_[indexCamera->itemToIndexedShape_[itemIdx]];
			const uint32_t numInstances = bs.numInstances();
			const uint32_t instanceID = bs.instanceID();
			for (uint32_t layerIdx = 0; layerIdx < numLayer; ++layerIdx) {
				const uint32_t globalIdx = is.globalBase_ +
					layerIdx * numInstances + // layer base
					instanceID;
				indexCamera->globalInstanceIDs_[layerIdx][itemIdx] = globalIdx;
				indexCamera->globalToInstanceIdx_[globalIdx] = instanceID;
			}
		}

		indexCamera->layerBits = getMinBits(numLayer);
		indexCamera->layerMask = (1u << indexCamera->layerBits) - 1u;
		indexCamera->shapeBits = getMinBits(indexCamera->indexShapes_.size());
		indexCamera->shapeMask = (1u << indexCamera->shapeBits) - 1u;
		indexCamera->keyBits = indexCamera->layerBits + indexCamera->shapeBits + static_cast<uint8_t>(distanceBits);

		if (indexCamera->keyBits <= 32) {
			using KeyType = uint32_t;
			indexCamera->tmp_sortKeys32_.resize(indexCamera->numKeys);
			indexCamera->sortKeys = static_cast<void*>(&indexCamera->tmp_sortKeys32_);
			indexCamera->sortFun[IndexCamera::SORT_FUN_SMALL] = smallSortFun<KeyType>;
			if (indexCamera->keyBits <= 24) {
				// use 24-bit radix sort (uint32_t keys)
				using RadixType = RadixSort_CPU_seq<uint32_t, KeyType, 8, 24>;
				indexCamera->radixSort = IndexCamera::createRadix<RadixType>(indexCamera->numKeys);
				indexCamera->sortFun[IndexCamera::SORT_FUN_LARGE] = radixSortFun<RadixType,KeyType>;
			} else {
				// use 32-bit radix sort (uint32_t keys)
				using RadixType = RadixSort_CPU_seq<uint32_t, KeyType, 8, 32>;
				indexCamera->radixSort = IndexCamera::createRadix<RadixType>(indexCamera->numKeys);
				indexCamera->sortFun[IndexCamera::SORT_FUN_LARGE] = radixSortFun<RadixType,KeyType>;
			}
		} else {
			using KeyType = uint64_t;
			indexCamera->tmp_sortKeys64_.resize(indexCamera->numKeys);
			indexCamera->sortKeys = static_cast<void*>(&indexCamera->tmp_sortKeys64_);
			indexCamera->sortFun[IndexCamera::SORT_FUN_SMALL] = smallSortFun<KeyType>;
			if (indexCamera->keyBits <= 40) {
				// use 40-bit radix sort (uint64_t keys)
				using RadixType = RadixSort_CPU_seq<uint32_t, KeyType, 8, 40>;
				indexCamera->radixSort = IndexCamera::createRadix<RadixType>(indexCamera->numKeys);
				indexCamera->sortFun[IndexCamera::SORT_FUN_LARGE] = radixSortFun<RadixType,KeyType>;
			} else if (indexCamera->keyBits <= 48) {
				// use 48-bit radix sort (uint64_t keys)
				using RadixType = RadixSort_CPU_seq<uint32_t, KeyType, 8, 48>;
				indexCamera->radixSort = IndexCamera::createRadix<RadixType>(indexCamera->numKeys);
				indexCamera->sortFun[IndexCamera::SORT_FUN_LARGE] = radixSortFun<RadixType,KeyType>;
			} else {
				// use 64-bit radix sort (uint64_t keys)
				using RadixType = RadixSort_CPU_seq<uint32_t, KeyType, 8, 64>;
				indexCamera->radixSort = IndexCamera::createRadix<RadixType>(indexCamera->numKeys);
				indexCamera->sortFun[IndexCamera::SORT_FUN_LARGE] = radixSortFun<RadixType,KeyType>;
			}
		}
		indexCamera->isDirty = false;
	}
	indexCamera->tmp_globalIDQueue_.clear();
	indexCamera->traversalMask = traversalMask;
	auto &frustumShapes = indexCamera->cullCamera->frustum();
	for (auto &shape: frustumShapes) {
		shape.updateOrthogonalProjection();
	}
}

void SpatialIndex::updateVisibility(uint32_t traversalMask) {
	if constexpr (SPATIAL_INDEX_USE_MULTITHREADING) {
		uint32_t numIndexedCameras = indexCameras_.size();
		if (!jobPool_) {
			uint32_t numThreads = numIndexedCameras - 1; // leave one for the local thread
			numThreads = std::max(1u, std::min(numThreads, maxNumThreads_));
			jobPool_ = std::make_unique<JobPool>(numThreads);
		}

		// Schedule jobs for all index cameras, but keep the one with the most keys for the local thread.
		IndexCamera *localIndexCamera = &indexCameras_.front();
		resetCamera(localIndexCamera, distanceBits_, traversalMask);
		for (uint32_t i = 1; i < numIndexedCameras; ++i) {
			IndexCamera *nextIndexCamera = &indexCameras_[i];
			resetCamera(nextIndexCamera, distanceBits_, traversalMask);
			if (nextIndexCamera->numKeys > localIndexCamera->numKeys) {
				std::swap(localIndexCamera, nextIndexCamera);
			}
			jobPool_->addJobPreFrame(Job{ .fn = visibilityJobFunc, .arg = nextIndexCamera });
		}

		// Execute jobs
		jobPool_->beginFrame(1u); // one local job
		Job localJob { .fn = visibilityJobFunc, .arg = localIndexCamera };
		do {
			jobPool_->performJob(localJob);
		} while (jobPool_->stealJob(localJob));
		jobPool_->endFrame();
	} else { // no multithreading
		for (auto &indexCamera: indexCameras_) {
			resetCamera(&indexCamera, distanceBits_, traversalMask);
			updateVisibility(&indexCamera);
		}
	}
}

void SpatialIndex::updateVisibility(IndexCamera *indexCamera) {
	const uint32_t L = indexCamera->cullCamera->numLayer();

	for (auto &indexShape: indexCamera->indexShapes_) {
		// Reset visibility + total count
		indexShape->tmp_layerVisibility_.assign(L, false);
		indexShape->tmp_totalCount_ = 0;
		// Map instance data for this shape, and reset the bin counts
		indexShape->mapInstanceData_internal();
		indexShape->tmp_binBase_ = indexShape->mappedBaseInstance();
		indexShape->tmp_binCounts_ = indexShape->mappedInstanceCounts();
		// Reset the per-bin counts. We accumulate them during traversal, so they need to be reset first.
		std::memset(indexShape->tmp_binCounts_, 0,
			sizeof(uint32_t) * indexShape->numLODs() * L);
	}

	// Process each layer
	auto &frustumShapes = indexCamera->cullCamera->frustum();
	for (uint32_t layerIdx = 0; layerIdx < frustumShapes.size(); ++layerIdx) {
		updateLayerVisibility(*indexCamera, layerIdx, frustumShapes[layerIdx]);
	}

	// Sort the visible instances according to their sort keys
	std::vector<uint32_t>& queuedGlobalIDs = indexCamera->tmp_globalIDQueue_;
	indexCamera->sortFun[static_cast<int>(queuedGlobalIDs.size()<SMALL_ARRAY_SIZE)]
		(indexCamera->radixSort.get(), indexCamera->sortKeys, queuedGlobalIDs);

	uint32_t shapeBase = 0;
	for (auto &indexShape: indexCamera->indexShapes_) {
		if (indexShape->tmp_totalCount_ == 0) {
			// No visible instances for this shape
			// Mark all layers as invisible
			indexShape->visible_.assign(L, false);
			indexShape->isVisibleInAnyLayer_ = false;
			indexShape->unmapInstanceData_internal();
			continue;
		}

	    // Compute base offsets for all bins in LOD-major order
		const uint32_t numBins = indexShape->numLODs() * L;
	    uint32_t runningBase = 0;
	    for (uint32_t b = 0; b < numBins; ++b) {
			indexShape->tmp_binBase_[b] = runningBase;
			runningBase += indexShape->tmp_binCounts_[b];
		}

		// Copy over the visibility flags from tmp_layerVisibility_ into visible_
	    bool isVisible = false;
		for (uint32_t layer = 0; layer < L; ++layer) {
			bool visibleInLayer = indexShape->tmp_layerVisibility_[layer];
			indexShape->visible_[layer] = visibleInLayer;
			if (!isVisible && visibleInLayer) { isVisible = true; }
		}
		indexShape->isVisibleInAnyLayer_ = isVisible;

	    // Write IDs to mapped buffer
		// Each shape has a fixed contiguous region in tmp_layerShapes_ starting at some offset.
		auto mapped_ids = indexShape->mappedInstanceIDs();
		std::vector<uint32_t>::iterator vecBegin = queuedGlobalIDs.begin() + shapeBase;
		std::vector<uint32_t>::iterator vecEnd = vecBegin + indexShape->tmp_totalCount_;
		const auto &globalToLocalID = indexCamera->globalToInstanceIdx_;
		std::transform(vecBegin, vecEnd, mapped_ids,
			[&globalToLocalID](uint32_t globalIdx) {
				return globalToLocalID[globalIdx];
			});

		indexShape->unmapInstanceData_internal();
		indexShape->tmp_binBase_ = nullptr;
		indexShape->tmp_binCounts_ = nullptr;
		shapeBase += indexShape->tmp_totalCount_;
	}
}

void SpatialIndex::addDebugShape(const ref_ptr<BoundingShape> &shape) {
	debugShapes_.push_back(shape);
}

void SpatialIndex::removeDebugShape(const ref_ptr<BoundingShape> &shape) {
	auto it = std::find(debugShapes_.begin(), debugShapes_.end(), shape);
	if (it != debugShapes_.end()) {
		debugShapes_.erase(it);
	}
}

void SpatialIndex::debugBoundingShape(DebugInterface &debug, const BoundingShape &shape) {
	SpatialIndexDebug &sid = static_cast<SpatialIndexDebug &>(debug);
	switch (shape.shapeType()) {
		case BoundingShapeType::SPHERE: {
			auto sphere = static_cast<const BoundingSphere *>(&shape);
			sid.drawSphere(*sphere);
			break;
		}
		case BoundingShapeType::AABB: {
			auto box = static_cast<const AABB *>(&shape);
			sid.drawBox(*box);
			break;
		}
		case BoundingShapeType::OBB: {
			auto box = static_cast<const OBB *>(&shape);
			sid.drawBox(*box);
			break;
		}
		case BoundingShapeType::FRUSTUM: {
			auto frustum = static_cast<const Frustum *>(&shape);
			sid.drawFrustum(*frustum);
			break;
		}
		case BoundingShapeType::LAST:
			break;
	}
}

void SpatialIndex::debugDraw(DebugInterface &debug) const {
	SpatialIndexDebug &sid = static_cast<SpatialIndexDebug &>(debug);
	for (auto &shape: shapes()) {
		for (auto &instance: *shape.second.get()) {
			if (instance->traversalMask() == 0) continue;
			debugBoundingShape(debug, *instance.get());
		}
	}
	for (auto &shape : debugShapes_) {
		if (shape->traversalMask() == 0) continue;
		debugBoundingShape(debug, *shape.get());
	}
	for (auto &camera: cameras()) {
		for (auto &frustum: camera->frustum()) {
			sid.debugFrustum(frustum, Vec3f(1.0f, 0.0f, 1.0f));
		}
	}
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
		if (input.hasAttribute("batch-size-3d")) {
			quadTree->setBatchSize3D(input.getValue<GLuint>("batch-size-3d", 2048u));
		}
		if (input.hasAttribute("close-distance")) {
			auto dst = input.getValue<float>("close-distance", 20.0f);
			dst = std::max(0.0f, dst);
			quadTree->setCloseDistanceSquared(dst * dst);
		}
		if (input.hasAttribute("subdivision-threshold")) {
			quadTree->setSubdivisionThreshold(input.getValue<GLuint>("subdivision-threshold", 4u));
		}

		index = quadTree;
	}

	if (input.hasAttribute("distance-bits")) {
		auto distanceBits = input.getValue<int>("distance-bits", 24);
		if (distanceBits == 16) {
			index->setDistanceBits(DISTANCE_KEY_16);
		} else if (distanceBits == 24) {
			index->setDistanceBits(DISTANCE_KEY_24);
		} else if (distanceBits == 32) {
			index->setDistanceBits(DISTANCE_KEY_32);
		} else {
			REGEN_WARN("Invalid distance-bits value " << distanceBits << ", using default 24 bits.");
			index->setDistanceBits(DISTANCE_KEY_24);
		}
	}
	if (input.hasAttribute("max-threads")) {
		auto maxThreads = input.getValue<int>("max-threads", 4);
		maxThreads = std::max(1, maxThreads);
		index->setMaxNumThreads(static_cast<uint8_t>(maxThreads));
	}

	return index;
}
