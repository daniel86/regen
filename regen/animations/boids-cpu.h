#ifndef REGEN_BOIDS_CPU_H
#define REGEN_BOIDS_CPU_H

#include "boid-simulation.h"
#include "animation.h"
#include "regen/math/vector-batch.h"

#define REGEN_BOID_USE_SSE

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
			Vec3i gridIndex = Vec3i::zero();
			std::vector<uint32_t> neighbors;
		};
		std::vector<BoidData> boidData_;
#ifdef REGEN_BOID_USE_SSE
		Vec3fBatch4 batchPositions_;
		std::vector<float> boidPositionsX_;
		std::vector<float> boidPositionsY_;
		std::vector<float> boidPositionsZ_;
#else
		std::vector<Vec3f> boidPositions_;
#endif

		void updateTransforms();

		void simulateBoids(float dt);

		void simulateBoid(uint32_t boidIdx, float dt);

		void limitVelocity(BoidData &boid, const Vec3f &lastDir);

		void homesickness(BoidData &boid, const Vec3f &boidPos);

		bool avoidCollisions(BoidData &boid, const Vec3f &boidPos, float dt);

		bool avoidDanger(BoidData &boid, const Vec3f &boidPos);

		void attract(BoidData &boid, const Vec3f &boidPos);

		void updateNeighbours0(BoidData &boid, const Vec3f &boidPos, uint32_t boidIndex);

		void updateNeighbours1(BoidData &boid, const Vec3f &boidPos, uint32_t boidIndex, const Vec3i &gridIndex);

		void updateNeighbours2(BoidData &boid, const Vec3f &boidPos, uint32_t boidIndex, const Vec3i &gridIndex);

		void updateGrid();

		Vec3i getGridIndex3D(const Vec3f &boid) const;
	};

	std::ostream &operator<<(std::ostream &out, const BoidSimulation::ObjectType &v);

	std::istream &operator>>(std::istream &in, BoidSimulation::ObjectType &v);
}

#endif //REGEN_BOIDS_CPU_H
