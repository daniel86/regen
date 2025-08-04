#ifndef REGEN_INDEXED_SHAPE_H_
#define REGEN_INDEXED_SHAPE_H_

#include "regen/glsl/shader-input.h"
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
		 * \brief Get the number of visible instances
		 * @return The number of visible instances
		 */
		uint32_t numVisibleInstances() const { return instanceCount_; }

		/**
		 * \brief Map the instance IDs for the shape
		 * \param mapMode The mapping mode
		 * \return The mapped data
		 */
		ClientData_rw<unsigned int> mapInstanceIDs(int mapMode);

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

		/**
		 * @param instanceBuffer The instance buffer to set
		 */
		void setInstanceBuffer(const ref_ptr<SSBO> &instanceBuffer) { instanceBuffer_ = instanceBuffer; }

		/**
		 * @return The instance buffer used for this indexed shape
		 */
		const ref_ptr<SSBO> &instanceBuffer() const { return instanceBuffer_; }

		/**
		 * \brief Check if the indexed shape has an instance buffer
		 */
		bool hasInstanceBuffer() const { return instanceBuffer_.get() != nullptr; }

		/**
		 * @return The indirect draw buffer used for this indexed shape
		 */
		const ref_ptr<SSBO> &indirectDrawBuffer(uint32_t partIdx) const { return indirectDrawBuffers_[partIdx]; }

		/**
		 * Note: Each part of a model has its own indirect draw buffer.
		 * @param indirectDrawBuffers The indirect draw buffers to set
		 */
		void setIndirectDrawBuffers(const std::vector<ref_ptr<SSBO>> &indirectDrawBuffers) {
			indirectDrawBuffers_ = indirectDrawBuffers;
		}

		/**
		 * @return True if the indexed shape has an indirect draw buffer
		 */
		bool hasIndirectDrawBuffers() const { return !indirectDrawBuffers_.empty(); }

		/**
		 * @param mode The sort mode to set
		 */
		void setSortMode(SortMode mode) { instanceSortMode_ = mode; }

		/**
		 * @return The sort mode used for this indexed shape
		 */
		SortMode instanceSortMode() const { return instanceSortMode_; }

	protected:
		ref_ptr <Camera> camera_;
		ref_ptr <BoundingShape> shape_;
		bool visible_ = true;
		unsigned int instanceCount_ = 1;
		ref_ptr<ShaderInput1ui> visibleVec_;
		ref_ptr<SSBO> instanceBuffer_;
		std::vector<ref_ptr<SSBO>> indirectDrawBuffers_;

		SortMode instanceSortMode_ = SortMode::FRONT_TO_BACK;

		unsigned int u_instanceCount_ = 0;
		bool u_visible_ = false;

		struct ShapeDistance {
			const BoundingShape *shape;
			float distance;
		};
		std::vector<ShapeDistance> instanceDistances_;
		std::vector<ref_ptr<BoundingShape>> boundingShapes_;

		struct MappedData {
			explicit MappedData(const ref_ptr <ShaderInput1ui> &visibleVec);

			ClientData_rw<unsigned int> mapped;
		};
		std::optional <MappedData> mappedInstanceIDs_;

		void mapInstanceIDs_internal();

		void unmapInstanceIDs_internal();

		unsigned int *mappedInstanceIDs();

		friend class SpatialIndex;
	};
} // namespace

#endif /* REGEN_SPATIAL_INDEX_H_ */
