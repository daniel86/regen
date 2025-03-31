#ifndef REGEN_BOID_SIMULATION_H
#define REGEN_BOID_SIMULATION_H

#include "animation.h"
#include "regen/states/model-transformation.h"
#include "regen/shapes/bounds.h"
#include "regen/shapes/quad-tree.h"
#include "regen/scene/scene-input.h"
#include "regen/scene/scene-loader.h"
#include "regen/gl-types/ssbo.h"
#include "regen/gl-types/pbo.h"
#include "regen/gl-types/bbox-buffer.h"
#include "regen/states/compute-pass.h"
#include <regen/textures/texture-2d.h>

namespace regen {
	/**
	 * \brief Boids simulation.
	 * The boids simulation is a simple flocking simulation.
	 */
	class BoidSimulation {
	public:
		enum ObjectType {
			ATTRACTOR,
			DANGER
		};

		explicit BoidSimulation(const ref_ptr<ModelTransformation> &tf);

		explicit BoidSimulation(const ref_ptr<ShaderInput3f> &position);

		/**
		 * Load the boids settings from a scene input node.
		 * @param parser the scene parser.
		 * @param node the scene input node.
		 */
		void loadSettings(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Set the visual range of the boids, i.e. how far they can see the neighbors.
		 * @param range the visual range.
		 */
		void setVisualRange(float range);

		/**
		 * Set the look ahead factor of the boids.
		 * @param factor the look ahead factor.
		 */
		void setLookAheadDistance(float distance) { lookAheadDistance_->setVertex(0, distance); }

		/**
		 * Set the repulsion factor of the boids.
		 * @param factor the repulsion factor.
		 */
		void setRepulsionFactor(float factor) { repulsionFactor_->setVertex(0, factor); }

		/**
		 * Set the maximum number of neighbors a boid can have.
		 * @param num the maximum number of neighbors.
		 */
		void setMaxNumNeighbors(unsigned int num) { maxNumNeighbors_->setVertex(0, num); }

		/**
		 * Set the maximum speed of the boids.
		 * @param speed the maximum speed.
		 */
		void setMaxBoidSpeed(float speed) { maxBoidSpeed_->setVertex(0, speed); }

		/**
		 * Set the maximum angular speed of the boids.
		 * @param speed the maximum angular speed.
		 */
		void setMaxAngularSpeed(float speed) { maxAngularSpeed_->setVertex(0, speed); }

		/**
		 * Set the coherence weight of the boids.
		 * @param weight the coherence weight.
		 */
		void setCoherenceWeight(float weight) { coherenceWeight_->setVertex(0, weight); }

		/**
		 * Set the alignment weight of the boids.
		 * @param weight the alignment weight.
		 */
		void setAlignmentWeight(float weight) { alignmentWeight_->setVertex(0, weight); }

		/**
		 * Set the separation weight of the boids.
		 * @param weight the separation weight.
		 */
		void setSeparationWeight(float weight) { separationWeight_->setVertex(0, weight); }

		/**
		 * Set the avoidance weight of the boids.
		 * @param weight the avoidance weight.
		 */
		void setAvoidanceWeight(float weight) { avoidanceWeight_->setVertex(0, weight); }

		/**
		 * Set the avoidance distance of the boids.
		 * @param distance the avoidance distance.
		 */
		void setAvoidanceDistance(float distance) { avoidanceDistance_->setVertex(0, distance); }

		/**
		 * Set the bounds of the boids simulation.
		 * @param bounds the bounds.
		 */
		void setSimulationBounds(const Bounds<Vec3f> &bounds);

		/**
		 * Set the base orientation of the boids.
		 * @param orientation the base orientation.
		 */
		void setBaseOrientation(float orientation);

		/**
		 * Set the home base of the boids.
		 * @param homeBase the home base.
		 */
		void addHomePoint(const Vec3f &homePoint);

		/**
		 * Add an attractor to the boids.
		 * @param tf the attractor.
		 */
		void addObject(ObjectType objectType,
					   const ref_ptr<ShaderInputMat4> &tf,
					   const ref_ptr<ShaderInput3f> &offset);

		/**
		 * Set the boid map, which is optional.
		 * @param mapCenter the center.
		 * @param mapSize the size.
		 * @param heightMap the height map.
		 * @param heightMapFactor the height map factor, i.e. its max height.
		 */
		void setMap(
				const Vec3f &mapCenter,
				const Vec2f &mapSize,
				const ref_ptr<Texture2D> &heightMap,
				float heightMapFactor);

		Vec3f getCellCenter(const Vec3i &gridIndex) const;

		static int getGridIndex(const Vec3i &v, const Vec3i &gridSize);

		virtual void initBoidSimulation() {};

	protected:
		int numBoids_;
		ref_ptr<ModelTransformation> tf_;
		ref_ptr<ShaderInput3f> position_;
		// The bounding box of the boids.
		Bounds<Vec3f> boidBounds_ = Bounds<Vec3f>(0.0f, 1.0f);
		Bounds<Vec3f> newBounds_ = Bounds<Vec3f>(0.0f, 0.0f);

		// height map
		ref_ptr<Texture2D> heightMap_;
		float heightMapFactor_ = 1.0f;
		Vec3f mapCenter_ = Vec3f::zero();
		Vec2f mapSize_ = Vec2f(10.0f);

		ref_ptr<ShaderInput3i> gridSize_;
		ref_ptr<ShaderInput1f> cellSize_;
		Bounds<Vec3f> gridBounds_ = Bounds<Vec3f>(0.0f, 0.0f);
		unsigned int numCells_ = 0;

		ref_ptr<ShaderInput1ui> maxNumNeighbors_;
		ref_ptr<ShaderInput1f> visualRange_;
		ref_ptr<ShaderInput1f> maxBoidSpeed_;
		ref_ptr<ShaderInput1f> maxAngularSpeed_;
		ref_ptr<ShaderInput1f> coherenceWeight_;
		ref_ptr<ShaderInput1f> alignmentWeight_;
		ref_ptr<ShaderInput1f> separationWeight_;
		ref_ptr<ShaderInput1f> avoidanceWeight_;
		ref_ptr<ShaderInput1f> avoidanceDistance_;
		ref_ptr<ShaderInput1f> lookAheadDistance_;
		ref_ptr<ShaderInput1f> repulsionFactor_;
		// The bounding box of the simulation (boids may slip outside).
		ref_ptr<ShaderInput3f> simulationBoundsMin_;
		ref_ptr<ShaderInput3f> simulationBoundsMax_;
		ref_ptr<ShaderInput3f> boidsScale_;
		ref_ptr<ShaderInput1f> baseOrientation_;

		std::vector<Vec3f> homePoints_;
		struct SimulationEntity {
			ref_ptr<ShaderInputMat4> tf;
			ref_ptr<ShaderInput3f> pos;
		};
		std::vector<SimulationEntity> attractors_;
		std::vector<SimulationEntity> dangers_;

		void initBoidSimulation0();

		void updateGridSize();

		static Vec2f computeUV(const Vec3f &boidPosition, const Vec3f &mapCenter, const Vec2f &mapSize);
	};

