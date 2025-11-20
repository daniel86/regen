#ifndef REGEN_SPATIAL_INDEX_H_
#define REGEN_SPATIAL_INDEX_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/indexed-shape.h>
#include <regen/shapes/hit-buffer.h>
#include <regen/camera/camera.h>
#include "regen/utility/debug-interface.h"
#include "regen/utility/radix-sort-cpu.h"
#include <regen/scene/loading-context.h>

#include "regen/utility/aligned-array.h"

namespace regen {
	/**
	 * Callback structure for intersection tests.
	 */
	struct IntersectionCallback {
		// Function pointer for the callback
		void (*fun)(const BoundingShape &, void *) = nullptr;
		// User data pointer passed to the callback
		void *userData = nullptr;
	};

	class SpatialIndex;

	/**
	 * @brief Index camera data
	 */
	struct IndexCamera {
		// We toggle between different sort modes depending on array size:
		enum SortFunType { SORT_FUN_LARGE = 0, SORT_FUN_SMALL = 1 };
		// Key arrays are passed as void* as we do support different types of key data.
		// The signature of sort functions is:
		using SortFun = void (*)(void*, void*, std::vector<uint32_t>&);

		// Back-reference to the spatial index
		SpatialIndex *index = nullptr;
		// The culling and sorting cameras
		ref_ptr<Camera> cullCamera;
		ref_ptr<Camera> lodCamera;
		bool hasLODCam = false;

		// LOD shift for each layer
		Vec4i lodShift = Vec4i::zero();
		// How to sort instances by distance to camera
		SortMode sortMode = SortMode::FRONT_TO_BACK;
		// the bitmask to filter shapes during traversal
		uint32_t traversalMask = 0;

		// The indexed shapes assigned to this camera.
		// Note: these are unique shapes, instances are not counted here.
		std::vector<IndexedShape*> indexShapes_; // size = numShapes
		std::unordered_map<std::string_view, ref_ptr<IndexedShape>> nameToShape_;
		// Mark camera as dirty when shapes are added/removed
		bool isDirty = false;
		// Index of this camera in the camera list of the spatial index,
		// i.e. index->indexCameras_[camIdx] == this
		uint32_t camIdx = 0;
		// Total number of keys = sum_{shape} numLayer * numInstances_{shape}
		uint32_t numKeys = 0;

		// Sort keys here are composed of:
		//		[ shapeIdx | layerIdx | distance ]
		// Below we have some attributes defining the bit sizes and masks for each field.
		uint8_t layerBits = 0u;
		uint8_t shapeBits = 0u;
		uint8_t keyBits = 0u;
		uint32_t layerMask = 0u;
		uint32_t shapeMask = 0u;

		// This is a stable vector mapping item indices to indexed shape indices,
		// i.e. itemToIndexedShape_[itemIdx] -> indexedShapeIdx
		std::vector<uint32_t> itemToIndexedShape_; // size = numItems

		// Temporary array filled up each frame with "global IDs" during traversal
		// The notion of global IDs is used to identify instances across layers and shapes.
		// Each shape instance gets a unique global ID in the range [0, numKeys) for each layer.
		std::vector<uint32_t> globalQueue_; // dynamic
		// When writing to the instance output arrays, we need to map from global ID to instance ID.
		std::vector<uint32_t> globalToInstance_;  // size = numKeys
		// Maps from item index to global IDs for each layer individually.
		std::vector<std::vector<uint32_t>> itemToGlobalID_;  // size = numKeys

		// Key arrays for sorting, only one is used at a time depending on key size:
		// 64-bit keys
		std::vector<uint64_t> sortKeys64_; // size = numKeys
		// 32-bit keys
		std::vector<uint32_t> sortKeys32_; // size = numKeys
		// Pointer to the current key array in use, which is either tmp_sortKeys32_ or tmp_sortKeys64_.
		void *sortKeys = nullptr; // note: erased type

