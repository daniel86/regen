#ifndef REGEN_BOUNDING_BOX_H_
#define REGEN_BOUNDING_BOX_H_

#include <regen/shapes/bounding-shape.h>
#include <regen/objects/mesh.h>
#include "bounds.h"

namespace regen {
	/**
	 * @brief Bounding box
	 */
	template <typename BatchType>
	class BoundingBox : public BatchedBoundingShape<BatchType> {
	public:
		/**
		 * @brief Construct a new Bounding Box object
		 * @param type The type of the box
		 * @param mesh The mesh
		 */
		BoundingBox(BoundingShapeType type, const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts)
		: BatchedBoundingShape<BatchType>(type, mesh, parts),
				  baseBounds_(Bounds<Vec3f>::create(mesh->minPosition(), mesh->maxPosition())) {
			for (const auto &part : parts) {
				baseBounds_.min.setMin(part->minPosition());
				baseBounds_.max.setMax(part->maxPosition());
			}
			basePosition_ = (baseBounds_.max + baseBounds_.min) * 0.5f;
		}

		/**
		 * @brief Construct a new Bounding Box object
		 * @param type The type of the box
		 * @param bounds The min/max bounds of the box's vertices (without transformation)
		 */
		BoundingBox(BoundingShapeType type, const Bounds<Vec3f> &bounds)
		: BatchedBoundingShape<BatchType>(type),
		  baseBounds_(bounds),
		  basePosition_((bounds.max + bounds.min) * 0.5f) {}

		~BoundingBox() override = default;

		/**
		 * @brief Get the min/max bounds of the box's vertices (without transformation)
		 * @return The bounds
		 */
		const Bounds<Vec3f> &baseBounds() const { return baseBounds_; }

	protected:
		// min/max bounds of the box's vertices (without transformation)
		Bounds<Vec3f> baseBounds_;
		// The center of the box before transformation
		Vec3f basePosition_ = Vec3f::zero();
	};
} // namespace

#endif /* REGEN_BOUNDING_BOX_H_ */
