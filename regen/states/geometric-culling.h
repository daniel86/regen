/*
 * geometric-culling.h
 *
 *  Created on: Oct 17, 2014
 *      Author: daniel
 */

#ifndef GEOMETRIC_CULLING_H_
#define GEOMETRIC_CULLING_H_

#include <regen/states/state-node.h>
#include <regen/shapes/spatial-index.h>
#include "regen/camera/sorting.h"
#include "regen/gl-types/ssbo.h"

namespace regen {
	/**
	 * @brief Geometric culling state node
	 */
	class GeometricCulling : public StateNode {
	public:
		/**
		 * @brief Construct a new Geometric Culling object
		 * @param camera The camera
		 * @param spatialIndex The spatial index
		 * @param shapeName The shape name
		 */
		GeometricCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<SpatialIndex> &spatialIndex,
				std::string_view shapeName);

		GeometricCulling(
				const ref_ptr<Camera> &camera,
				const std::vector<ref_ptr<Mesh>> &meshVector,
				const ref_ptr<ModelTransformation> &tf);

		~GeometricCulling() override = default;

		/**
		 * @brief Set the instance sorting
		 * @param instanceSorting The instance sorting
		 */
		void setInstanceSortMode(SortMode mode) { instanceSortMode_ = mode; }

		void traverse(RenderState *rs) override;

	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<SpatialIndex> spatialIndex_;
		ref_ptr<IndexedShape> shapeIndex_;
		SortMode instanceSortMode_ = SortMode::FRONT_TO_BACK;

		GLuint numInstances_ = 1;
		// stores sorted instanceIDs, first sort criteria is the LOD group, second distance to camera
		ref_ptr<SSBO> instanceIDBuffer_;
		ref_ptr<ShaderInput1ui> instanceIDMap_;
		// provides offset to instanceIDMap_ as a uniform for the next LOD level
		ref_ptr<ShaderInput1i> instanceIDOffset_;
		// stores how many instances are currently visible for each LOD level
		std::vector<uint32_t> lodNumInstances_;
		// temporary storage for instanceIDs, used to fill the instanceIDMap_
		std::vector<std::vector<GLuint>> lodGroups_;
		ref_ptr<Mesh> mesh_;
		std::vector<ref_ptr<Mesh>> meshVector_;
		ref_ptr<ModelTransformation> tf_;

		void createInstanceBuffer();

		void updateMeshLOD();

		void activateLOD(uint32_t lodLevel);

		void computeLODGroups();

		void traverseInstanced_(RenderState *rs, unsigned int numVisible);

		void traverseCPU(RenderState *rs);

		void traverseGPU(RenderState *rs);

		void computeLODGroups_(
			const uint32_t *mappedData,
			int begin,
			int end,
			int increment);
	};
}


#endif /* GEOMETRIC_CULLING_H_ */