		// radix sort for sorting the instances
		std::unique_ptr<void, void(*)(void*)> radixSort =
			createRadix<RadixSort_CPU_seq<uint32_t, uint32_t, 8, 32>>(10);
		// sorting functions, one for large arrays, one for small arrays
		SortFun sortFun[2] = { nullptr, nullptr };

		// Helper function to create radix sorters
		template <typename RadixType>
		static std::unique_ptr<void, void(*)(void*)> createRadix(size_t numKeys) {
			return std::unique_ptr<void, void(*)(void*)>(
				new RadixType(numKeys),
				[](void* ptr) { delete static_cast<RadixType*>(ptr); });
		}
	};

	/**
	 * @brief Spatial index
	 */
	class SpatialIndex : public Resource {
	public:
		static constexpr const char *TYPE_NAME = "SpatialIndex";
		// Threshold for using standard sort (small arrays) vs radix sort (large arrays)
		static constexpr uint32_t SMALL_ARRAY_SIZE = 256;
		// The bit sizes for distance in sort key
		enum DistanceKeySize {
			DISTANCE_KEY_16 = 16,
			DISTANCE_KEY_24 = 24,
			DISTANCE_KEY_32 = 32
		};

		SpatialIndex();

		~SpatialIndex() override;

		static ref_ptr<SpatialIndex> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @brief Set the maximum number of threads to use
		 * @param maxNumThreads The maximum number of threads
		 */
		void setMaxNumThreads(uint8_t maxNumThreads) { maxNumThreads_ = maxNumThreads; }

		/**
		 * @brief Set the number of bits to use for distance in sort key
		 * @param distanceBits The number of bits
		 */
		void setDistanceBits(DistanceKeySize distanceBits) { distanceBits_ = distanceBits; }

		/**
		 * @brief Get the number of bits used for distance in sort key
		 * @return The number of bits
		 */
		DistanceKeySize distanceBits() const { return distanceBits_; }

		/**
		 * @brief Get the indexed shape for a camera
		 * @param camera The camera
		 * @param shapeName The shape name
		 * @return The indexed shape
		 */
		ref_ptr<IndexedShape> getIndexedShape(const ref_ptr<Camera> &camera, std::string_view shapeName);

		/**
		 * @brief Get all shapes with a given name
		 * @param shapeName The shape name
		 * @return The shapes
		 */
		ref_ptr<std::vector<ref_ptr<BoundingShape>>> getShapes(std::string_view shapeName) const;

		/**
		 * @brief Add a camera to the index
		 * @param cullCamera The culling camera
		 * @param lodCamera The camera used to compute LOD level
		 * @param sortMode The sort mode
		 * @param lodShift The LOD shift
		 */
		void addCamera(
				const ref_ptr<Camera> &cullCamera,
				const ref_ptr<Camera> &lodCamera,
				SortMode sortMode,
				const Vec4i &lodShift);

		/**
		 * @brief Check if the index has a camera
		 * @param camera The camera
		 * @return True if the index has the camera, false otherwise
		 */
		bool hasCamera(const Camera &camera) const;

		/**
		 * @brief Get the number of instances of a shape
		 * @param shapeID The shape ID
		 * @return The number of instances
		 */
		uint32_t numInstances(std::string_view shapeID) const;

		/**
		 * @brief Get the shape with a given ID
		 * @param shapeID The shape ID
		 * @return The shape
		 */
		ref_ptr<BoundingShape> getShape(std::string_view shapeID) const;

		/**
		 * @brief Get the shape with a given ID and instance
		 * @param shapeID The shape ID
		 * @param instance The instance ID
		 * @return The shape
		 */
		ref_ptr<BoundingShape> getShape(std::string_view shapeID, uint32_t instance) const;

		/**
		 * @brief Get the shapes in the index
		 * @return The shapes
		 */
		auto &shapes() const { return itemBoundingShapes_; }

		/**
		 * @brief Get the number of shapes in the index
		 * @return The number of shapes
		 */
		unsigned int numShapes() const { return itemBoundingShapes_.size(); }

