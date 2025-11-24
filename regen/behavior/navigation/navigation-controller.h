#ifndef REGEN_NAVIGATION_CONTROLLER_H_
#define REGEN_NAVIGATION_CONTROLLER_H_

#include "../../animation/transform-animation.h"
#include "regen/behavior/perception/collision-monitor.h"
#include "regen/math/bezier.h"
#include "regen/states/model-transformation.h"
#include "regen/textures/height-map.h"
#include "regen/utility/indexed.h"

namespace regen {
	class NavigationController : public Animation, public CollisionMonitor {
	public:
		static constexpr uint32_t NO_NAVIGATION = 0;
		enum NavigationState {
			// Approaching a target position.
			APPROACHING = 1 << 0,
			// Aligning with a group.
			FLOCKING    = 1 << 1,
		};

		enum ApproachingMode {
			// Direct path to target.
			DIRECT_PATH,
			// Curved path to target using Bezier curves.
			CURVE_PATH
		};

		NavigationController(
			const Indexed<ref_ptr<ModelTransformation>> &tfIndexed,
			const ref_ptr<WorldModel> &world);

		~NavigationController() override = default;

		/**
		 * @param numEntries The number of entries in the Bezier arc length lookup table.
		 */
		void setNumBezierLUTEntries(uint32_t numEntries) {
			numLUTEntries_ = numEntries;
		}

		/**
		 * Set the walk speed.
		 * @param speed the speed.
		 */
		void setWalkSpeed(float speed) { walkSpeed_ = speed; }

		/**
		 * Set the run speed.
		 * @param speed the speed.
		 */
		void setRunSpeed(float speed) { runSpeed_ = speed; }

		/**
		 * Set the maximum turn speed in degrees per second.
		 * @param deg the degrees per second.
		 */
		void setMaxTurnDegPerSecond(float deg) {
			maxTurnDegPerSec_ = deg;
			maxTurn_ = maxTurnDegPerSec_ * (M_PI / 180.0f);
		}

		/**
		 * set the personal space for avoidance.
		 * @param space the personal space.
		 */
		void setPersonalSpace(float space) {
			personalSpace_ = space;
			personalSpaceSq_ = personalSpace_ * personalSpace_;
		}

		/**
		 * set the turn factor for navigation within personal space.
		 * @param v the turn factor.
		 */
		void setTurnFactorPersonalSpace(float v) { turnFactorPersonalSpace_ = v; }

		/**
		 * Set the weight for avoidance steering (0 = no avoidance, 1 = full avoidance).
		 * @param weight the weight.
		 */
		void setAvoidanceWeight(float weight) { collisionWeight_ = weight; }

		/**
		 * Set the distance where collisions are ignored when approaching goals.
		 * @param distance the distance.
		 */
		void setPushThroughDistance(float distance) { pushThroughDistance_ = distance; }

		/**
		 * Set the look ahead treshold for dynamic avoidance.
		 * When closer to the goal than this threshold, the look ahead distance
		 * will be reduced to allow better approach to the goal.
		 * @param threshold the threshold.
		 */
		void setLookAheadThreshold(float threshold) { lookAheadThreshold_ = threshold; }

		/**
		 * Set the decay factor for avoidance strength that influences how fast
		 * avoidance is blended out when no new perception frame arrives.
		 * @param decay the decay factor.
		 */
		void setAvoidanceDecay(float decay) { avoidanceDecay_ = decay; }

		/**
		 * Set the weight for wall avoidance steering.
		 * @param weight the weight.
		 */
		void setWallAvoidance(float weight) { wallAvoidance_ = weight; }

		/**
		 * Set the weight for character avoidance steering.
		 * @param weight the weight.
		 */
		void setCharacterAvoidance(float weight) { characterAvoidance_ = weight; }

		/**
		 * Set the weight for cohesion steering.
		 * @param weight the weight.
		 */
		void setCohesionWeight(float weight) { cohesionWeight_ = weight; }

