#include "behavior-actions.h"
#include "blackboard.h"
#include "world/action-type.h"

//#define NPC_ACTIONS_DEBUG

using namespace regen;

float traitStrength(const std::vector<float> &positive, const std::vector<float> &negative) {
	if (positive.empty() && negative.empty()) return 0.5f;
	float pos = 0.f, neg = 0.f;
	for (float t : positive) pos += t;
	for (float t : negative) neg += t;
	// Combine into [0,1] range. 0.5 = neutral, >0.5 favors positive traits.
	pos = positive.empty() ? 0.0f : pos / positive.size();
	neg = negative.empty() ? 0.0f : neg / negative.size();
	return math::clamp(0.5f + 0.5f * (pos - neg), 0.0f, 1.0f);
}

static void setLingerTime(Blackboard& kb, PlaceType placeType) {
	switch (placeType) {
		case PlaceType::HOME:
			kb.setLingerTime(kb.baseTimePlace() * (0.75f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.laziness()}, {kb.sociability(), kb.spirituality(), kb.bravery()})));
			break;
		case PlaceType::GATHERING:
			kb.setLingerTime(kb.baseTimePlace() * (0.75f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.sociability()}, {kb.laziness()})));
			break;
		case PlaceType::SPIRITUAL:
			kb.setLingerTime(kb.baseTimePlace() * (1.5f +
				0.5f * math::random<float>() +
				1.0f * traitStrength({kb.spirituality(), kb.sociability()}, {kb.laziness()})));
			break;
		case PlaceType::PATROL_POINT:
			kb.setLingerTime(kb.baseTimePlace() * (0.5f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.bravery(), kb.alertness()}, {kb.laziness(), kb.sociability()})));
			break;
		default:
			kb.setLingerTime(kb.baseTimePlace() * (0.5f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.laziness()}, {kb.sociability(), kb.spirituality(), kb.bravery()})));
			break;
	}
}

static void setInitialDistance(Blackboard& kb, Patient &patient) {
	const Vec3f &characterPos = kb.currentPosition();
	if (!patient.object) {
		REGEN_WARN("["<<kb.instanceId()<<"] Unable to set initial distance, no object set.");
		patient.currentDistance = std::numeric_limits<float>::max();
	} else {
		float dist = (patient.object->position2D() - Vec2f(characterPos.x, characterPos.z)).length();
		patient.currentDistance = dist;
	}
}

BehaviorStatus SelectTargetPlace::tick(Blackboard& kb, float /*dt_s*/) {
	auto &targetPlace = kb.targetPlace();
	if (targetPlace.get() && (kb.lingerTime() > 0.0f || kb.activityTime() > 0.0f)) {
		return BehaviorStatus::SUCCESS;
	}
	if (desiredPlaceType == PlaceType::LAST) {
		// Compute randomized weights for the places.
		float homeWeight = (0.5f + 0.25f * math::random<float>() +
			0.25f * traitStrength({kb.laziness()}, {kb.sociability(), kb.spirituality()}));
		float marketWeight = (0.5f + 0.25f * math::random<float>() +
			0.25f * traitStrength({kb.sociability()}, {kb.laziness()}));
		float shrineWeight = (0.5f + 0.25f * math::random<float>() +
			0.25f * traitStrength({kb.spirituality()}, {kb.laziness()}));
		if (!kb.homePlace()) homeWeight = 0.0f;
		auto &gatheringPlaces = kb.getPlacesByType(PlaceType::GATHERING);
		auto &spiritualPlaces = kb.getPlacesByType(PlaceType::SPIRITUAL);
		if (gatheringPlaces.empty()) marketWeight = 0.0f;
		if (spiritualPlaces.empty()) shrineWeight = 0.0f;

		float r = math::random<float>() * (homeWeight + marketWeight + shrineWeight);
		marketWeight += homeWeight;
		shrineWeight += marketWeight;

		if (r < homeWeight) {
			kb.setTargetPlace(kb.homePlace());
		} else if (r < marketWeight) {
			kb.setTargetPlace(gatheringPlaces[math::randomInt() % gatheringPlaces.size()]);
		} else {
			kb.setTargetPlace(spiritualPlaces[math::randomInt() % spiritualPlaces.size()]);
		}
	} else {
		auto &places = kb.getPlacesByType(desiredPlaceType);
		if (places.empty()) {
			return BehaviorStatus::FAILURE;
		}
		kb.setTargetPlace(places[math::randomInt() % places.size()]);
	}
	setLingerTime(kb, kb.targetPlace()->placeType());
#ifdef NPC_ACTIONS_DEBUG
	REGEN_INFO("["<<kb.instanceId()<<"] Selected target place " <<
		(kb.targetPlace().get() ? kb.targetPlace()->name() : "null"));
#endif
	return BehaviorStatus::SUCCESS;
}

