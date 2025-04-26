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
#include "regen/gl-types/buffer-mapping.h"

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

		/**
		 * Update buffers needed for LOD computation.
		 */
		void createBuffers();

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
		// temporary storage for instanceIDs, used to fill the instanceIDMap_
		std::vector<std::vector<GLuint>> lodGroups_;
		ref_ptr<Mesh> mesh_;
		std::vector<ref_ptr<Mesh>> meshVector_;
		ref_ptr<ModelTransformation> tf_;

		// GPU LOD update
		ref_ptr<ComputePass> radixCull_;
		ref_ptr<ComputePass> radixHistogramPass_;
		ref_ptr<State> radixOffsetsPass_;
		ref_ptr<ComputePass> radixGlobalOffsetsPass_;
		ref_ptr<ComputePass> radixLocaleOffsetsPass_;
		ref_ptr<ComputePass> radixDistributeOffsetsPass_;
		ref_ptr<ComputePass> radixScatterPass_;
		ref_ptr<UBO> cullUBO_;
		ref_ptr<UBO> frustumUBO_;
		ref_ptr<SSBO> keyBuffer_;
		ref_ptr<SSBO> valueBuffer_[2];
		ref_ptr<SSBO> globalHistogramBuffer_;
		ref_ptr<SSBO> lodGroupSizeBuffer_;
		ref_ptr<SSBO> blockSumsBuffer_;
		ref_ptr<SSBO> blockOffsetsBuffer_;
		ref_ptr<ShaderInput1ui> lodGroupSize_;
		ref_ptr<BufferStructMapping<Vec4ui>> lodGroupSizeMapping_;
		Vec4f frustumPlanes_[6];
		int32_t histogramReadIndex_ = 0u;
		int32_t histogramBitOffsetIndex_ = 0u;
		int32_t scatterReadIndex_ = 0u;
		int32_t scatterWriteIndex_ = 0u;
		int32_t scatterBitOffsetIndex_ = 0u;

		void initLODState();

		void createComputeShader();

		void updateMeshLOD();

		void activateLOD(uint32_t lodLevel);

		void computeLODGroups();

		void traverseInstanced_(RenderState *rs, unsigned int numVisible);

		void traverseCPU(RenderState *rs);

		void traverseGPU(RenderState *rs);

		void radixSortGPU(RenderState *rs);

		void computeLODGroups_(
			const uint32_t *mappedData,
			int begin,
			int end,
			int increment);

		void printInstanceMap(RenderState *rs);

		void printHistogram(RenderState *rs);
	};
}


#endif /* GEOMETRIC_CULLING_H_ */
