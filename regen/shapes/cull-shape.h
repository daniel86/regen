#ifndef REGEN_CULL_SHAPE_H_
#define REGEN_CULL_SHAPE_H_

#include <regen/states/state.h>
#include "regen/gl-types/ssbo.h"
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
		CullShape(const ref_ptr<SpatialIndex> &spatialIndex, std::string_view shapeName);

		/**
		 * \brief Create a CullShape with a bounding shape.
		 * @param boundingShape the bounding shape to use for culling.
		 * @param shapeName the name of the shape.
		 */
		CullShape(const ref_ptr<BoundingShape> &boundingShape, std::string_view shapeName);

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
		 * @return the model transformation of this shape, if any.
		 */
		const ref_ptr<ModelTransformation> &tf() const { return tf_; }

		/**
		 * @return the model offset of this shape, if any.
		 */
		const ref_ptr<ShaderInput3f> &modelOffset() const { return modelOffset_; }

		/**
		 * @return the instance ID map, if any (only used if num instances > 1).
		 */
		const ref_ptr<ShaderInput1ui>& instanceIDMap() const { return instanceIDMap_; }

		/**
		 * @return the instance ID buffer, if any (only used if num instances > 1).
		 */
		const ref_ptr<SSBO>& instanceIDBuffer() const { return instanceIDBuffer_; }

	protected:
		std::string shapeName_;
		std::vector<ref_ptr<Mesh>> parts_;
		ref_ptr<ModelTransformation> tf_;
		ref_ptr<ShaderInput3f> modelOffset_;
		uint32_t numInstances_ = 1u;
		ref_ptr<ShaderInput1ui> instanceIDMap_;
		// stores sorted instanceIDs, first sort criteria is the LOD group, second distance to camera
		ref_ptr<SSBO> instanceIDBuffer_;
		ref_ptr<SpatialIndex> spatialIndex_;

		void initCullShape(const ref_ptr<BoundingShape> &boundingShape, bool isIndexShape);

		void createBuffers();
	};
} // namespace

#endif /* REGEN_CULL_SHAPE_H_ */
