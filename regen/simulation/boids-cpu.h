#ifndef REGEN_BOIDS_CPU_H
#define REGEN_BOIDS_CPU_H

#include "boid-simulation.h"
#include "../animation/animation.h"
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

		/**
		 * Model offset constructor.
		 * @param modelOffset A shader input with model offsets, each instance of the model will be a boid.
		 */
		explicit BoidsCPU(const ref_ptr<ShaderInput4f> &modelOffset);

		~BoidsCPU() override;

		BoidsCPU(const BoidsCPU &) = delete;

		static ref_ptr<BoidsCPU> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf);

		// Animation interface
		void cpuUpdate(double dt) override;

		void initBoidSimulation() override;

	protected:
		struct Private;
		Private *priv_;

		struct BoidData {
			std::vector<int32_t> neighbors; // size = maxNumNeighbors
			uint32_t numNeighbors = 0;
			Vec3f force;
			//Vec3f sumPos, sumVel, sumSep;
		};
		std::vector<BoidData> boidData_;   // size = numBoids_

		inline void setBoidPosition(uint32_t boidIndex, const Vec3f &pos);

		inline Vec3f getBoidPosition(uint32_t boidIndex) const;

		inline void setBoidVelocity(uint32_t boidIndex, const Vec3f &vel);

		inline Vec3f getBoidVelocity(uint32_t boidIndex) const;

		inline Quaternion getBoidOrientation(uint32_t boidIndex) const;

		inline void setBoidOrientation(
				uint32_t boidIndex, const Quaternion &orientation);

		void updateTransforms();

		void simulateBoids(float dt);

		void simulateBoid(int32_t boidIdx, float dt);

		Vec3f accumulateForce(
				BoidData &boid,
				const Vec3f &boidPos,
				const Vec3f &boidVel);

		Vec3f limitVelocity(
				const Vec3f &lastDir,
				const Vec3f &boidVel,
				float &boidSpeed);

		void homesickness(
				const Vec3f &boidPos, Vec3f &boidForce);

		bool avoidCollisions(
				const Vec3f &boidPos,
				const Vec3f &boidVel,
				Vec3f &boidForce,
				float dt);

		bool avoidDanger(const Vec3f &boidPos, Vec3f &boidForce);

		void attract(const Vec3f &boidPos, Vec3f &boidForce);

		void updateCellIndex();

		void updateNeighbours(int32_t boidIdx, const int32_t *neighborIndices, uint32_t neighborCount);

		void clearGrid();

		void updateGrid();

		inline Vec3i getGridIndex3D(const Vec3f &boid) const;
	};

	std::ostream &operator<<(std::ostream &out, const BoidSimulation::ObjectType &v);

	std::istream &operator>>(std::istream &in, BoidSimulation::ObjectType &v);
}

#endif //REGEN_BOIDS_CPU_H
