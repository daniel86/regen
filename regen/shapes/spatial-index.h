#ifndef REGEN_SPATIAL_INDEX_H_
#define REGEN_SPATIAL_INDEX_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/indexed-shape.h>
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

	/**
	 * @brief Buffer for storing hit shape indices.
	 */
	struct HitBuffer {
		// Aligned array of shape indices
		AlignedArray<uint32_t> data;
		// Current count of hits
		uint32_t count = 0;
		void resize(uint32_t newSize) { data.resize(newSize); }
		void reset() { count = 0; }
		void push(uint32_t v) { data[count++] = v; }
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

		~SpatialIndex() override = default;

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
		 * @param sortCamera The sorting camera
		 * @param sortMode The sort mode
		 * @param lodShift The LOD shift
		 */
		void addCamera(
				const ref_ptr<Camera> &cullCamera,
				const ref_ptr<Camera> &sortCamera,
				SortMode sortMode,
				Vec4i lodShift);

		/**
		 * @brief Check if the index has a camera
		 * @param camera The camera
		 * @return True if the index has the camera, false otherwise
		 */
		bool hasCamera(const Camera &camera) const;

		/**
		 * @brief Check if a shape is visible
		 * Note: update must be called before this function
		 * @param camera The camera
		 * @param layerIdx The layer index
		 * @param shapeID The shape ID
		 * @return True if the shape is visible, false otherwise
		 */
		bool isVisible(const Camera &camera, uint32_t layerIdx, std::string_view shapeID);

		/**
		 * @brief Get the number of instances of a shape
		 * @param shapeID The shape ID
		 * @return The number of instances
		 */
		GLuint numInstances(std::string_view shapeID) const;

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
		auto &shapes() const { return nameToShape_; }

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
		 * @brief Iterate over all intersections with a shape
		 * @param shape The shape
		 * @param callback The intersection callback
		 * @param traversalMask The traversal mask
		 */
		virtual void foreachIntersection(
				const BoundingShape &shape,
				const IntersectionCallback &callback,
				uint32_t traversalMask) = 0;

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

		template <typename RadixType>
		static std::unique_ptr<void, void(*)(void*)> createRadix(size_t numKeys) {
			return std::unique_ptr<void, void(*)(void*)>(
				new RadixType(numKeys),
				[](void* ptr) { delete static_cast<RadixType*>(ptr); });
		}

		enum SortFunType { SORT_FUN_LARGE = 0, SORT_FUN_SMALL = 1 };
		using SortFun = void (*)(void*, void*, std::vector<uint32_t>&);

		// data for each camera
		struct IndexCamera {
			SpatialIndex *index = nullptr;
			ref_ptr<Camera> cullCamera;
			ref_ptr<Camera> sortCamera;
			std::unordered_map<std::string_view, ref_ptr<IndexedShape>> nameToShape_;
			// flattened list of shapes for faster access
			std::vector<IndexedShape*> indexShapes_;
			// These vectors are filled up during traversal and sorted according to the instance distance to the camera.
			// Global instance idx used to access the key arrays
			std::vector<uint32_t> tmp_globalInstanceIDs_; // size = sum_{shape} numLayers * numInstances_{shape}
			// Local instance idx used to generate the output instance IDs
			std::vector<uint32_t> tmp_localInstanceIDs_;  // size = sum_{shape} numLayers * numInstances_{shape}
			std::vector<uint64_t> tmp_sortKeys64_;  // size = sum_{shape} numLayers * numInstances_{shape}
			std::vector<uint32_t> tmp_sortKeys32_;  // size = sum_{shape} numLayers * numInstances_{shape}
			// total number of keys = sum_{shape} numLayers * numInstances_{shape}
			uint32_t numKeys = 0;
			// the bitmask to filter shapes during traversal
			uint32_t traversalMask = 0;
			// index of this camera in the index's camera list
			uint32_t camIdx = 0;
			// mark camera as dirty when shapes are added/removed
			bool isDirty = false;
			// how to sort instances by distance to camera
			SortMode sortMode = SortMode::FRONT_TO_BACK;
			// radix sort for sorting the instances
			std::unique_ptr<void, void(*)(void*)> radixSort =
				createRadix<RadixSort_CPU_seq<uint32_t, uint32_t, 8, 32>>(10);
			void *sortKeys = nullptr; // erased type
			// sorting functions, one for large arrays, one for small arrays
			SortFun sortFun[2] = { nullptr, nullptr };
			uint8_t layerBits = 0u;
			uint8_t shapeBits = 0u;
			uint8_t keyBits = 0u;
			Vec4i lodShift = Vec4i(0);
			// a function to push sort keys
			void (*pushKeyFun)(IndexCamera*, uint32_t, uint16_t, uint32_t, float, SortMode) = nullptr;
			static void pushKey32(IndexCamera*, uint32_t, uint16_t, uint32_t, float, SortMode);
			static void pushKey64(IndexCamera*, uint32_t, uint16_t, uint32_t, float, SortMode);
			// a function to create a distance key
			uint32_t (*setDistance32)(float, SortMode) = nullptr;
			uint64_t (*setDistance64)(float, SortMode) = nullptr;
		};
		std::unordered_map<std::string_view, ref_ptr<std::vector<ref_ptr<BoundingShape>>>> nameToShape_;
		std::unordered_map<const Camera *, uint32_t> cameraToIndexCamera_;
		std::vector<IndexCamera> indexCameras_;
		std::vector<ref_ptr<BoundingShape>> itemShapes_;
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

		void debugBoundingShape(DebugInterface &debug, const BoundingShape &shape) const;

		static void createIndexShape(IndexCamera &ic, const ref_ptr<BoundingShape> &shape);

		// used internally when handling intersections
		struct TraversalData {
			SpatialIndex *index;
			IndexCamera *indexCamera;
			const Vec3f *camPos;
			uint32_t layerIdx;
		};

		friend struct VisibilityJob;

		static void handleIntersection(const BoundingShape &b_shape, void *userData);

		static void resetCamera(IndexCamera *indexCamera, DistanceKeySize distanceBits, uint32_t traversalMask);
	};

	/**
	 * @brief Job for updating visibility
	 */
	struct VisibilityJob {
		static inline void run(void *arg) {
			auto *ic = static_cast<SpatialIndex::IndexCamera *>(arg);
			ic->index->updateVisibility(ic);
		}
	};
} // namespace

#endif /* REGEN_SPATIAL_INDEX_H_ */