BehaviorStatus SetTargetPlace::tick(Blackboard& kb, float /*dt_s*/) {
	auto currentTargetPlace = kb.targetPlace().get();
	if (currentTargetPlace && (kb.lingerTime() > 0.0f || kb.activityTime() > 0.0f)) {
		return BehaviorStatus::SUCCESS;
	}
	if (targetPlace.get() != currentTargetPlace) {
		kb.setTargetPlace(targetPlace);
		setInitialDistance(kb, kb.navigationTarget());
		setLingerTime(kb, targetPlace->placeType());
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Setting target place to " <<
			(targetPlace.get() ? targetPlace->name() : "null"));
#endif
	}
	return BehaviorStatus::SUCCESS;
}

BehaviorStatus SetRoamingTarget::tick(Blackboard& kb, float /*dt_s*/) {
	auto currentPlace = kb.currentPlace();
	Vec2f placeCenter;
	float placeRadius;

	if (currentPlace.get()) {
		// Pick a random position within the current place.
		placeCenter = currentPlace->position2D();
		placeRadius = currentPlace->radius();
	} else {
		REGEN_WARN("["<<kb.instanceId()<<"] No current place for roaming.");
		placeCenter = Vec2f::zero();
		placeRadius = 50.0f;
	}

	float angle = math::random<float>() * 2.0f * M_PI;
	float radius = math::random<float>() * placeRadius;
	Vec2f targetPos = placeCenter + Vec2f(cos(angle), sin(angle)) * radius;
	roamingWP_->setPosition(Vec3f(targetPos.x, 0.0f, targetPos.y)); // Height will be set by navigation controller.
	// Set navigation target to the position.
	Patient navTarget;
	navTarget.object = roamingWP_;
	navTarget.affordance = {};
	navTarget.affordanceSlot = -1;
	kb.setNavigationTarget(navTarget);
	setInitialDistance(kb, kb.navigationTarget());

	return BehaviorStatus::SUCCESS;
}

static bool canUseAt(Blackboard& kb, ActionType action, const ref_ptr<Place> &place) {
	return place->hasAffordance(action) && kb.canPerformAction(action);
}

static bool canWalkAt(Blackboard& kb, PathwayType pathway, const ref_ptr<Place> &place) {
	return place->hasPathWay(pathway) && kb.canPerformAction(ActionType::PATROLLING);
}

