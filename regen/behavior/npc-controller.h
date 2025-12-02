#ifndef REGEN_NPC_CONTROLLER_H_
#define REGEN_NPC_CONTROLLER_H_

#include "behavior-tree.h"
#include "regen/animation/bone-tree.h"
#include "regen/objects/model-transformation.h"
#include "regen/utility/indexed.h"
#include "navigation/navigation-controller.h"
#include "navigation/path-planner.h"
#include "perception/perception-system.h"
#include "skeleton/bone-controller.h"

namespace regen {
	/**
	 * A base class for non-player character controllers.
	 */
	class NonPlayerCharacterController : public NavigationController {
	public:
		NonPlayerCharacterController(
			const ref_ptr<Mesh> &mesh,
			const Indexed<ref_ptr<ModelTransformation> > &tfIndexed,
			const ref_ptr<BoneAnimationItem> &bones,
			const ref_ptr<WorldModel> &world);

		~NonPlayerCharacterController() override;

		/**
		 * Set the world time.
		 * @param worldTime the world time.
		 */
		void setWorldTime(const WorldTime *worldTime) { worldTime_ = worldTime; }

		/**
		 * Set the decision interval for the NPC.
		 * @param interval_s the decision interval in seconds.
		 */
		void setDecisionInterval(float interval_s) { decisionInterval_s_ = interval_s; }

		/**
		 * Set the perception interval for the NPC.
		 * @param interval_s the perception interval in seconds.
		 */
		void setPerceptionInterval(float interval_s) { perceptionInterval_s_ = interval_s; }

		/**
		 * Set the perception system for the NPC.
		 * @param ps the perception system.
		 */
		void setPerceptionSystem(std::unique_ptr<PerceptionSystem> ps);

		/**
		 * Set the behavior tree for the NPC.
		 * @param rootNode the behavior tree.
		 */
		void setBehaviorTree(std::unique_ptr<BehaviorTree::Node> rootNode);

		/**
		 * Add a waypoint to the path planner.
		 * @param wp the waypoint.
		 */
		void addWayPoint(const ref_ptr<WayPoint> &wp);

		/**
		 * Add a connection between two waypoints to the path planner.
		 * @param from the from waypoint.
		 * @param to the to waypoint.
		 * @param bidirectional if true, add a connection in both directions.
		 */
		void addConnection(
				const ref_ptr<WayPoint> &from,
				const ref_ptr<WayPoint> &to,
				bool bidirectional = true);

		/**
		 * Get the knowledge base of the NPC.
		 * @return the knowledge base.
		 */
		Blackboard& knowledgeBase() { return knowledgeBase_; }

		/**
		 * Update animation and movement target.
		 * @param dt the time difference.
		 */
		virtual void updateController(double dt);

		/**
		 * Initialize the controller.
		 * This is called once before the first updateController() call.
		 */
		virtual void initializeController() {}

		// override
		void cpuUpdate(double dt) override;

	protected:
		ref_ptr<ModelTransformation> tf_;
		ref_ptr<BoneAnimationItem> animItem_;
		const WorldTime *worldTime_ = nullptr;

		Blackboard knowledgeBase_;
		std::unique_ptr<BehaviorTree> behaviorTree_;
		std::unique_ptr<PerceptionSystem> perceptionSystem_;
		ref_ptr<BoneController> boneController_;
		ref_ptr<PathPlanner> pathPlanner_;

		float decisionInterval_s_ = 0.25f;
		float perceptionInterval_s_ = 0.1f;

		float timeSinceLastDecision_s_ = 100.0f;
		float timeSinceLastPerception_s_ = 100.0f;

		// Flag is used for continuous movement.
		bool isLastAnimationMovement_ = false;
		bool hasNavigatingAction_ = false;
		bool hasPatrollingAction_ = false;
		bool hasStrollingAction_ = false;

		bool startNavigate(bool loopPath, bool advancePath);

		Vec2f pickTargetPosition(const Patient &navTarget) const;

		Vec2f pickTravelPosition(const WorldObject &wp) const;

		uint32_t findClosestWP(const std::vector<ref_ptr<WayPoint>> &wps) const;

		bool updatePathNPC();

		void updateNavigationBehavior();

		void updateKnowledgeBase(float dt_s);

		void setIdle();
	};
} // namespace

#endif /* REGEN_NPC_CONTROLLER_H_ */
