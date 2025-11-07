#ifndef REGEN_BEHAVIOR_ACTIONS_H_
#define REGEN_BEHAVIOR_ACTIONS_H_
#include "behavior-tree.h"
#include "world/action-type.h"
#include "world/place.h"

namespace regen {
	/**
	 * A base class for action nodes in a behavior tree.
	 */
	class BehaviorActionNode : public BehaviorTree::Node {
	public:
		BehaviorActionNode(): BehaviorTree::Node() {}

		~BehaviorActionNode() override = default;
	};

	/**
	 * An action node that executes a lambda function.
	 */
	class BehaviorLambdaNode : public BehaviorActionNode {
		BehaviorTickFunction func;
	public:
		explicit BehaviorLambdaNode(BehaviorTickFunction f)
			: BehaviorActionNode(), func(std::move(f)) {}

		~BehaviorLambdaNode() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override {
			return func(bb);
		}
	};

	/**
	 * An action node that selects a target place of a desired type,
	 * or one based on the NPC's traits if no type is specified.
	 */
	class SelectTargetPlace : public BehaviorActionNode {
		PlaceType desiredPlaceType = PlaceType::LAST;
	public:
		SelectTargetPlace() : BehaviorActionNode() {}

		explicit SelectTargetPlace(PlaceType pt)
			: BehaviorActionNode(), desiredPlaceType(pt) {}

		~SelectTargetPlace() override = default;

		void setDesiredPlaceType(PlaceType pt) { desiredPlaceType = pt; }

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that sets the target place to a specific place.
	 */
	class SetTargetPlace : public BehaviorActionNode {
		ref_ptr<Place> targetPlace;
	public:
		explicit SetTargetPlace(const ref_ptr<Place> &place)
			: BehaviorActionNode(), targetPlace(place) {}

		~SetTargetPlace() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that sets a roaming target point.
	 */
	class SetRoamingTarget : public BehaviorActionNode {
		ref_ptr<WayPoint> roamingWP_;
	public:
		SetRoamingTarget() : BehaviorActionNode() {
			roamingWP_ = ref_ptr<WayPoint>::alloc("roaming-target");
			roamingWP_->setRadius(0.5f);
		}

		~SetRoamingTarget() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that selects an activity based on the NPC's traits
	 * and the possibilities afforded by the current place.
	 */
	class SelectPlaceActivity : public BehaviorActionNode {
		ref_ptr<Place> lastPlace_;
		std::vector<std::pair<ActionType, float>> actionPossibilities_;
	public:
		SelectPlaceActivity() : BehaviorActionNode() {}

		~SelectPlaceActivity() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;

	protected:
		void updateActionPossibilities(Blackboard& kb);
	};

	/**
	 * An action node that sets a desired activity.
	 */
	class SetDesiredActivity : public BehaviorActionNode {
		const ActionType desiredAction;
		float minDuration_ = 0.0f;
		float maxDuration_ = 0.0f;
	public:
		explicit SetDesiredActivity(ActionType a)
			: BehaviorActionNode(), desiredAction(a) {}

		~SetDesiredActivity() override = default;

		void setDurationRange(float minDuration, float maxDuration) {
			minDuration_ = minDuration;
			maxDuration_ = maxDuration;
		}

		void setFixedDuration(float duration) {
			minDuration_ = duration;
			maxDuration_ = duration;
		}

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that selects a patient (object) for the desired activity
	 * at the current place.
	 */
	class SelectPlacePatient : public BehaviorActionNode {
		ActionType lastDesiredAction_ = ActionType::IDLE;
		ref_ptr<Place> lastPlace_;
	public:
		SelectPlacePatient() : BehaviorActionNode() {}

		~SelectPlacePatient() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that selects a location for the desired activity
	 * at the current place.
	 */
	class SelectPlaceLocation : public SelectPlacePatient {
	public:
		SelectPlaceLocation() : SelectPlacePatient() {}

		~SelectPlaceLocation() override = default;
	};