void SelectPlaceActivity::updateActionPossibilities(Blackboard& kb) {
	// TODO: improve probability handling, make it more generic!
	actionPossibilities_.clear();
	actionPossibilities_.emplace_back(ActionType::IDLE, (
			0.05f +
			0.05f * math::random<float>() +
			0.1f * traitStrength({kb.laziness()}, {kb.alertness(), kb.sociability()})));

	if (canWalkAt(kb, PathwayType::PATROL, lastPlace_)) {
		actionPossibilities_.emplace_back(ActionType::PATROLLING, (
				0.1f +
				0.1f * math::random<float>() +
				0.4f * traitStrength({kb.alertness(), kb.bravery()}, {kb.laziness(), kb.sociability()})));
	}
	if (canWalkAt(kb, PathwayType::STROLL, lastPlace_)) {
		actionPossibilities_.emplace_back(ActionType::STROLLING, (
				0.15f +
				0.15f * math::random<float>() +
				0.4f * traitStrength({kb.laziness()}, {kb.sociability(), kb.alertness(), kb.bravery()})));
	}
	if (canUseAt(kb, ActionType::OBSERVING, lastPlace_)) {
		actionPossibilities_.emplace_back(ActionType::OBSERVING, (
				0.25f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.alertness()}, {kb.sociability()})));
	}
	if (canUseAt(kb, ActionType::CONVERSING, lastPlace_)) {
		actionPossibilities_.emplace_back(ActionType::CONVERSING, (
				0.25f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.sociability()}, {kb.laziness()})));
	}
	if (canUseAt(kb, ActionType::PRAYING, lastPlace_)) {
		actionPossibilities_.emplace_back(ActionType::PRAYING, (
				0.25f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.spirituality()}, {kb.laziness()})));
	}
	if (canUseAt(kb, ActionType::SLEEPING, lastPlace_)) {
		actionPossibilities_.emplace_back(ActionType::SLEEPING, (
				0.1f +
				0.1f * math::random<float>() +
				0.5f * traitStrength({kb.laziness()}, {kb.alertness(), kb.sociability()})));
	}
	if (canUseAt(kb, ActionType::ATTACKING, lastPlace_)) {
		actionPossibilities_.emplace_back(ActionType::ATTACKING, (
				0.15f +
				0.15f * math::random<float>() +
				0.4f * traitStrength({kb.bravery()}, {kb.laziness(), kb.sociability()})));
	}
	// Sum and normalize weights.
	float totalWeight = 0.0f;
	for (auto &p : actionPossibilities_) {
		totalWeight += p.second;
	}
	if (!actionPossibilities_.empty()) {
		for (auto &p : actionPossibilities_) {
			p.second /= totalWeight;
		}
	}
}

static void setActionTime(Blackboard& kb, ActionType activity) {
	switch (activity) {
		case ActionType::OBSERVING:
			kb.setActivityTime(kb.baseTimeActivity() * (0.5f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.laziness()}, {kb.alertness(), kb.sociability()})));
			break;
		case ActionType::CONVERSING:
			kb.setActivityTime(kb.baseTimeActivity() * (0.5f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.sociability()}, {kb.laziness()})));
			break;
		case ActionType::PRAYING:
			kb.setActivityTime(kb.baseTimeActivity() * (1.0f +
				0.5f * math::random<float>() +
				1.0f * traitStrength({kb.spirituality()}, {kb.laziness()})));
			break;
		case ActionType::SLEEPING:
			kb.setActivityTime(kb.baseTimeActivity() * (1.5f +
				0.5f * math::random<float>() +
				1.0f * traitStrength({kb.laziness()}, {kb.alertness(), kb.sociability()})));
			break;
		case ActionType::PATROLLING:
			kb.setActivityTime(kb.baseTimeActivity() * (1.5f +
				0.5f * math::random<float>() +
				0.5f * traitStrength({kb.alertness(), kb.bravery()}, {kb.laziness(), kb.sociability()})));
			break;
		case ActionType::STROLLING:
			kb.setActivityTime(kb.baseTimeActivity() * (1.0f +
				0.5f * math::random<float>() +
				0.5f * traitStrength({}, {kb.laziness(), kb.sociability()})));
			break;
		case ActionType::ATTACKING:
			kb.setActivityTime(kb.baseTimeActivity() * (0.5f +
				0.25f * math::random<float>() +
				0.5f * traitStrength({kb.bravery()}, {kb.laziness(), kb.sociability()})));
			break;
		case ActionType::NAVIGATING:
			kb.setActivityTime(kb.baseTimeActivity() * (4.0f +
				2.0f * math::random<float>() +
				2.0f * traitStrength({}, {kb.laziness()})));
			break;
		default: // IDLE
			kb.setActivityTime(kb.baseTimeActivity() * (
				0.1f * math::random<float>() +
				0.5f * traitStrength({kb.laziness()}, {kb.alertness(), kb.sociability()})));
			break;
	}
}

