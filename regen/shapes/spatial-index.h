#ifndef REGEN_SPATIAL_INDEX_H_
#define REGEN_SPATIAL_INDEX_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/camera/camera.h>
#include "regen/utility/debug-interface.h"

namespace regen {
	/**
	 * @brief Spatial index
	 */
	class SpatialIndex {
	public:
		SpatialIndex() = default;

		virtual ~SpatialIndex() = default;

		/**
		 * @brief Add a camera to the index
		 * @param camera The camera
		 */
		void addCamera(const ref_ptr<Camera> &camera);

		/**
		 * @brief Check if a shape is visible
		 * Note: update must be called before this function
		 * @param camera The camera
		 * @param shapeID The shape ID
		 * @return True if the shape is visible, false otherwise
		 */
		bool isVisible(const Camera &camera, std::string_view shapeID);

		/**
		 * @brief Get the visible instances of a shape
		 * Note: update must be called before this function
		 * @param camera The camera
		 * @param shapeID The shape ID
		 * @return The visible instances
		 */
		const std::vector<unsigned int> &
		getVisibleInstances(const Camera &camera, std::string_view shapeID);

		/**
		 * @brief Get the number of instances of a shape
		 * @param shapeID The shape ID
		 * @return The number of instances
		 */
		GLuint numInstances(std::string_view shapeID) const;

		/**
		 * @brief Get the mesh of a shape
		 * @param shapeID The shape ID
		 * @return The mesh
		 */
		ref_ptr<Mesh> getMeshOfShape(std::string_view shapeID) const;

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
		virtual bool hasIntersection(const BoundingShape &shape) const = 0;

		/**
		 * @brief Get the number of intersections with a shape
		 * @param shape The shape
		 * @return The number of intersections
		 */
		virtual int numIntersections(const BoundingShape &shape) const = 0;

		/**
		 * @brief Iterate over all intersections with a shape
		 * @param shape The shape
		 * @param callback The callback function
		 */
		virtual void foreachIntersection(
				const BoundingShape &shape,
				const std::function<void(const BoundingShape &)> &callback) const = 0;

		/**
		 * @brief Draw debug information
		 * @param debug The debug interface
		 */
		virtual void debugDraw(DebugInterface &debug) const = 0;

	protected:
		std::vector<ref_ptr<BoundingShape>> shapes_;
		std::vector<ref_ptr<Camera>> cameras_;
		std::vector<ref_ptr<Camera>> omniCameras_;
		std::map<const Camera *, std::set<std::string_view>> visibleShapes_;
		std::map<const Camera *, std::map<std::string_view, std::vector<unsigned int>>> visibleInstances_;

		void updateVisibility();

		void updateVisibility(const ref_ptr<Camera> &camera, const BoundingShape &shape);

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
	};
} // namespace

#endif /* REGEN_SPATIAL_INDEX_H_ */