	/**
	 * \brief GPU Boid simulation.
	 * The boids simulation is a simple flocking simulation.
	 * The simulation is done on the GPU using compute shaders.
	 */
	class BoidSimulation_GPU : public BoidSimulation, public Animation {
	public:
		/**
		 * TF constructor.
		 * @param tf A model transformation, each instance of the model will be a boid.
		 */
		explicit BoidSimulation_GPU(const ref_ptr<ModelTransformation> &tf);

		/**
		 * Position constructor.
		 * @param position A shader input with the boid positions.
		 */
		explicit BoidSimulation_GPU(const ref_ptr<ShaderInput3f> &position);

		~BoidSimulation_GPU() override = default;

		static ref_ptr<BoidSimulation_GPU> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ShaderInput3f> &position);

		static ref_ptr<BoidSimulation_GPU> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf);

		// override Animation
		void glAnimate(RenderState *rs, GLdouble dt) override;

		void initBoidSimulation() override;

	protected:
		// Grid attributes
		ref_ptr<ShaderInput1i> listHeads_;
		ref_ptr<ShaderInput3f> gridMin_;

		// Boid SSBOs & PBOs & UBOs
		ref_ptr<SSBO> tfBuffer_;
		ref_ptr<SSBO> velBuffer_;
		ref_ptr<SSBO> listHeadBuffer_;
		ref_ptr<SSBO> listBodyBuffer_;
		ref_ptr<BBoxBuffer> bboxBuffer_;
		ref_ptr<UBO> simulationUBO_;
		ref_ptr<UBO> gridUBO_;