		/**
		 * Set the weight for member separation steering.
		 * @param weight the weight.
		 */
		void setMemberSeparationWeight(float weight) { memberSeparationWeight_ = weight; }

		/**
		 * Set the weight for group separation steering.
		 * @param weight the weight.
		 */
		void setGroupSeparationWeight(float weight) { groupSeparationWeight_ = weight; }

		/**
		 * Set the weight for wall tangent steering (0 = no wall following, 1 = full wall following).
		 * @param weight the weight.
		 */
		void setWallTangentWeight(float weight) { wallTangentWeight_ = weight; }

		/**
		 * Set the weight for velocity orientation (0 = no orientation, 1 = full orientation).
		 * @param weight the weight.
		 */
		void setVelOrientationWeight(float weight) { velOrientationWeight_ = weight; }

		/**
		 * Set the base orientation of the mesh around y axis for looking into z direction.
		 * @param baseOrientation the base orientation.
		 */
		void setBaseOrientation(float baseOrientation) { baseOrientation_ = baseOrientation; }

		/**
		 * Set the floor height, only used if no height map is set.
		 * @param floorHeight the floor height.
		 */
		void setFloorHeight(float floorHeight) { floorHeight_ = floorHeight; }

		/**
		 * @return the look ahead distance for the collision shape.
		 */
		float lookAheadDistance() const { return lookAheadDistance_; }

		/**
		 * Set the height map.
		 * @param heightMap the height map.
		 */
		void setHeightMap(const ref_ptr<HeightMap> &heightMap);

		/**
		 * Start approaching a target position from a source position.
		 * @param source the source position.
		 * @param target the target position.
		 */
		void startApproaching(const Vec2f &source, const Vec2f &target);

		/**
		 * Start approaching a target position from a source position with a desired orientation.
		 * @param source the source position.
		 * @param target the target position.
		 * @param orientation the desired orientation at the target position (in radians).
		 */
		void startApproaching(const Vec2f &source, const Vec2f &target, const Vec3f &orientation);

		/**
		 * Stop approaching the current target.
		 */
		void stopApproaching();

		/**
		 * @return true if currently approaching a target.
		 */
		bool isNavigationApproaching() const { return (navModeMask_ & APPROACHING) != 0; }

		/**
		 * @return the current position.
		 */
		const Vec3f& currentPosition() const { return currentPos_; }

		/**
		 * @return the current direction.
		 */
		const Vec3f& currentDirection() const { return currentDir_; }

		/**
		 * @return the current velocity.
		 */
		const Vec3f& currentVelocity() const { return currentVel_; }

		/**
		 * @return the current speed.
		 */
		float currentSpeed() const { return currentSpeed_; }

		/**
		 * @return true if the NPC is standing still.
		 */
		bool isStandingStill() const { return currentSpeed_ < 3.0f; }

		/**
		 * @return the current Bezier curve path when approaching.
		 */
		math::Bezier<Vec2f> currentCurvePath() const { return curvePath_; }

		/**
		 * Start flocking with a group.
		 * @param group the group to flock with.
		 */
		void startFlocking(const ref_ptr<ObjectGroup> &group);

		/**
		 * Stop flocking.
		 */
		void stopFlocking();

		/**
		 * @return true if currently flocking.
		 */
		bool isNavigationFlocking() const { return (navModeMask_ & FLOCKING) != 0; }

		/**
		 * Stop all navigation.
		 */
		void stopNavigation();

		/**
		 * Initialize a perception frame.
		 */
		void navCollisionFrameBegin();

		/**
		 * Cleanup a perception frame.
		 */
		void navCollisionFrameEnd();

		/**
		 * Handle a perception event within a perception frame.
		 * @param evt the perception data.
		 */
		void navCollisionFrameAdd(const CollisionEvent &evt);

		// Override Animation
		void cpuUpdate(double dt) override;

	protected:
		ref_ptr<ModelTransformation> tf_;
		uint32_t tfIdx_;

		ref_ptr<WorldModel> worldModel_;
		ref_ptr<ObjectGroup> navGroup_;

