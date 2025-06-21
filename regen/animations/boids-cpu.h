#ifndef REGEN_BOIDS_CPU_H
#define REGEN_BOIDS_CPU_H

#include "boid-simulation.h"
#include "animation.h"
#include "regen/math/simd.h"

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

		~BoidsCPU() override;

		BoidsCPU(const BoidsCPU &) = delete;

		static ref_ptr<BoidsCPU> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf);

		// Animation interface
		void animate(double dt) override;

		void initBoidSimulation() override;

	protected:
		struct Private;
		Private *priv_;

		struct BoidData {
			Vec3f force;
			Vec3f velocity;
			std::vector<int32_t> neighbors; // size = maxNumNeighbors
			uint32_t numNeighbors = 0;
		};
		std::vector<BoidData> boidData_;   // size = numBoids_
		// Note: SoA data layout for SIMD-friendly processing
		// Note: vectorSIMD is used to ensure 32-bit alignment which is good for SIMD operations.
		vectorSIMD<float> boidPositionsX_;    // size = numBoids_
		vectorSIMD<float> boidPositionsY_;    // size = numBoids_
		vectorSIMD<float> boidPositionsZ_;    // size = numBoids_
		vectorSIMD<int32_t> boidGridIndices_; // size = numBoids_
		//vectorSIMD<int32_t> boidGridIndicesX_; // size = numBoids_
		//vectorSIMD<int32_t> boidGridIndicesY_; // size = numBoids_
		//vectorSIMD<int32_t> boidGridIndicesZ_; // size = numBoids_

		inline void setBoidPosition(uint32_t boidIndex, const Vec3f &pos);

		inline Vec3f getBoidPosition(uint32_t boidIndex) const;

		void updateTransforms();

		void simulateBoids(float dt);

		void simulateBoid(int32_t boidIdx, float dt);

		void limitVelocity(BoidData &boid, const Vec3f &lastDir);

		void homesickness(BoidData &boid, const Vec3f &boidPos);

		bool avoidCollisions(BoidData &boid, const Vec3f &boidPos, float dt);

		bool avoidDanger(BoidData &boid, const Vec3f &boidPos);

		void attract(BoidData &boid, const Vec3f &boidPos);

		void updateNeighbours(BoidData &boid, const Vec3f &boidPos, int32_t boidIndex,
				const vectorSIMD<int32_t> &neighborIndices, uint32_t neighborCount);

		void clearGrid();

		void updateGrid();

		inline Vec3i getGridIndex3D(const Vec3f &boid) const;
	};

	std::ostream &operator<<(std::ostream &out, const BoidSimulation::ObjectType &v);

	std::istream &operator>>(std::istream &in, BoidSimulation::ObjectType &v);
}

#endif //REGEN_BOIDS_CPU_H
