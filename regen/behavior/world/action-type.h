#ifndef REGEN_MODEL_ACTION_TYPE_H_
#define REGEN_MODEL_ACTION_TYPE_H_

#include <iostream>
#include <vector>
#include "regen/behavior/skeleton/motion-type.h"

namespace regen {
	/**
	 * The types of actions a character can perform.
	 */
	enum class ActionType {
		IDLE = 0,
		CONVERSING,
		INSPECTING,
		OBSERVING,
		GROOMING,
		PRAYING,
		SLEEPING,
		SITTING,
		INTIMIDATING,
		ATTACKING,
		BLOCKING,
		FLEEING,
		NAVIGATING,
		STROLLING,
		PATROLLING,
		// enforce walk animation without path based navigation.
		// e.g. flocking could include walking, but not all the time.
		WALKING,
		FLOCKING,
		LAST_ACTION
	};

	/**
	 * Get the possible motion types for a given action type.
	 * @param action The action type.
	 * @return A vector of motion types that can be used to perform the action.
	 */
	std::vector<MotionType> getMotionTypesForAction(ActionType action);

	std::ostream &operator<<(std::ostream &out, const ActionType &v);

	std::istream &operator>>(std::istream &in, ActionType &v);
} // namespace

#endif /* REGEN_MODEL_ACTION_TYPE_H_ */
