#ifndef REGEN_INDEXED_SHAPE_H_
#define REGEN_INDEXED_SHAPE_H_

#include "regen/gl-types/shader-input.h"
#include "regen/shapes/bounding-shape.h"
#include "regen/camera/camera.h"

namespace regen {
	/**
	 * \brief Encapsulates information about a shape in the index and its visibility
	 * for a given camera.
	 */
	class IndexedShape {
	public:
		IndexedShape(const ref_ptr <Camera> &camera, const ref_ptr <BoundingShape> &shape);

		~IndexedShape() = default;

		/**
		 * \brief Check if the shape is visible
		 * \return True if the shape is visible, false otherwise
		 */
		bool isVisible() const;

		/**
		 * \brief Check if the shape has visible instances
		 * \return True if the shape has visible instances, false otherwise
		 */
		bool hasVisibleInstances() const;

		/**
		 * \brief Map the instance IDs for the shape
		 * \param mapMode The mapping mode
		 * \return The mapped data
		 */
		ShaderData_rw<unsigned int> mapInstanceIDs(int mapMode);

		/**
		 * \brief Get the camera
		 * \return The camera
		 */
		auto &camera() const { return camera_; }

		/**
		 * \brief Get the shape
		 * \return The shape
		 */
		auto &shape() const { return shape_; }

	protected:
		ref_ptr <Camera> camera_;
		ref_ptr <BoundingShape> shape_;
		bool visible_ = true;
		unsigned int instanceCount_ = 1;
		ref_ptr <ShaderInput1ui> visibleVec_;

		std::set<unsigned int> u_visibleSet_;
		unsigned int u_instanceCount_ = 0;
		bool u_visible_ = false;

		struct ShapeDistance {
			const BoundingShape *shape;
			float distance;
		};
		std::vector<ShapeDistance> instanceDistances_;

		struct MappedData {
			explicit MappedData(const ref_ptr <ShaderInput1ui> &visibleVec);

			ShaderData_rw<unsigned int> mapped;
		};
		std::optional <MappedData> mappedInstanceIDs_;

		void mapInstanceIDs_internal();

		void unmapInstanceIDs_internal();

		unsigned int *mappedInstanceIDs();

		friend class SpatialIndex;
	};
} // namespace

#endif /* REGEN_SPATIAL_INDEX_H_ */