BehaviorStatus SelectPlaceActivity::tick(Blackboard& kb, float /*dt_s*/) {
	auto &place = kb.currentPlace();
	if (kb.lingerTime() <= 0.0f) {
		// No place, or time to leave
		kb.setActivityTime(0.0f);
		return BehaviorStatus::SUCCESS;
	}
	if (!place.get()) {
		// No place, or time to leave
#ifdef NPC_ACTIONS_DEBUG
		REGEN_WARN("["<<kb.instanceId()<<"] No current place for activity selection.");
#endif
		return BehaviorStatus::FAILURE;
	}
	if (kb.activityTime() > 0.0f) {
		// Continue current activity, skip selection for now and move one.
		return BehaviorStatus::SUCCESS;
	}
	// Update weights for action possibilities at this place.
	if (place.get() != lastPlace_.get()) {
		lastPlace_ = place;
		updateActionPossibilities(kb);
	}
	// Get a random number [0,1] and pick an activity based on the weights.
	ActionType nextActivity = ActionType::IDLE;
	float r = math::random<float>();
	float scan = 0.0f;
	for (uint32_t i = 0; i < actionPossibilities_.size(); i++) {
		scan += actionPossibilities_[i].second;
		if (r <= scan) {
			nextActivity = actionPossibilities_[i].first;
			break;
		}
	}
	kb.setDesiredAction(nextActivity);
	setActionTime(kb, nextActivity);
	// If we use an object affordance of another type, unset the interaction target, else
	// we may continue using the same object.
	auto &currentInteraction = kb.interactionTarget();
	if (currentInteraction.affordance.get() && currentInteraction.affordance->type != nextActivity) {
		kb.unsetInteractionTarget();
	}
#ifdef NPC_ACTIONS_DEBUG
	REGEN_INFO("["<<kb.instanceId()<<"] Selected activity " << nextActivity <<
		" at place '" << place->name() << "'.");
#endif
	return BehaviorStatus::SUCCESS;
}

BehaviorStatus SetDesiredActivity::tick(Blackboard& kb, float /*dt_s*/) {
	if (kb.activityTime() <= 0.0f) {
		kb.setDesiredAction(desiredAction);
		if (minDuration_ >= 0.0f) {
			kb.setActivityTime(minDuration_ +
				(math::random<float>() * (maxDuration_ - minDuration_)));
		} else {
			setActionTime(kb, desiredAction);
		}
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Setting desired activity " << desiredAction << ".");
#endif
	}
	return BehaviorStatus::SUCCESS;
}

static int reserveAffordanceSlot(Blackboard &kb, const ref_ptr<Affordance> &aff) {
	if (!aff->hasFreeSlot()) return -1;
	bool randomizeSlot = (aff->layout != SlotLayout::GRID);
	return aff->reserveSlot(kb.characterObject(), randomizeSlot);
}

static BehaviorStatus selectPatient(Blackboard &kb,
		const ref_ptr<WorldObject> &obj,
		const ref_ptr<Affordance> &aff,
		int slotIdx) {
	kb.setInteractionTarget(obj, aff, slotIdx);
	setInitialDistance(kb, kb.interactionTarget());
	kb.setDistanceToTarget(kb.distanceToPatient());
	//setActionTime(kb, desiredAction);
	return BehaviorStatus::SUCCESS;
}

