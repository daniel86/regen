#ifndef REGEN_BOUNDING_BOX_H_
#define REGEN_BOUNDING_BOX_H_

#include <regen/shapes/bounding-shape.h>
#include "bounds.h"

namespace regen {
	/**
	 * @brief Bounding box
	 */
	class BoundingBox : public BoundingShape {
	public:
		/**
		 * @brief Construct a new Bounding Box object
		 * @param type The type of the box
		 * @param mesh The mesh
		 */
		BoundingBox(BoundingShapeType type, const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts);

		/**
		 * @brief Construct a new Bounding Box object
		 * @param type The type of the box
		 * @param bounds The min/max bounds of the box's vertices (without transformation)
		 */
		BoundingBox(BoundingShapeType type, const Bounds<Vec3f> &bounds);

		~BoundingBox() override = default;

		/**
		 * @brief Get the min/max bounds of the box's vertices (without transformation)
		 * @return The bounds
		 */
		const Bounds<Vec3f> &baseBounds() const { return baseBounds_; }

		/**
		 * @brief Get the min/max bounds of the box's vertices after transformation
		 * @return The bounds
		 */
		const Bounds<Vec3f> &tfBounds() const { return tfBounds_; }

		/**
		 * @brief Get the vertices of this box
		 * @return The vertices
		 */
		auto *boxVertices() const { return vertices_; }

		/**
		 * @brief Get the axes of this box
		 * @return The axes
		 */
		virtual const Vec3f *boxAxes() const = 0;

		/**
		 * @brief Check if this box has intersection with another box
		 * @param other The other box
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithBox(const BoundingBox &other) const;

		/**
		 * @brief Project this box onto an axis
		 * @param axis The axis
		 * @return The min/max projection
		 */
		std::pair<float, float> project(const Vec3f &axis) const;

		// BoundingShape interface
		void updateBaseBounds(const Vec3f &min, const Vec3f &max) override;

	protected:
		// min/max bounds of the box's vertices (without transformation)
		Bounds<Vec3f> baseBounds_;
		// min/max bounds of the box's vertices after transformation
		Bounds<Vec3f> tfBounds_;
		// The center of the box before transformation
		Vec3f basePosition_;
		// transformed vertices
		Vec3f vertices_[8];
	};
} // namespace

#endif /* REGEN_BOUNDING_BOX_H_ */
