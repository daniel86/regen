#ifndef REGEN_LOD_STATE_H_
#define REGEN_LOD_STATE_H_

#include <regen/states/state-node.h>
#include <regen/shapes/spatial-index.h>
#include "regen/camera/sorting.h"
#include "regen/gl-types/ssbo.h"
#include "compute-pass.h"
#include "regen/gl-types/pbo.h"
#include "regen/gl-types/buffer-mapping.h"
#include "radix-sort.h"
#include "regen/shapes/cull-shape.h"

namespace regen {
	/**
	 * @brief Dynamic LOD handling and culling
	 */
	class LODState : public State {
	public:
		/**
		 * @param camera The camera
		 * @param shape The cull shape used for LOD computation
		 */
		LODState(const ref_ptr<Camera> &camera, const ref_ptr<CullShape> &shape);

		~LODState() override = default;

		/**
		 * @brief Set the instance sorting
		 * @param instanceSorting The instance sorting
		 */
		void setInstanceSortMode(SortMode mode) { instanceSortMode_ = mode; }

		/**
		 * @brief Get the instance sorting mode
		 * @return The instance sorting mode
		 */
		SortMode instanceSortMode() const { return instanceSortMode_; }

		// override
		void enable(RenderState *rs) override;

	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<CullShape> cullShape_;
		ref_ptr<IndexedShape> shapeIndex_;
		SortMode instanceSortMode_ = SortMode::FRONT_TO_BACK;

		// stores how many instances are currently visible for each LOD level
		std::vector<uint32_t> lodNumInstances_;
		std::vector<uint32_t> lodBoundaries_;
		ref_ptr<Mesh> mesh_;
		bool hasShadowTarget_;

		// GPU LOD update
		ref_ptr<ComputePass> cullPass_;
		ref_ptr<RadixSort> radixSort_;
		ref_ptr<UBO> cullUBO_;
		ref_ptr<UBO> frustumUBO_;
		ref_ptr<SSBO> lodGroupSizeBuffer_;
		ref_ptr<ShaderInput1ui> lodGroupSize_;
		ref_ptr<BufferStructMapping<Vec4ui>> lodGroupSizeMapping_;
		Vec4f frustumPlanes_[6];

		void initLODState();

		void createComputeShader();

		void updateMeshLOD();

		void updateVisibility(uint32_t lodLevel, uint32_t numInstances, uint32_t instanceOffset);

		void resetVisibility();

		void computeLODGroups();

		void traverseCPU(RenderState *rs);

		void traverseGPU(RenderState *rs);
	};
}


#endif /* REGEN_LOD_STATE_H_ */