BehaviorStatus SelectPlacePatient::tick(Blackboard& kb, float /*dt_s*/) {
	auto &place = kb.currentPlace();
	if (kb.lingerTime() <= 0.0f) {
		// No time left at this place
		kb.setActivityTime(-1.0f);
		return BehaviorStatus::SUCCESS;
	}
	if (!place.get()) {
		// No place
#ifdef NPC_ACTIONS_DEBUG
		REGEN_WARN("["<<kb.instanceId()<<"] No current place for activity selection.");
#endif
		return BehaviorStatus::FAILURE;
	}
	ActionType desiredAction = kb.desiredAction();
	bool hasCurrentPatient = kb.hasInteractionTarget();
	if (hasCurrentPatient && desiredAction == lastDesiredAction_ && place.get() == lastPlace_.get()) {
		// No change, stick with last selection.
		return BehaviorStatus::SUCCESS;
	}
	lastDesiredAction_ = desiredAction;
	lastPlace_ = place;
	if (hasCurrentPatient) {
		kb.unsetInteractionTarget();
	}
	// TODO: Rather remember last patient, and if action type did not change,
	//  try to continue using the same object if possible.
	//  The perform/move-to actions unset interaction target when done, so that information
	//  is lost currently.

	// Retrieve affordances from place.
	auto &objects = place->getAffordanceObjects(desiredAction);
	if (objects.empty()) {
		// No objects with the desired affordance at this place
		return BehaviorStatus::FAILURE;
	}
	// For now pick a random affordance.
	ref_ptr<WorldObject> selected;
	ref_ptr<Affordance> aff;
	uint32_t startIdx = math::randomInt() % objects.size();
	for (uint32_t i = 0; i < objects.size(); i++) {
		auto candidate = objects[(i + startIdx) % objects.size()];
		auto candidateAff = candidate->getAffordance(desiredAction);
		if (candidateAff->hasFreeSlot()) {
			selected = candidate;
			aff = candidateAff;
			break;
		}
	}
	if (!selected) {
		// No available slots on any object.
		return BehaviorStatus::FAILURE;
	}
	int slotIdx = reserveAffordanceSlot(kb, aff);
	if (slotIdx == -1) {
		return BehaviorStatus::FAILURE;
	} else {
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Setting patient to '" <<
			selected->name() << "' for action " << desiredAction);
#endif
		return selectPatient(kb, selected, aff, slotIdx);
	}
}

BehaviorStatus SetPatient::tick(Blackboard &kb, float /*dt_s*/) {
	ref_ptr<WorldObject> patient;
	ref_ptr<Affordance> aff;
	ActionType desiredAction = kb.desiredAction();
	int affordanceSlot = -1;

	for (const auto &option : options) {
		aff = option->getAffordance(desiredAction, affordanceName);
		affordanceSlot = reserveAffordanceSlot(kb, aff);
		if (affordanceSlot != -1) {
			patient = option;
			break;
		}
	}
	if (affordanceSlot == -1) {
		return BehaviorStatus::FAILURE;
	}

	if (patient.get() == kb.interactionTarget().object.get()) {
		// Already set
		return BehaviorStatus::SUCCESS;
	} else if ( kb.interactionTarget().object.get()) {
		kb.unsetInteractionTarget();
	}

#ifdef NPC_ACTIONS_DEBUG
	REGEN_INFO("["<<kb.instanceId()<<"] Setting patient to '" <<
		patient->name() << "' for action " << desiredAction);
#endif
	return selectPatient(kb, patient, aff, affordanceSlot);
}

BehaviorStatus UnsetPatient::tick(Blackboard &kb, float /*dt_s*/) {
	if (kb.interactionTarget().object.get()) {
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Unsetting patient '" <<
			kb.interactionTarget().object->name() << "'.");
#endif
		kb.unsetInteractionTarget();
	}
	return BehaviorStatus::SUCCESS;
}

BehaviorStatus MoveToTargetPoint::tick(Blackboard &kb, float /*dt_s*/) {
	if (kb.distanceToTarget() < reachRadius_) {
		// Reached target place.
		kb.unsetNavigationTarget();
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Reached target point.");
#endif
		return BehaviorStatus::SUCCESS;
	} else {
		kb.setCurrentAction(ActionType::NAVIGATING);
		return BehaviorStatus::RUNNING;
	}
}

