#ifndef REGEN_AABB_H_
#define REGEN_AABB_H_

#include <regen/shapes/bounding-box.h>

namespace regen {
	/**
	 * @brief Axis-aligned bounding box
	 */
	class AABB : public BoundingBox {
	public:
		/**
		 * @brief Construct a new AABB object
		 * @param mesh The mesh
		 */
		AABB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts);

		/**
		 * @brief Construct a new AABB object
		 * @param bounds The min/max of the AABB
		 */
		AABB(const Bounds<Vec3f> &bounds);

		~AABB() override = default;

		/**
		 * @brief Check if this AABB has intersection with another AABB
		 * @param other The other AABB
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithAABB(const AABB &other) const;

		// override BoundingBox::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// override BoundingBox::update
		bool updateTransform(bool forceUpdate) final;

		// override BoundingBox::axes
		const Vec3f *boxAxes() const final;

	protected:
		void updateAABB();

		void setVertices(const Bounds<Vec3f> &minMax);
	};
} // namespace

#endif /* REGEN_AABB_H_ */