	/**
	 * An action node that sets a specific patient (object) for the desired activity.
	 */
	class SetPatient : public BehaviorActionNode {
		std::vector<ref_ptr<WorldObject>> options = {};
		std::string affordanceName;
	public:
		explicit SetPatient(const ref_ptr<WorldObject> &obj, std::string_view affordanceName)
				: BehaviorActionNode(), affordanceName(affordanceName) {
			options.push_back(obj);
		}

		explicit SetPatient(const WorldObjectVec* objs, std::string_view affordanceName)
				: BehaviorActionNode(), affordanceName(affordanceName) {
			if (objs) {
				for (const auto &obj: *objs) {
					options.push_back(obj);
				}
			}
		}

		~SetPatient() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that unsets the current patient (object).
	 */
	class UnsetPatient : public BehaviorActionNode {
	public:
		UnsetPatient() : BehaviorActionNode() {}

		~UnsetPatient() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that moves the NPC to its target point.
	 */
	class MoveToTargetPoint : public BehaviorActionNode {
		float reachRadius_ = 0.5f;
	public:
		MoveToTargetPoint() : BehaviorActionNode() {}

		~MoveToTargetPoint() override = default;

		/** Set the radius within which the target point is considered reached. */
		void setReachRadius(float r) { reachRadius_ = r; }

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that moves the NPC to its target place.
	 */
	class MoveToTargetPlace : public BehaviorActionNode {
	public:
		MoveToTargetPlace() : BehaviorActionNode() {}

		~MoveToTargetPlace() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that moves the NPC to its desired location (object).
	 */
	class MoveToLocation : public BehaviorActionNode {
	public:
		MoveToLocation() : BehaviorActionNode() {}

		~MoveToLocation() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that moves the NPC to its current social group.
	 */
	class MoveToGroup : public BehaviorActionNode {
	public:
		MoveToGroup() : BehaviorActionNode() {}

		~MoveToGroup() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that moves the NPC to its patient (object).
	 */
	class MoveToPatient : public BehaviorActionNode {
	public:
		MoveToPatient() : BehaviorActionNode() {}

		~MoveToPatient() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that makes the NPC perform a specific action.
	 */
	class PerformAction : public BehaviorActionNode {
		const ActionType actionType;
		float maxDuration_ = -1.0f;
		float currentDuration_ = 0.0f;
	public:
		explicit PerformAction(ActionType a) : BehaviorActionNode(), actionType(a) {}

		~PerformAction() override = default;

		/** Set the maximum duration for the action.
		 *  After this duration, the action will be stopped.
		 *  Default is infinite duration.
		 */
		void setMaxDuration(float maxDuration) { maxDuration_ = maxDuration; }

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that makes the NPC perform its desired action.
	 */
	class PerformDesiredAction : public BehaviorActionNode {
	public:
		PerformDesiredAction() : BehaviorActionNode() {}

		~PerformDesiredAction() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that makes the NPC perform the afforded action
	 * with its current patient (object).
	 * This will report failure in case the object is out of reach.
	 */
	class PerformAffordedAction : public BehaviorActionNode {
	public:
		PerformAffordedAction() : BehaviorActionNode() {}

		~PerformAffordedAction() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that makes the NPC form a group at its current location.
	 * The action fails in case the NPC does not find a friendly NPC at the current location.
	 * Else a groups is formed and the action succeeds.
	 * Effectively this is a navigation action where multiple NPC align their positions and directions.
	 */
	class FormLocationGroup : public BehaviorActionNode {
	public:
		FormLocationGroup() : BehaviorActionNode() {}

		~FormLocationGroup() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that makes the NPC leave its current social group, if any.
	 */
	class LeaveGroup : public BehaviorActionNode {
	public:
		LeaveGroup() : BehaviorActionNode() {}

		~LeaveGroup() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

	/**
	 * An action node that makes the NPC leave its current location, if any.
	 */
	class LeaveLocation : public BehaviorActionNode {
	public:
		LeaveLocation() : BehaviorActionNode() {}

		~LeaveLocation() override = default;

		BehaviorStatus tick(Blackboard& bb, float dt_s) override;
	};

} // namespace

#endif /* REGEN_BEHAVIOR_ACTIONS_H_ */
