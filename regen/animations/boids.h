/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef REGEN_BOIDS_H
#define REGEN_BOIDS_H

#include "animation.h"
#include "regen/states/model-transformation.h"
#include "regen/shapes/bounds.h"
#include "regen/shapes/quad-tree.h"
#include "regen/scene/scene-input.h"
#include "regen/scene/scene-parser.h"

namespace regen {
	/**
	 * \brief Boids simulation.
	 * The boids simulation is a simple flocking simulation.
	 * This is a CPU implementation, each frame the boid data will be copied to the GPU
	 * which can be too much for huge number of boids.
	 * But for a few hundred or a couple of thousand boids it should be fine.
	 * For massive number of boids a GPU implementation is recommended.
	 */
	class BoidsSimulation_CPU : public Animation {
	public:
		/**
		 * TF constructor.
		 * @param tf A model transformation, each instance of the model will be a boid.
		 */
		explicit BoidsSimulation_CPU(const ref_ptr<ModelTransformation> &tf);

		/**
		 * Position constructor.
		 * @param position A shader input with the boid positions.
		 */
		explicit BoidsSimulation_CPU(const ref_ptr<ShaderInput3f> &position);

		virtual ~BoidsSimulation_CPU() = default;

		/**
		 * Load the boids settings from a scene input node.
		 * @param parser the scene parser.
		 * @param node the scene input node.
		 */
		void loadSettings(scene::SceneParser *parser, const ref_ptr<scene::SceneInputNode> &node);

		/**
		 * Set the base orientation of the boids.
		 * @param orientation the base orientation.
		 */
		void setBaseOrientation(float orientation) { baseOrientation_ = orientation; }

		/**
		 * Set the bounds of the boids simulation.
		 * No boid will leave the bounds.
		 * @param bounds the bounds.
		 */
		void setBounds(const Bounds<Vec3f> &bounds);

		/**
		 * Set the home base of the boids.
		 * @param homeBase the home base.
		 */
		void addHomePoint(const Vec3f &homePoint) { homePoints_.push_back(homePoint); }

		/**
		 * Add an attractor to the boids.
		 * @param tf the attractor.
		 */
		void addAttractor(const ref_ptr<ShaderInputMat4> &tf);

		/**
		 * Add an attractor to the boids.
		 * @param pos the attractor.
		 */
		void addAttractor(const ref_ptr<ShaderInput3f> &pos);

		/**
		 * Add a danger to the boids.
		 * @param tf the danger.
		 */
		void addDanger(const ref_ptr<ShaderInputMat4> &tf);

		/**
		 * Add a danger to the boids.
		 * @param pos the danger.
		 */
		void addDanger(const ref_ptr<ShaderInput3f> &pos);

		/**
		 * Set the visual range of the boids, i.e. how far they can see the neighbors.
		 * @param range the visual range.
		 */
		void setVisualRange(float range) { visualRange_->setVertex(0, range); }

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

		// Animation interface
		void animate(double dt) override;

	protected:
		ref_ptr<ModelTransformation> tf_;
		ref_ptr<ShaderInput3f> position_;
		float baseOrientation_ = 0.0f;
		Bounds<Vec3f> bounds_;
		Vec3f boidsScale_;
		std::vector<Vec3f> homePoints_;
		struct SimulationEntity {
			ref_ptr<ShaderInputMat4> tf;
			ref_ptr<ShaderInput3f> pos;
		};
		std::vector<SimulationEntity> attractors_;
		std::vector<SimulationEntity> dangers_;

		ref_ptr<Texture2D> heightMap_;
		ref_ptr<Texture2D> normalMap_;
		float heightMapFactor_ = 1.0f;
		Vec3f mapCenter_ = Vec3f::zero();
		Vec2f mapSize_ = Vec2f(10.0f);

		// boid simulation parameters
		ref_ptr<ShaderInput1ui> maxNumNeighbors_;
		ref_ptr<ShaderInput1f> maxBoidSpeed_;
		ref_ptr<ShaderInput1f> maxAngularSpeed_;
		ref_ptr<ShaderInput1f> coherenceWeight_;
		ref_ptr<ShaderInput1f> alignmentWeight_;
		ref_ptr<ShaderInput1f> separationWeight_;
		ref_ptr<ShaderInput1f> avoidanceWeight_;
		ref_ptr<ShaderInput1f> avoidanceDistance_;
		ref_ptr<ShaderInput1f> visualRange_;
		ref_ptr<ShaderInput1f> lookAheadDistance_;
		ref_ptr<ShaderInput1f> repulsionFactor_;

		struct BoidData {
			//ref_ptr<BoundingSphere> shape;
			Vec3f position;
			Vec3f force;
			Vec3f velocity;
			Vec3f direction = Vec3f::front(); // normalized velocity
			std::vector<const BoidData *> neighbors;
		};
		std::vector<BoidData> boidData_;

		Vec3f avgPosition_ = Vec3f::zero();
		Vec3f avgVelocity_ = Vec3f::zero();
		Vec3f separation_ = Vec3f::zero();
		Vec3f boidDirection_ = Vec3f::front();
		Quaternion boidRotation_;

		void initBoidSimulation(unsigned int);

		void simulateBoids(float dt);

		void simulateBoid(BoidData &boid, float dt);

		static Vec2f computeUV(const Vec3f &boidPosition, const Vec3f &mapCenter, const Vec2f &mapSize);

		// simulation restrictions
		void limitVelocity(BoidData &boid);

		void homesickness(BoidData &boid);

		bool avoidCollisions(BoidData &boid, float dt);

		bool avoidDanger(BoidData &boid);

		void attract(BoidData &boid);
	};
}

#endif //REGEN_BOIDS_H
