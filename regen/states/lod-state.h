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
#include "compute-pass.h"
#include "regen/gl-types/pbo.h"

namespace regen {
	/**
	 * @brief Dynamic LOD handling and culling
	 */
	class LODState : public StateNode {
	public:
		/**
		 * @param camera The camera
		 * @param spatialIndex The spatial index
		 * @param shapeName The shape name
		 */
		LODState(
				const ref_ptr<Camera> &camera,
				const ref_ptr<SpatialIndex> &spatialIndex,
				std::string_view shapeName);

		/**
		 * @brief Constructor for LODState
		 * @param camera The camera
		 * @param meshVector The mesh vector
		 * @param tf The model transformation
		 */
		LODState(
				const ref_ptr<Camera> &camera,
				const std::vector<ref_ptr<Mesh>> &meshVector,
				const ref_ptr<ModelTransformation> &tf);

		~LODState() override = default;

		/**
		 * @brief Set the instance sorting
		 * @param instanceSorting The instance sorting
		 */
		void setInstanceSortMode(SortMode mode) { instanceSortMode_ = mode; }

		/**
		 * @brief Set the LOD thresholds
		 * @param thresholds The LOD thresholds
		 */
		void setThresholds(const Vec3f &thresholds);

		// override
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
		ref_ptr<ShaderInput3f> lodThresholds_;
		// temporary storage for instanceIDs, used to fill the instanceIDMap_
		std::vector<std::vector<GLuint>> lodGroups_;
		ref_ptr<Mesh> mesh_;
		std::vector<ref_ptr<Mesh>> meshVector_;
		ref_ptr<ModelTransformation> tf_;

		// GPU LOD update
		ref_ptr<ComputePass> radixSort_;
		ref_ptr<ComputePass> radixMerge_;
		ref_ptr<ShaderInput1ui> mergeSegmentSize_;
		uint32_t radixMergeReadBinding_ = 0u;
		uint32_t radixMergeWriteBinding_ = 0u;
		ref_ptr<SSBO> keyBuffer_;
		ref_ptr<SSBO> tmpIDBuffer_;
		ref_ptr<SSBO> workGroupBuffer_;
		// includes array data: lodGroupSize
		ref_ptr<SSBO> lodGroupSizeBuffer_;
		ref_ptr<ShaderInput1ui> lodGroupSize_;
		ref_ptr<PBO> lodGroupSizePBO_;
		Vec4ui *m_lodGroupSize_ = nullptr;

		void createInstanceBuffer();

		void createComputeShader();

		void updateMeshLOD();

		void activateLOD(uint32_t lodLevel);

		void computeLODGroups();

		void traverseInstanced_(RenderState *rs, unsigned int numVisible);

		void traverseCPU(RenderState *rs);

		void traverseGPU(RenderState *rs);

		void radixSortGPU(RenderState *rs);

		void debugGPU(RenderState *rs, bool debugFinalBuffer);

		void computeLODGroups_(
			const uint32_t *mappedData,
			int begin,
			int end,
			int increment);
	};
}


#endif /* GEOMETRIC_CULLING_H_ */
