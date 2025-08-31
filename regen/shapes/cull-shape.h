#ifndef REGEN_CULL_SHAPE_H_
#define REGEN_CULL_SHAPE_H_

#include <regen/states/state.h>
#include "regen/buffer/ssbo.h"
#include "regen/states/model-transformation.h"
#include "spatial-index.h"

namespace regen {
	// forward declaration, because Mesh header includes this header
	class Mesh;

	/**
	 * \brief A cull shape is a state that holds the information about a shape that is used for culling.
	 *
	 * It can be used to cull meshes based on their bounding shape and spatial index.
	 */
	class CullShape : public State {
	public:
		/**
		 * \brief Create a CullShape with a spatial index.
		 * @param spatialIndex the spatial index to use for culling.
		 * @param shapeName the name of the shape.
		 */
		CullShape(
			const ref_ptr<SpatialIndex> &spatialIndex,
			std::string_view shapeName);

		/**
		 * \brief Create a CullShape with a bounding shape.
		 * @param boundingShape the bounding shape to use for culling.
		 * @param shapeName the name of the shape.
		 */
		CullShape(
			const ref_ptr<BoundingShape> &boundingShape,
			std::string_view shapeName);

		/**
		 * @return the number of instances of this shape.
		 */
		uint32_t numInstances() const { return numInstances_; }

		/**
		 * @return the name of the shape.
		 */
		const std::string& shapeName() const { return shapeName_; }

		/**
		 * @return true if this shape is used for culling based on spatial index.
		 */
		bool isIndexShape() const { return spatialIndex_.get() != nullptr; }

		/**
		 * @return the spatial index used for culling, if any.
		 */
		const ref_ptr<SpatialIndex> &spatialIndex() const { return spatialIndex_; }

		/**
		 * @return the parts of this shape, i.e. the meshes that are part of this shape.
		 */
		const std::vector<ref_ptr<Mesh>> &parts() const { return parts_; }

		/**
		 * @return the bounding shape of this cull shape.
		 */
		const ref_ptr<BoundingShape> &boundingShape() const { return boundingShape_; }

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
		 * \brief Compute the index into the flattened (lod, layer) arrays
		 * \param lodLevel The LOD level
		 * \param layerIdx The layer index
		 * \param numLayers The number of layers
		 * \return The index into the flattened array
		 */
		static inline uint32_t binIdx(uint32_t lodLevel, uint32_t layerIdx, uint32_t numLayers) {
			return lodLevel * numLayers + layerIdx;
		}

		/**
		 * @param mode The sort mode to set
		 */
		void setInstanceSortMode(SortMode mode) { instanceSortMode_ = mode; }

		/**
		 * @return The sort mode used for this indexed shape
		 */
		SortMode instanceSortMode() const { return instanceSortMode_; }

	protected:
		std::string shapeName_;
		std::vector<ref_ptr<Mesh>> parts_;
		uint32_t numInstances_ = 1u;
		ref_ptr<SpatialIndex> spatialIndex_;
		ref_ptr<BoundingShape> boundingShape_;
		SortMode instanceSortMode_ = SortMode::FRONT_TO_BACK;

		ref_ptr<SSBO> instanceBuffer_;
		std::vector<ref_ptr<SSBO>> indirectDrawBuffers_;

		void initCullShape(
				const ref_ptr<BoundingShape> &boundingShape,
				bool isIndexShape);
	};
} // namespace

#endif /* REGEN_CULL_SHAPE_H_ */
