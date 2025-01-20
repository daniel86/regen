#ifndef REGEN_OBB_H_
#define REGEN_OBB_H_

#include <regen/shapes/bounding-box.h>
#include <regen/shapes/aabb.h>

namespace regen {
	/**
	 * @brief Oriented bounding box
	 */
	class OBB : public BoundingBox {
	public:
		/**
		 * @brief Construct a new OBB object
		 * @param mesh The mesh
		 */
		explicit OBB(const ref_ptr<Mesh> &mesh);

		/**
		 * @brief Construct a new OBB object
		 * @param halfSize The half size of the OBB
		 */
		explicit OBB(const Bounds<Vec3f> &bounds);

		~OBB() override = default;

		/**
		 * @brief Check if this OBB has intersection with another bounding box
		 * @param other The other bounding box
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithOBB(const BoundingBox &other) const;

		// override BoundingBox::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// override BoundingBox::axes
		const Vec3f *boxAxes() const final { return boxAxes_; }

		// override BoundingBox::update
		bool updateTransform(bool forceUpdate) final;

	protected:
		Vec3f boxAxes_[3];

		bool overlapOnAxis(const BoundingBox &b, const Vec3f &axis) const;

		void updateOBB();
	};
} // namespace

#endif /* REGEN_OBB_H_ */