BehaviorStatus MoveToTargetPlace::tick(Blackboard &kb, float /*dt_s*/) {
	if (kb.lingerTime() <= 0.0f) {
		// Unable to reach in time.
		kb.unsetNavigationTarget();
		kb.setActivityTime(-1.0f);
		return BehaviorStatus::SUCCESS;
	}
	if (kb.navigationTarget().object.get() == kb.targetPlace().get()) {
		if (kb.distanceToTarget() < kb.targetPlace()->radius()) {
			// Reached target place.
			kb.unsetNavigationTarget();
			kb.setCurrentPlace(kb.targetPlace());
#ifdef NPC_ACTIONS_DEBUG
			REGEN_INFO("["<<kb.instanceId()<<"] Reached target place '" << kb.targetPlace()->name() << "'.");
#endif
			return BehaviorStatus::SUCCESS;
		} else {
			// Already moving to target place.
			kb.setCurrentAction(ActionType::NAVIGATING);
			return BehaviorStatus::RUNNING;
		}
	}
	kb.unsetCurrentPlace();
	kb.setCurrentAction(ActionType::NAVIGATING);
	kb.setNavigationTarget(kb.targetPlace(), {}, -1);
	setInitialDistance(kb, kb.navigationTarget());
#ifdef NPC_ACTIONS_DEBUG
	REGEN_INFO("["<<kb.instanceId()<<"] Moving to target place '" << kb.targetPlace()->name() << "'.");
#endif
	return BehaviorStatus::RUNNING;
}

BehaviorStatus MoveToLocation::tick(Blackboard& kb, float /*dt_s*/) {
	if (kb.lingerTime() <= 0.0f && kb.activityTime() <= 0.0f) {
		// Unable to reach in time.
		kb.unsetNavigationTarget();
		return BehaviorStatus::SUCCESS;
	}
	auto &location = kb.interactionTarget().object;
	if (!location.get()) {
		REGEN_WARN("["<<kb.instanceId()<<"] No interaction target set.");
		kb.unsetNavigationTarget();
		return BehaviorStatus::FAILURE;
	}
	if (location->objectType() != ObjectType::LOCATION) {
		REGEN_WARN("["<<kb.instanceId()<<"] Interaction target is not a location.");
		kb.unsetNavigationTarget();
		return BehaviorStatus::FAILURE;
	}
	// Note: we do not move to the affordance, but rather into the broad location space.
	if (kb.navigationTarget().object.get()
			&& location.get() == kb.navigationTarget().object.get()) {
		if (kb.distanceToTarget() < location->radius() * 0.75f) {
			// Reached location.
			kb.unsetNavigationTarget();
			kb.setCurrentLocation(ref_ptr<Location>::staticCast(location));
#ifdef NPC_ACTIONS_DEBUG
			REGEN_INFO("["<<kb.instanceId()<<"] Reached location '" << location->name() << "'.");
#endif
			return BehaviorStatus::SUCCESS;
		} else if (kb.navigationTarget().affordance.get() == kb.interactionTarget().affordance.get()
				&& kb.navigationTarget().affordanceSlot == kb.interactionTarget().affordanceSlot) {
			// Already moving to location.
			kb.setCurrentAction(ActionType::NAVIGATING);
			return BehaviorStatus::RUNNING;
		}
	}
	kb.unsetCurrentLocation();
	kb.setCurrentAction(ActionType::NAVIGATING);
	kb.setNavigationTarget(location, {}, -1);
	setInitialDistance(kb, kb.navigationTarget());
#ifdef NPC_ACTIONS_DEBUG
	REGEN_INFO("["<<kb.instanceId()<<"] Moving to location '" << location->name() << "'.");
#endif
	return BehaviorStatus::RUNNING;
}

BehaviorStatus MoveToGroup::tick(Blackboard& kb, float /*dt_s*/) {
	if (!kb.isPartOfGroup()) {
		// Not in a group.
		kb.unsetNavigationTarget();
#ifdef NPC_ACTIONS_DEBUG
		REGEN_WARN("["<<kb.instanceId()<<"] Not part of a group in MoveToGroup.");
#endif
		return BehaviorStatus::FAILURE;
	}
	if (kb.lingerTime() <= 0.0f && kb.activityTime() <= 0.0f) {
		// Unable to reach in time.
		kb.unsetNavigationTarget();
		return BehaviorStatus::SUCCESS;
	}
	if (kb.navigationTarget().object.get() == kb.currentGroup().get()) {
		if (kb.distanceToTarget() < kb.currentGroup()->radius()) {
			// Reached group.
#ifdef NPC_ACTIONS_DEBUG
			REGEN_INFO("["<<kb.instanceId()<<"] Reached group.");
#endif
			kb.unsetNavigationTarget();
			return BehaviorStatus::SUCCESS;
		} else {
			// Already moving to group.
			kb.setCurrentAction(ActionType::NAVIGATING);
			return BehaviorStatus::RUNNING;
		}
	}
	kb.setCurrentAction(ActionType::NAVIGATING);
	kb.setNavigationTarget(kb.currentGroup(), {}, -1);
	setInitialDistance(kb, kb.navigationTarget());
#ifdef NPC_ACTIONS_DEBUG
	REGEN_INFO("["<<kb.instanceId()<<"] Moving to group at position " <<
		kb.currentGroup()->position2D() << ".");
#endif
	return BehaviorStatus::RUNNING;
}

