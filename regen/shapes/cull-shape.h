#ifndef REGEN_CULL_SHAPE_H_
#define REGEN_CULL_SHAPE_H_

#include <regen/states/state.h>
#include "regen/buffer/ssbo.h"
#include "regen/states/model-transformation.h"
#include "indexed-shape.h"
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
			std::string_view shapeName,
			bool useSharedInstanceBuffer = false);

		/**
		 * \brief Create a CullShape with a bounding shape.
		 * @param boundingShape the bounding shape to use for culling.
		 * @param shapeName the name of the shape.
		 */
		CullShape(
			const ref_ptr<BoundingShape> &boundingShape,
			std::string_view shapeName,
			bool useSharedInstanceBuffer = false);

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
		 * Note that by default the cull shape is not using a shared instance buffer,
		 * as it is better for performance to use a per-frame updates for instance data.
		 * @return true if this shape manages an instance buffer.
		 */
		bool hasInstanceBuffer() const { return instanceBuffer_.get() != nullptr; }

		/**
		 * @return the instance ID map, if any (only used if num instances > 1).
		 */
		const ref_ptr<ShaderInput1ui>& instanceData() const { return instanceData_; }

		/**
		 * @return the instance ID buffer, if any (only used if num instances > 1).
		 */
		const ref_ptr<SSBO>& instanceBuffer() const { return instanceBuffer_; }

	protected:
		std::string shapeName_;
		std::vector<ref_ptr<Mesh>> parts_;
		uint32_t numInstances_ = 1u;
		ref_ptr<SpatialIndex> spatialIndex_;
		ref_ptr<BoundingShape> boundingShape_;

		// a "shared" instance buffer.
		// shared because the same buffer is used in different passes per draw with different
		// ordering of instance IDs (depending on camera). This is not optimal in terms of memory pressure,
		// but allows to use the same buffer for different passes in case memory runs low.
		ref_ptr<ShaderInput1ui> instanceData_;
		ref_ptr<SSBO> instanceBuffer_;

		void initCullShape(
				const ref_ptr<BoundingShape> &boundingShape,
				bool isIndexShape,
				bool useSharedInstanceBuffer);

		void createBuffers();
	};
} // namespace

#endif /* REGEN_CULL_SHAPE_H_ */
