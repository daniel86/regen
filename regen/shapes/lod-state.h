#ifndef REGEN_LOD_STATE_H_
#define REGEN_LOD_STATE_H_

#include <regen/states/state-node.h>
#include <regen/shapes/spatial-index.h>
#include "regen/camera/sorting.h"
#include "regen/buffer/ssbo.h"
#include "regen/buffer/pbo.h"
#include "regen/buffer/staging-buffer.h"
#include "regen/states/compute-pass.h"
#include "regen/states/radix-sort.h"
#include "regen/shapes/cull-shape.h"
#include "regen/gl-types/draw-command.h"

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
		 * @brief Check if the GPU-based LOD path is used.
		 * @return True if the GPU-based LOD path is used, false otherwise
		 */
		inline bool useGPUPath() const { return !cullShape_->isIndexShape(); }

		/**
		 * @brief Check if the CPU-based LOD path is used.
		 * @return True if the CPU-based LOD path is used, false otherwise
		 */
		inline bool useCPUPath() const { return cullShape_->isIndexShape(); }

		/**
		 * @brief Get the number of render layers from the camera.
		 * @return The number of render layers
		 */
		uint32_t numDrawLayers() const { return camera_->numLayer(); }

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

		/**
		 * @return True if visible instance are compacted before sort.
		 */
		bool useCompaction() const { return useCompaction_; }

		/**
		 * @param useCompaction True to enable compaction of visible instances before sort.
		 */
		void setUseCompaction(bool useCompaction) { useCompaction_ = useCompaction; }

		/**
		 * @return The instance buffer used for LOD computation.
		 */
		const ref_ptr<SSBO> &instanceBuffer() const { return instanceBuffer_; }

		/**
		 * @return true if this state has indirect draw buffers for the mesh parts.
		 */
		bool hasIndirectDrawBuffers() const { return !indirectDrawBuffers_.empty(); }

		/**
		 * Get the indirect draw buffer for a specific mesh part.
		 * @param partIdx the index of the mesh part
		 * @return the indirect draw buffer for the mesh part
		 */
		ref_ptr<SSBO> getIndirectDrawBuffer(const ref_ptr<Mesh> &mesh) const;

		// override
		void enable(RenderState *rs) override;

	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<CullShape> cullShape_;
		ref_ptr<IndexedShape> shapeIndex_;
		SortMode instanceSortMode_ = SortMode::FRONT_TO_BACK;
		uint32_t cameraStamp_ = 0;
		uint32_t tfStamp_ = 0;

		uint32_t numLODs_ = 1;
		uint32_t fixedLOD_;
		ref_ptr<Mesh> mesh_;
		bool hasShadowTarget_;
		bool hasVisibleInstance_ = false;
		bool useCompaction_ = true;

		ref_ptr<ShaderInput> instanceData_;
		// stores sorted instanceIDs, first sort criteria is the LOD group, second distance to camera
		ref_ptr<SSBO> instanceBuffer_;

		// GPU LOD update
		ref_ptr<ComputePass> cullPass_;
		ref_ptr<ComputePass> copyIndirect_;
		ref_ptr<RadixSort_GPU> radixSort_;
		// buffer for indirect draw calls, one per mesh part
		// (parts have different index buffers, so we cannot use a single buffer for all parts)
		std::vector<ref_ptr<SSBO>> indirectDrawBuffers_;
		ref_ptr<SSBO> clearIndirectBuffer_;
		ref_ptr<SSBO> clearNumVisibleBuffer_;
		struct IndirectDrawData {
			// The current draw commands for the mesh part.
			// Only maintained in case of CPU-based LOD update.
			std::vector<DrawCommand> current; // size: 4 * numIndirectLayers()
			// The draw commands for the mesh part that are used to clear the indirect draw buffer
			// to zero before the LOD computation.
			std::vector<DrawCommand> clear; // size: 4 * numIndirectLayers()
		};
		std::vector<IndirectDrawData> indirectDrawData_;

		ref_ptr<Animation> lodAnim_;

		void initLODState();

		void createComputeShader();

		void createInstanceBuffer();

		void createIndirectDrawBuffers();

		ref_ptr<SSBO> createIndirectDrawBuffer(const std::vector<DrawCommand> &initialData);

		void updateVisibility(
				uint32_t layerIdx,
				uint32_t lodLevel,
				uint32_t numInstances,
				uint32_t instanceOffset);

		void resetVisibility();

		void traverseCPU();

		void traverseGPU(RenderState *rs);

		friend class InstanceUpdater;
	};

	/**
	 * @brief State to set the base LOD of a mesh
	 */
	class SetBaseLOD : public State {
	public:
		explicit SetBaseLOD(const ref_ptr<Mesh> &mesh) : mesh_(mesh) {}

		~SetBaseLOD() override = default;

		// Override enable
		void enable(RenderState *rs) override {
			State::enable(rs);
			mesh_->activateLOD(0);
		}

	protected:
		ref_ptr<Mesh> mesh_;
	};
}


#endif /* REGEN_LOD_STATE_H_ */