static float getReachRadius(const Patient &patient) {
	return patient.affordance.get() ?
		0.75 * Affordance::reachDistance :
		0.75 * patient.object->radius();
}

BehaviorStatus MoveToPatient::tick(Blackboard& kb, float /*dt_s*/) {
	if (!kb.interactionTarget().object) {
		// no patient set.
		REGEN_WARN("["<<kb.instanceId()<<"] No patient set.");
		kb.unsetInteractionTarget();
		kb.unsetNavigationTarget();
		return BehaviorStatus::FAILURE;
	}
	if (kb.activityTime() <= 0.0f) {
		// No remaining time to reach patient.
		kb.unsetInteractionTarget();
		kb.unsetNavigationTarget();
		return BehaviorStatus::SUCCESS;
	}
	auto &it = kb.interactionTarget();
	if (kb.distanceToPatient() <= getReachRadius(it)) {
		// Reached patient/affordance
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Reached patient '" << it.object->name() << "'.");
#endif
		kb.unsetNavigationTarget();
		return BehaviorStatus::SUCCESS;
	}
	auto currentNavTarget = kb.navigationTarget().object.get();
	if (currentNavTarget == kb.interactionTarget().object.get() &&
			kb.navigationTarget().affordance.get() == kb.interactionTarget().affordance.get() &&
			kb.navigationTarget().affordanceSlot == kb.interactionTarget().affordanceSlot) {
		// Already moving to patient.
		kb.setCurrentAction(ActionType::NAVIGATING);
		return BehaviorStatus::RUNNING;
	} else if (currentNavTarget != nullptr) {
		// There can only be one navigation target at a time.
		kb.unsetNavigationTarget();
	}
#ifdef NPC_ACTIONS_DEBUG
	REGEN_INFO("["<<kb.instanceId()<<"] Moving to patient '" << it.object->name() << "'.");
#endif
	kb.setCurrentAction(ActionType::NAVIGATING);
	kb.setNavigationTarget(kb.interactionTarget());
	return BehaviorStatus::RUNNING;
}

BehaviorStatus PerformAction::tick(Blackboard& kb, float dt_s) {
	if (kb.activityTime() <= 0.0f) {
		// Time ran out, stop action.
		currentDuration_ = 0.0f;
		return BehaviorStatus::SUCCESS;
	}
	if (maxDuration_ > 0.0f && currentDuration_ > maxDuration_) {
		// Time ran out, stop action.
		currentDuration_ = 0.0f;
		return BehaviorStatus::SUCCESS;
	} else {
		currentDuration_ += dt_s;
	}
	kb.setCurrentAction(actionType);
	return BehaviorStatus::RUNNING;
}

BehaviorStatus PerformDesiredAction::tick(Blackboard& kb, float /*dt_s*/) {
	if (kb.activityTime() <= 0.0f) {
		// Time ran out, stop action.
		return BehaviorStatus::SUCCESS;
	}
	kb.setCurrentAction(kb.desiredAction());
	return BehaviorStatus::RUNNING;
}

