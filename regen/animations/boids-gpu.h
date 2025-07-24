#ifndef REGEN_BOIDS_GPU_H
#define REGEN_BOIDS_GPU_H

#include "boid-simulation.h"
#include "animation.h"
#include "regen/buffer/bbox-buffer.h"
#include "regen/states/compute-pass.h"
#include "regen/gl-types/queries/elapsed-time.h"

namespace regen {
	/**
	 * \brief GPU Boid simulation.
	 * The boids simulation is a simple flocking simulation.
	 * The simulation is done on the GPU using compute shaders.
	 */
	class BoidsGPU : public BoidSimulation, public Animation {
	public:
		/**
		 * TF constructor.
		 * @param tf A model transformation, each instance of the model will be a boid.
		 */
		explicit BoidsGPU(const ref_ptr<ModelTransformation> &tf);

		~BoidsGPU() override = default;

		static ref_ptr<BoidsGPU> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf);

		// override Animation
		void glAnimate(RenderState *rs, GLdouble dt) override;

		void initBoidSimulation() override;

	protected:
		struct BoidData {
			Vec4f pos;
			Vec4f vel;
		};
		// Boid SSBOs & PBOs & UBOs
		ref_ptr<SSBO> tfBuffer_;         // input TF data
		ref_ptr<SSBO> velBuffer_;        // velocity data buffer
		ref_ptr<SSBO> gridOffsetBuffer_; // offsets of grid cells into data buffer
		ref_ptr<SSBO> boidDataBuffer_;   // optional: sorted boid data
		ref_ptr<UBO> gridUBO_;
		ref_ptr<ShaderInput1ui> u_numCells_;
		ref_ptr<ShaderInput3f> gridMin_;

		// Update states
		ref_ptr<State> simulationState_;
		ref_ptr<State> updateGridState_;
		ref_ptr<ComputePass> gridResetPass_;
		ref_ptr<State> radixSort_;
		// Bounding box of boids for grid computation
		ref_ptr<BBoxBuffer> bboxBuffer_;
		ref_ptr<ComputePass> bboxPass_;

		double time_ = 1.0;
		double bbox_time_ = 0.0;
		unsigned int vrStamp_ = 0;
		int32_t simulationTimeLoc_ = -1;
		ref_ptr<TimeElapsedQuery> timeElapsedQuery_;

		uint32_t radixBits_ = 8u;
		uint32_t simulationGroupSize_ = 256u;
		uint32_t scanGroupSize_ = 512u;
		uint32_t sortGroupSize_ = 256u;

		void createResource();

		void createShader(const ref_ptr<ComputePass> &pass, const ref_ptr<State> &update);

		void updateGrid();

		void simulate(RenderState *rs, double dt);

		void computeBBox(const Vec3f *posData);

		void printOffsets(RenderState *rs);

		void debugGridSorting(RenderState *rs);

		void debugVelocity(RenderState *rs);
	};
}

#endif //REGEN_GPU_BOIDS_H
