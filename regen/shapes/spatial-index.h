#ifndef REGEN_SPATIAL_INDEX_H_
#define REGEN_SPATIAL_INDEX_H_

#include <map>
#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/indexed-shape.h>
#include <regen/camera/camera.h>
#include "regen/utility/debug-interface.h"
#include "regen/utility/radix-sort-cpu.h"
#include <regen/scene/loading-context.h>

namespace regen {
	/**
	 * @brief Spatial index
	 */
	class SpatialIndex : public Resource {
	public:
		static constexpr const char *TYPE_NAME = "SpatialIndex";
		// Threshold for using standard sort (small arrays) vs radix sort (large arrays)
		static constexpr uint32_t SMALL_ARRAY_SIZE = 256;

		SpatialIndex();

		~SpatialIndex() override = default;

		static ref_ptr<SpatialIndex> load(LoadingContext &ctx, scene::SceneInputNode &input);

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
		 * @param camera The camera
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
		 * @param callback The callback function
		 * @param userData User data passed to the callback
		 * @param traversalMask The traversal mask
		 */
		virtual void foreachIntersection(
				const BoundingShape &shape,
				void (*callback)(const BoundingShape&, void*),
				void *userData,
				uint32_t traversalMask) = 0;

		/**
		 * @brief Draw debug information
		 * @param debug The debug interface
		 */
		virtual void debugDraw(DebugInterface &debug) const;

		void addDebugShape(const ref_ptr<BoundingShape> &shape);

		void removeDebugShape(const ref_ptr<BoundingShape> &shape);

	protected:
		struct IndexCamera {
			ref_ptr<Camera> cullCamera;
			ref_ptr<Camera> sortCamera;
			std::unordered_map<std::string_view, ref_ptr<IndexedShape>> nameToShape_;
			// flattened list of shapes for faster access
			std::vector<IndexedShape*> indexShapes_;
			// These vectors are filled up during traversal and sorted
			// according to the instance distance to the camera.
			std::vector<uint32_t> tmp_layerInstances_; // size = sum_{shape} numLayers * numInstances_{shape}
			std::vector<uint32_t> tmp_layerShapes_; // size = sum_{shape} numLayers * numInstances_{shape}
			std::vector<uint64_t> tmp_sortKeys_; // size = sum_{shape} numLayers * numInstances_{shape}
			// total number of keys = sum_{shape} numLayers * numInstances_{shape}
			uint32_t numKeys = 0;
			// mark camera as dirty when shapes are added/removed
			bool isDirty = false;
			// how to sort instances by distance to camera
			SortMode sortMode = SortMode::FRONT_TO_BACK;
			// radix sort for sorting the instances
			RadixSort_CPU_seq<uint32_t, uint64_t, 8> radixSort;
			Vec4i lodShift = Vec4i(0);
		};
		std::unordered_map<std::string_view, ref_ptr<std::vector<ref_ptr<BoundingShape>>>> nameToShape_;
		std::unordered_map<const Camera *, IndexCamera> cameras_;
		// additional shapes for debugging only
		std::vector<ref_ptr<BoundingShape>> debugShapes_;

		void updateVisibility(uint32_t traversalMask);

		void updateLayerVisibility(IndexCamera &camera, uint32_t layerIdx, const BoundingShape &shape, uint32_t traversalMask);

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

		static void handleIntersection(const BoundingShape &b_shape, void *userData);

		static void updateLOD_Major(IndexCamera &indexCamera, IndexedShape *indexShape, uint32_t shapeBase);
	};
} // namespace

#endif /* REGEN_SPATIAL_INDEX_H_ */