BehaviorStatus PerformAffordedAction::tick(Blackboard& kb, float /*dt_s*/) {
	auto &patient = kb.interactionTarget();
	if (!patient.object || !patient.affordance || patient.affordanceSlot == -1) {
		// Invalid state when entering this action
		REGEN_WARN("["<<kb.instanceId()<<"] No valid patient or affordance in PerformAffordedAction.");
		return BehaviorStatus::FAILURE;
	}
	if (kb.activityTime() <= 0.0f) {
		// Time ran out, stop action.
		kb.unsetInteractionTarget();
		return BehaviorStatus::SUCCESS;
	}
	if (kb.distanceToPatient() > getReachRadius(patient)) {
		// Not in reach of the affordance -> failure.
		// Usually you want to have PerformAffordedAction in a selection followed by MoveToPatient
		// which will become active in this case.
		return BehaviorStatus::FAILURE;
	}
	// Make afforded action current, this will make the controller play the action animation.
	kb.setCurrentAction(patient.affordance->type);
	return BehaviorStatus::RUNNING;
}

BehaviorStatus FormLocationGroup::tick(Blackboard &kb, float /*dt_s*/) {
	auto &location = kb.currentLocation();
	if (!location) {
		// Invalid state when entering this action
		REGEN_WARN("["<<kb.instanceId()<<"] No current location.");
		return BehaviorStatus::FAILURE;
	}
	if (location->numVisitors() < 2) {
		// Not enough characters at location to form a group.
		return BehaviorStatus::FAILURE;
	}
	if (kb.isPartOfGroup()) {
		// Already in a group, nothing to do.
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Already part of a group.");
#endif
		return BehaviorStatus::SUCCESS;
	}
	Vec2f currentPos2D(kb.currentPosition().x, kb.currentPosition().z);

	// Find a friendly character at the location to form/join a group with.
	// Consider characters that are closest first, for this we need to sort by distance.
	std::vector<std::pair<uint32_t, float>> candidates(location->numVisitors());
	uint32_t candidateIdx = 0;
	for (int32_t characterIdx = 0; characterIdx < location->numVisitors(); characterIdx++) {
		WorldObject *character = location->visitor(characterIdx);
		if (!character) continue;
		float dist = (character->position2D() - currentPos2D).lengthSquared();
		candidates[candidateIdx++] = std::make_pair(characterIdx, dist);
	}
	std::sort(candidates.begin(), candidates.end(),
		[](const std::pair<uint32_t,float> &a, const std::pair<uint32_t,float> &b) {
			return a.second < b.second;
		});

	WorldObject *friendlyCharacter = nullptr;
	for (uint32_t i = 0; i < candidateIdx; i++) {
		WorldObject *character = location->visitor(candidates[i].first);
		if (character == kb.characterObject()) continue;
		if (character->isPartOfGroup()) {
			if (character->currentGroup()->isFull()) {
				// Try next character
				continue;
			}
		}
		friendlyCharacter = character;
		break;
	}
	if (!friendlyCharacter) {
		// No friendly character found to form/join a group with.
		return BehaviorStatus::FAILURE;
	}
	if (friendlyCharacter->isPartOfGroup()) {
		// Join existing group.
		kb.joinGroup(friendlyCharacter->currentGroup());
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Joining existing group with '" << friendlyCharacter->name() << "'.");
#endif
	} else {
		// Form new group.
		kb.formGroup(kb.desiredAction(), friendlyCharacter);
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Forming new group with '" << friendlyCharacter->name() << "'.");
#endif
	}
	return BehaviorStatus::SUCCESS;
}

BehaviorStatus LeaveGroup::tick(Blackboard &kb, float /*dt_s*/) {
	if (kb.isPartOfGroup()) {
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Leaving current group, remaining members: " <<
			kb.currentGroup()->numMembers() - 1 << ".");
#endif
		kb.leaveCurrentGroup();
	}
	return BehaviorStatus::SUCCESS;
}

BehaviorStatus LeaveLocation::tick(Blackboard &kb, float /*dt_s*/) {
	if (kb.currentLocation().get()) {
#ifdef NPC_ACTIONS_DEBUG
		REGEN_INFO("["<<kb.instanceId()<<"] Leaving current location '" << kb.currentLocation()->name() << "'.");
#endif
		kb.unsetCurrentLocation();
	}
	return BehaviorStatus::SUCCESS;
}