		// if set, we will ignore collisions with this shape
		ref_ptr<BoundingShape> patientShape_;
		// Base orientation of the mesh around y axis
		float baseOrientation_ = M_PI_2;
		Quaternion baseRotation_ = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

		float personalSpace_ = 4.5f;
		float personalSpaceSq_ = personalSpace_ * personalSpace_;
		float wallTangentWeight_ = 0.4f;
		float collisionWeight_ = 0.7f;
		float velOrientationWeight_ = 0.9f;
		float maxTurnDegPerSec_ = 90.0f;
		float maxTurn_ = maxTurnDegPerSec_ * (M_PI / 180.0f);
		// increase turn threshold when navigating within personal space.
		float turnFactorPersonalSpace_ = 3.0f;
		float affordanceSlotRadius_ = 0.75f;
		float pushThroughDistance_ = 1.0f;
		float lookAheadThreshold_ = 6.0f;
		float avoidanceDecay_ = 0.1f;
		float radialDamping_ = 0.1f;
		uint32_t numLUTEntries_ = 100;

		// Terrain information.
		float floorHeight_ = 0.0f;
		ref_ptr<HeightMap> heightMap_;

		// Walking behavior parameters.
		float walkSpeed_ = 0.05f;
		float runSpeed_ = 0.1f;
		float lookAheadDistance_ = 15.0f;
		float heightSmoothFactor_ = 12.0f;
		float wallAvoidance_ = 1.0f;
		float characterAvoidance_ = 10.0f;
		float cohesionWeight_ = 1.0f;
		float memberSeparationWeight_ = 1.5f;
		float groupSeparationWeight_ = 5.0f;

		bool isWalking_ = true;
		int navModeMask_ = NO_NAVIGATION;
		// The current path of the NPC, if any.
		std::vector<ref_ptr<WayPoint>> currentPath_;
		float distanceToTarget_ = std::numeric_limits<float>::max();
		uint32_t currentPathIndex_ = 0;
		math::Bezier<Vec2f> curvePath_;
		math::ArcLengthLUT curveLUT_;
		Vec3f curveEndDir_ = Vec3f::zero();

		// Number of collisions counted in last perception update.
		uint32_t navCollisionCount_ = 0;
		// Avoidance vector computed in last perception update.
		Vec3f navCollision_ = Vec3f::zero();
		float collisionStrength_ = 0.0f;
		// Dynamic look ahead distance for collision shape.
		float dynLookAheadDistance_ = lookAheadDistance_;
		bool hasNewPerception_ = false;

		Vec3f navFlocking_ = Vec3f::zero();
		float flockingStrength_ = 0.0f;

		Vec3f currentPos_;
		Vec3f currentVel_ = Vec3f::zero();
		Vec3f currentVelDir_ = Vec3f::zero();
		float currentSpeed_ = 0.0f;
		Vec3f currentForce_ = Vec3f::zero();
		Vec3f currentDir_;
		Mat4f currentVal_;
		Vec3f initialScale_;

		Vec3f approachDir_ = Vec3f::zero();
		Vec3f desiredDir_ = Vec3f::zero();
		Vec3f desiredVel_ = Vec3f::zero();
		double lastDT_ = 0.0;
		float curveTime_ = 0.0f;
		float frameTime_ = 0.0f;
		float approachTime_ = 0.0;
		float approachDuration_ = 0.0;

		float getHeight(const Vec2f &pos);

	private:
		void updateNavController();

		void updateNavVelocity(float desiredSpeed, const Vec3f &avoidance, float avoidanceStrength);

		void updateNavOrientation(const Vec2f &currentPos2D, float desiredSpeed);

		void updateNavFlocking();

		struct NPCNeighborData {
			NavigationController *npc;
			float lookAhead = 10.0f;
			uint32_t neighborCount = 0;
			Vec3f avoidance = Vec3f::zero();
		};

		void handleCharacterCollision(const CollisionData &percept);

		void handleStaticCollision(const CollisionData &percept);
	};
} // namespace

#endif /* REGEN_NAVIGATION_CONTROLLER_H_ */
