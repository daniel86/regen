#ifndef REGEN_BOUNDING_BOX_H_
#define REGEN_BOUNDING_BOX_H_

#include <regen/shapes/bounding-shape.h>
#include "bounds.h"

namespace regen {
	/**
	 * @brief Bounding box type
	 */
	enum class BoundingBoxType {
		AABB = 0,
		OBB
	};

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
		BoundingBox(BoundingBoxType type, const ref_ptr<Mesh> &mesh);

		/**
		 * @brief Construct a new Bounding Box object
		 * @param type The type of the box
		 * @param bounds The min/max bounds of the box's vertices (without transformation)
		 */
		BoundingBox(BoundingBoxType type, const Bounds<Vec3f> &bounds);

		~BoundingBox() override = default;

		/**
		 * @brief Get the type of this box
		 * @return The type
		 */
		auto boxType() const { return type_; }

		/**
		 * @brief Check if this box is an AABB
		 * @return True if this box is an AABB, false otherwise
		 */
		auto isAABB() const { return type_ == BoundingBoxType::AABB; }

		/**
		 * @brief Check if this box is an OBB
		 * @return True if this box is an OBB, false otherwise
		 */
		auto isOBB() const { return type_ == BoundingBoxType::OBB; }

		/**
		 * @brief Get the min/max bounds of the box's vertices (without transformation)
		 * @return The bounds
		 */
		auto &bounds() const { return bounds_; }

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
		bool hasIntersectionWithBox(const BoundingBox &box) const;

		/**
		 * @brief Project this box onto an axis
		 * @param axis The axis
		 * @return The min/max projection
		 */
		std::pair<float, float> project(const Vec3f &axis) const;

		// BoundingShape interface
		Vec3f getCenterPosition() const override;

		// BoundingShape interface
		void updateBounds(const Vec3f &min, const Vec3f &max) override;

	protected:
		BoundingBoxType type_;
		// min/max bounds of the box's vertices (without transformation)
		Bounds<Vec3f> bounds_;
		Vec3f basePosition_;
		// transformed vertices
		Vec3f vertices_[8];
	};
} // namespace

#endif /* REGEN_BOUNDING_BOX_H_ */