		/**
		 * @brief Get the shape at the given index
		 * @param itemIdx The item index
		 * @return The shape
		 */
		const ref_ptr<BoundingShape>& itemShape(uint32_t itemIdx) const { return itemBoundingShapes_[itemIdx]; }

		/**
		 * @brief Get the cameras in the index
		 * @return The cameras
		 */
		std::vector<const Camera *> cameras() const;

		/**
		 * @brief Update the index
		 * @param dt The time delta
		 */
		virtual void update(float dt) = 0;

		/**
		 * @brief Insert a shape into the index
		 * @param shape The shape
		 */
		virtual void insert(const ref_ptr<BoundingShape> &shape) = 0;

		/**
		 * @brief Remove a shape from the index
		 * @param shape The shape
		 */
		virtual void remove(const ref_ptr<BoundingShape> &shape) = 0;

		/**
		 * @brief Check if the index has intersection with a shape
		 * @param shape The shape
		 * @param traversalMask The traversal mask
		 * @return True if there is an intersection, false otherwise
		 */
		virtual bool hasIntersection(const BoundingShape &shape, uint32_t traversalMask) = 0;

		/**
		 * @brief Get the number of intersections with a shape
		 * @param shape The shape
		 * @param traversalMask The traversal mask
		 * @return The number of intersections
		 */
		virtual int numIntersections(const BoundingShape &shape, uint32_t traversalMask) = 0;

		/**
		 * @brief Run intersection tests and fill the hit buffer.
		 * The hit buffer is packed with shape indices, these point to the shapes array
		 * in this index.
		 * @param shape The shape
		 * @param mask The traversal mask
		 * @return The hit buffer
		 */
		virtual HitBuffer& foreachIntersection(const BoundingShape &shape, uint32_t mask) = 0;

		/**
		 * @brief Draw debug information
		 * @param debug The debug interface
		 */
		virtual void debugDraw(DebugInterface &debug) const;

		void addDebugShape(const ref_ptr<BoundingShape> &shape);

		void removeDebugShape(const ref_ptr<BoundingShape> &shape);

	protected:
		// pool for multithreaded jobs
		std::unique_ptr<JobPool> jobPool_;
		// max number of threads to use
		uint32_t maxNumThreads_ = std::thread::hardware_concurrency();
		// number of bits to use for distance in sort key
		DistanceKeySize distanceBits_ = DISTANCE_KEY_24;

		std::unordered_map<std::string_view, ref_ptr<std::vector<ref_ptr<BoundingShape>>>> nameToShape_;
		std::unordered_map<const Camera *, uint32_t> cameraToIndexCamera_;
		std::vector<IndexCamera> indexCameras_;
		std::vector<ref_ptr<BoundingShape>> itemBoundingShapes_;
		// additional shapes for debugging only
		std::vector<ref_ptr<BoundingShape>> debugShapes_;

		void updateVisibility(uint32_t traversalMask);

		void updateVisibility(IndexCamera *indexCamera);

		void updateLayerVisibility(IndexCamera &camera, uint32_t layerIdx, const BoundingShape &shape);

		/**
		 * @brief Add a shape to the index
		 * @param shape The shape to add
		 */
		void addToIndex(const ref_ptr<BoundingShape> &shape);

		/**
		 * @brief Remove a shape from the index
		 * @param shape The shape to remove
		 */
		void removeFromIndex(const ref_ptr<BoundingShape> &shape);

		static void debugBoundingShape(DebugInterface &debug, const BoundingShape &shape);

		static void createIndexShape(IndexCamera &ic, const ref_ptr<BoundingShape> &shape);

		friend struct VisibilityJob;

		void resetCamera(IndexCamera *indexCamera, DistanceKeySize distanceBits, uint32_t traversalMask);

	private:
		struct Private;
		Private *priv_;
	};

	/**
	 * @brief Job for updating visibility
	 */
	struct VisibilityJob {
		static void run(void *arg) {
			auto *ic = static_cast<IndexCamera *>(arg);
			ic->index->updateVisibility(ic);
		}
	};
} // namespace

#endif /* REGEN_SPATIAL_INDEX_H_ */