		// Update states
		ref_ptr<State> updateGridState_;
		ref_ptr<State> updateBoidsState_;
		ref_ptr<ComputePass> computeGridState_;
		ref_ptr<ComputePass> computeBoidState_;

		double time_ = 0.0;
		unsigned int vrStamp_ = 0;

		void initBuffers();

		void initAnimationState();

		void createShader(const ref_ptr<ComputePass> &pass, const ref_ptr<State> &update);

		void updateGrid();

		void simulate(RenderState *rs, double dt);

		void computeBBox(const Vec3f *posData);
	};

	/**
	 * \brief CPU Boid simulation.
	 * A spatial grid is used to speed up the simulation.
	 * For a couple of thousand boids this should be fine.
	 * For a massive number of boids a GPU implementation is recommended.
	 */
	class BoidSimulation_CPU : public BoidSimulation, public Animation {
	public:
		/**
		 * TF constructor.
		 * @param tf A model transformation, each instance of the model will be a boid.
		 */
		explicit BoidSimulation_CPU(const ref_ptr<ModelTransformation> &tf);

		/**
		 * Position constructor.
		 * @param position A shader input with the boid positions.
		 */
		explicit BoidSimulation_CPU(const ref_ptr<ShaderInput3f> &position);

		~BoidSimulation_CPU() override;

		BoidSimulation_CPU(const BoidSimulation_CPU &) = delete;

		static ref_ptr<BoidSimulation_CPU> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ShaderInput3f> &position);

		static ref_ptr<BoidSimulation_CPU> load(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<ModelTransformation> &tf);

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
			std::vector<int> neighbors;
		};
		std::vector<BoidData> boidData_;
		std::vector<Vec3f> boidPositions_;

		void updateTransforms();

		void simulateBoids(float dt);

		void simulateBoid(BoidData &boid, Vec3f &boidPos, float dt);

		void limitVelocity(BoidData &boid, const Vec3f &lastDir);

		void homesickness(BoidData &boid, const Vec3f &boidPos);

		bool avoidCollisions(BoidData &boid, const Vec3f &boidPos, float dt);

		bool avoidDanger(BoidData &boid, const Vec3f &boidPos);

		void attract(BoidData &boid, const Vec3f &boidPos);

		void updateNeighbours0(BoidData &boid, const Vec3f &boidPos, int boidIndex);

		void updateNeighbours1(BoidData &boid, const Vec3f &boidPos, int boidIndex, const Vec3i &gridIndex);

		void updateNeighbours2(BoidData &boid, const Vec3f &boidPos, int boidIndex, const Vec3i &gridIndex);

		void updateGrid();

		Vec3i getGridIndex3D(const Vec3f &boid) const;
	};

	std::ostream &operator<<(std::ostream &out, const BoidSimulation::ObjectType &v);

	std::istream &operator>>(std::istream &in, BoidSimulation::ObjectType &v);
}

#endif //REGEN_BOID_SIMULATION_H
