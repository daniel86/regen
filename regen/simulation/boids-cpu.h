#ifndef REGEN_BOIDS_CPU_H
#define REGEN_BOIDS_CPU_H

#include "boid-simulation.h"
#include "regen/animation/animation.h"
#include "regen/compute/simd.h"

namespace regen {
	/**
	 * \brief CPU Boid simulation.
	 * A spatial grid is used to speed up the simulation.
	 * For a couple of thousand boids this should be fine.
	 * For a massive number of boids a GPU implementation is recommended.
	 */
	class BoidsCPU : public BoidSimulation, public Animation {
	public:
		/**
		 * TF constructor.
		 * @param tf A model transformation, each instance of the model will be a boid.
		 */
		explicit BoidsCPU(const ref_ptr<ModelTransformation> &tf);

		/**
		 * Model offset constructor.
		 * @param modelOffset A shader input with model offsets, each instance of the model will be a boid.
		 * @param modelDirection Optional shader input with model directions.
		 */
		explicit BoidsCPU(const ref_ptr<ShaderInput4f> &modelOffset,
			const ref_ptr<ShaderInput3f> &modelDirection = {});

		~BoidsCPU() override;

		BoidsCPU(const BoidsCPU &) = delete;

		static ref_ptr<BoidsCPU> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf);

		// Animation interface
		void cpuUpdate(double dt) override;

		void initBoidSimulation() override;

	protected:
		struct Private;
		Private *priv_;

		inline void setBoidPosition(uint32_t boidIndex, const Vec3f &pos);

		inline Vec3f getBoidPosition(uint32_t boidIndex) const;

		inline void setBoidVelocity(uint32_t boidIndex, const Vec3f &vel);

		inline Vec3f getBoidVelocity(uint32_t boidIndex) const;

		inline void setBoidOrientation(uint32_t boidIndex, const Quaternion &orientation);

		void updateTransforms();

		void advanceBoid(uint32_t boidIdx, float dt);

		Vec3f limitVelocity(
				const Vec3f &lastDir,
				const Vec3f &boidVel,
				float &boidSpeed) const;

		void homesickness(const Vec3f &boidPos, Vec3f &boidForce) const;

		bool avoidCollisions(
				const Vec3f &boidPos,
				const Vec3f &boidVel,
				Vec3f &boidForce,
				float dt) const;

		bool avoidDanger(const Vec3f &boidPos, Vec3f &boidForce) const;

		void attract(const Vec3f &boidPos, Vec3f &boidForce) const;

		void updateCellIndex();

		void clearGrid();

		void insertIntoCells();

		friend struct BoidSliceData;
	};
}

#endif //REGEN_BOIDS_CPU_H
