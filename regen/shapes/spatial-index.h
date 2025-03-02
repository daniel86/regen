#ifndef REGEN_SPATIAL_INDEX_H_
#define REGEN_SPATIAL_INDEX_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/indexed-shape.h>
#include <regen/camera/camera.h>
#include "regen/utility/debug-interface.h"
#include "regen/utility/ThreadPool.h"

namespace regen {
	/**
	 * @brief Spatial index
	 */
	class SpatialIndex {
	public:
		SpatialIndex();

		virtual ~SpatialIndex() = default;

		/**
		 * @brief Get the indexed shape for a camera
		 * @param camera The camera
		 * @param shapeName The shape name
		 * @return The indexed shape
		 */
		ref_ptr<IndexedShape> getIndexedShape(const ref_ptr<Camera> &camera, std::string_view shapeName);

		/**
		 * @brief Add a camera to the index
		 * @param camera The camera
		 */
		void addCamera(const ref_ptr<Camera> &camera, bool sortInstances = true);

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
		 * @param shapeID The shape ID
		 * @return True if the shape is visible, false otherwise
		 */
		bool isVisible(const Camera &camera, std::string_view shapeID);

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
		 * @brief Get the shapes in the index
		 * @return The shapes
		 */
		auto &shapes() const { return shapes_; }

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
		 * @return True if there is an intersection, false otherwise
		 */
		virtual bool hasIntersection(const BoundingShape &shape) = 0;

		/**
		 * @brief Get the number of intersections with a shape
		 * @param shape The shape
		 * @return The number of intersections
		 */
		virtual int numIntersections(const BoundingShape &shape) = 0;

		/**
		 * @brief Iterate over all intersections with a shape
		 * @param shape The shape
		 * @param callback The callback function
		 */
		virtual void foreachIntersection(
				const BoundingShape &shape,
				const std::function<void(const BoundingShape &)> &callback) = 0;

		/**
		 * @brief Draw debug information
		 * @param debug The debug interface
		 */
		virtual void debugDraw(DebugInterface &debug) const = 0;

	protected:
		ThreadPool threadPool_;
		struct IndexCamera {
			ref_ptr<Camera> camera;
			std::map<std::string_view, ref_ptr<IndexedShape>> shapes;
			bool sortInstances;
		};
		std::map<std::string_view, std::vector<ref_ptr<BoundingShape>>> shapes_;
		std::map<const Camera *, IndexCamera> cameras_;

		void updateVisibility();

		void updateVisibility(IndexCamera &camera, const BoundingShape &shape, bool isMultiShape);

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

		static void createIndexShape(IndexCamera &ic, const ref_ptr<BoundingShape> &shape);
	};
} // namespace

#endif /* REGEN_SPATIAL_INDEX_H_ */
