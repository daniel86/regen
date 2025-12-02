#ifndef REGEN_BOID_SIMULATION_H
#define REGEN_BOID_SIMULATION_H

#include "regen/shader/shader-input.h"
#include "regen/shapes/bounds.h"
#include "regen/textures/texture.h"
#include "regen/objects/model-transformation.h"
#include "regen/textures/collision-map.h"

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

		explicit BoidSimulation(
			const ref_ptr<ShaderInput4f> &modelOffset,
			const ref_ptr<ShaderInput3f> &modelDirection = {});

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
		 * Set the attraction range of the boids, i.e. how far they are attracted to the neighbors.
		 * @param range the attraction range.
		 */
		void setAttractionRange(float range);

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

		/**
		 * Set the collision map, which is optional.
		 * @param mapCenter the center.
		 * @param mapSize the size.
		 * @param collisionMap the collision map.
		 */
		void setCollisionMap(
				const Vec3f &mapCenter,
				const Vec2f &mapSize,
				const ref_ptr<Texture2D> &collisionMap,
				CollisionMapType mapType);

		Vec3f getCellCenter(const Vec3i &gridIndex) const;

		Vec3f getCellCenter(const Vec3ui &gridIndex) const;

		static inline uint32_t getGridIndex(const Vec3i &v, const Vec3ui &gridSize) {
			return v.x + v.y * gridSize.x + v.z * gridSize.x * gridSize.y;
		}

		static inline int32_t getGridIndex(const Vec3i &v, const Vec3i &gridSize) {
			return v.x + v.y * gridSize.x + v.z * gridSize.x * gridSize.y;
		}

		virtual void initBoidSimulation() {};

	protected:
		uint32_t numBoids_;
		ref_ptr<ModelTransformation> tf_;
		ref_ptr<ShaderInput4f> modelOffset_;
		ref_ptr<ShaderInput3f> modelDirection_;
		// The bounding box of the boids.
		Bounds<Vec3f> boidBounds_ = Bounds<Vec3f>::create(Vec3f::zero(), Vec3f::one());

		// height map
		ref_ptr<Texture2D> heightMap_;
		float heightMapFactor_ = 1.0f;
		Vec3f mapCenter_ = Vec3f::zero();
		Vec2f mapSize_ = Vec2f::create(10.0f);

		// collision map
		ref_ptr<Texture2D> collisionMap_;
		Vec3f collisionMapCenter_ = Vec3f::zero();
		Vec2f collisionMapSize_ = Vec2f::create(10.0f);
		CollisionMapType collisionMapType_ = COLLISION_SCALAR;

		ref_ptr<ShaderInput3ui> gridSize_;
		ref_ptr<ShaderInput1f> cellSize_;
		Bounds<Vec3f> gridBounds_ = Bounds<Vec3f>::create(Vec3f::zero(), Vec3f::zero());
		unsigned int numCells_ = 0;

		ref_ptr<ShaderInput1ui> maxNumNeighbors_;
		ref_ptr<ShaderInput1f> visualRange_;
		ref_ptr<ShaderInput1f> attractionRange_;
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

	std::ostream &operator<<(std::ostream &out, const BoidSimulation::ObjectType &v);

	std::istream &operator>>(std::istream &in, BoidSimulation::ObjectType &v);
}

#endif //REGEN_BOID_SIMULATION_H
