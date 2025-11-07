#include "action-type.h"

#include "regen/utility/logging.h"

using namespace regen;

std::vector<MotionType> regen::getMotionTypesForAction(ActionType action) {
	std::vector<MotionType> motions;
	switch (action) {
		case ActionType::IDLE:
		case ActionType::OBSERVING:
			motions.push_back(MotionType::IDLE);
			break;
		case ActionType::SITTING:
			motions.push_back(MotionType::SIT);
			break;
		case ActionType::INSPECTING:
			motions.push_back(MotionType::INSPECT);
			break;
		case ActionType::PRAYING:
			motions.push_back(MotionType::CROUCH);
			break;
		case ActionType::ATTACKING:
			motions.push_back(MotionType::ATTACK);
			break;
		case ActionType::BLOCKING:
			motions.push_back(MotionType::BLOCK);
			break;
		case ActionType::SLEEPING:
			motions.push_back(MotionType::SLEEP);
			break;
		case ActionType::INTIMIDATING:
			motions.push_back(MotionType::INTIMIDATE);
			break;
		case ActionType::NAVIGATING:
		case ActionType::WALKING:
		case ActionType::PATROLLING:
		case ActionType::STROLLING:
			motions.push_back(MotionType::WALK);
			break;
		case ActionType::FLEEING:
			motions.push_back(MotionType::RUN);
			break;
		case ActionType::CONVERSING:
			motions.push_back(MotionType::IDLE);
			motions.push_back(MotionType::VOCALIZE);
			motions.push_back(MotionType::AGREE);
			motions.push_back(MotionType::DISAGREE);
			break;
		case ActionType::FLOCKING:
			motions.push_back(MotionType::MOTION_LAST);
			break;
		default:
			// no default motion
			break;
	}
	return motions;
}

std::ostream &regen::operator<<(std::ostream &out, const ActionType &v) {
	switch (v) {
		case ActionType::IDLE:
			return out << "IDLE";
		case ActionType::CONVERSING:
			return out << "CONVERSE";
		case ActionType::INSPECTING:
			return out << "INSPECT";
		case ActionType::GROOMING:
			return out << "GROOM";
		case ActionType::STROLLING:
			return out << "STROLL";
		case ActionType::OBSERVING:
			return out << "OBSERVE";
		case ActionType::PATROLLING:
			return out << "PATROL";
		case ActionType::PRAYING:
			return out << "PRAY";
		case ActionType::SLEEPING:
			return out << "SLEEP";
		case ActionType::ATTACKING:
			return out << "ATTACK";
		case ActionType::INTIMIDATING:
			return out << "INTIMIDATE";
		case ActionType::BLOCKING:
			return out << "BLOCK";
		case ActionType::FLEEING:
			return out << "FLEE";
		case ActionType::NAVIGATING:
			return out << "NAVIGATING";
		case ActionType::WALKING:
			return out << "WALK";
		case ActionType::SITTING:
			return out << "SIT";
		case ActionType::FLOCKING:
			return out << "FLOCK";
		case ActionType::LAST_ACTION:
			return out << "LAST_ACTION";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, ActionType &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "IDLE") v = ActionType::IDLE;
	else if (val == "CONVERSE" || val == "TALK") v = ActionType::CONVERSING;
	else if (val == "INSPECT" || val == "LOOKAROUND") v = ActionType::INSPECTING;
	else if (val == "GROOM" || val == "GROOMING") v = ActionType::GROOMING;
	else if (val == "STROLL") v = ActionType::STROLLING;
	else if (val == "PATROL") v = ActionType::PATROLLING;
	else if (val == "FLEE" || val == "RUN") v = ActionType::FLEEING;
	else if (val == "NAVIGATE" || val == "GO") v = ActionType::NAVIGATING;
	else if (val == "WALK") v = ActionType::WALKING;
	else if (val == "SIT" || val == "SITTING") v = ActionType::SITTING;
	else if (val == "BLOCK") v = ActionType::BLOCKING;
	else if (val == "ATTACK" || val == "FIGHT") v = ActionType::ATTACKING;
	else if (val == "INTIMIDATE" || val == "THREATEN") v = ActionType::INTIMIDATING;
	else if (val == "OBSERVE" || val == "WATCH") v = ActionType::OBSERVING;
	else if (val == "SLEEP" || val == "SLEEPING") v = ActionType::SLEEPING;
	else if (val == "PRAY" || val == "PRAYING") v = ActionType::PRAYING;
	else if (val == "FLOCK" || val == "FLOCKING") v = ActionType::FLOCKING;
	else {
		REGEN_WARN("Unknown action type '" << val << "'. Using IDLE.");
		v = ActionType::IDLE;
	}
	return in;
}
