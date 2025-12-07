#include "blackboard.h"

using namespace regen;

Blackboard::Blackboard(uint32_t instanceId, const Vec3f *currentPos, const Vec3f *currentDir)
		: instanceId_(instanceId),
		  currentPos_(currentPos),
		  currentDir_(currentDir) { // fear decays by 5% per second
	for (int i = 0; i < static_cast<int>(Trait::LAST_TRAIT); i++) {
		traits_[i] = 0.5f;
	}
	actionCapabilities_.resize(static_cast<int>(ActionType::LAST_ACTION), false);
	setAttribute(CharacterAttribute::HEALTH, 1.0f);
	setAttribute(CharacterAttribute::HUNGER, 0.0f);
	setAttribute(CharacterAttribute::STAMINA, 1.0f);
	setAttribute(CharacterAttribute::FEAR, 0.0f);
}

void Blackboard::advanceTime(float dt_s) {
	lingerTime_ -= dt_s;
	activityTime_ -= dt_s;
	// hunger increases over time
	attributes_[static_cast<int>(CharacterAttribute::HUNGER)] =
		std::min(1.0f, hunger() + dt_s * hungerRate_);
	// fear decays over time
	attributes_[static_cast<int>(CharacterAttribute::FEAR)] =
		std::max(0.0f, fear() - dt_s * fearDecayRate_);
}

void Blackboard::setAttribute(CharacterAttribute attr, float value) {
	attributes_[static_cast<int>(attr)] = value;
}

void Blackboard::setInteractionTarget(const ref_ptr<WorldObject> &obj, const ref_ptr<Affordance> &aff, int slotIdx) {
	if (interactionTarget_.object.get()) {
		bool allSame = (interactionTarget_.object.get() == obj.get() &&
			interactionTarget_.affordance.get() == aff.get() &&
			interactionTarget_.affordanceSlot == slotIdx);
		if (allSame) {
			return; // already set
		} else {
			unsetInteractionTarget();
		}
	}
	interactionTarget_.object = obj;
	interactionTarget_.affordance = aff;
	interactionTarget_.affordanceSlot = slotIdx;

	// Update desired location based on interaction target
	if (obj->objectType() == ObjectType::LOCATION) {
		desiredLocation_ = ref_ptr<Location>::staticCast(obj);
	} else {
		const auto &locOfSelected = obj->currentLocation();
		if (locOfSelected.get()) {
			desiredLocation_ = locOfSelected;
		} else {
			desiredLocation_ = {};
		}
	}
}

void Blackboard::unsetInteractionTarget() {
	if (interactionTarget_.affordance.get()) {
		interactionTarget_.affordance->releaseSlot(interactionTarget_.affordanceSlot);
		interactionTarget_.affordance = {};
		interactionTarget_.affordanceSlot = -1;
	}
	interactionTarget_.object = {};
}

void Blackboard::setNavigationTarget(const ref_ptr<WorldObject> &obj, const ref_ptr<Affordance> &aff, int slotIdx) {
	if (navigationTarget_.object.get()) {
		bool allSame = (navigationTarget_.object.get() == obj.get() &&
			navigationTarget_.affordance.get() == aff.get() &&
			navigationTarget_.affordanceSlot == slotIdx);
		if (allSame) {
			return; // already set
		} else {
			unsetNavigationTarget();
		}
	}
	navigationTarget_.object = obj;
	navigationTarget_.affordance = aff;
	navigationTarget_.affordanceSlot = slotIdx;
	if (currentLocation_.get() &&
			obj.get() != currentLocation_.get() &&
			obj->currentLocation().get() != currentLocation_.get()) {
		unsetCurrentLocation();
	}
}

void Blackboard::unsetNavigationTarget() {
	navigationTarget_.affordance = {};
	navigationTarget_.affordanceSlot = -1;
	navigationTarget_.object = {};
}


void Blackboard::unsetDesiredAction() {
	desiredAction_ = ActionType::IDLE;
}

void Blackboard::setCurrentAction(ActionType action) {
	if (numCurrentActions_ >= currentActions_.size()) {
		currentActions_.resize(currentActions_.size() + 4, ActionType::LAST_ACTION);
	}
	currentActions_[numCurrentActions_++] = action;
}

bool Blackboard::isCurrentAction(ActionType action) const {
	for (uint32_t i = 0; i < numCurrentActions_; i++) {
		if (currentActions_[i] == action) {
			return true;
		}
	}
	return false;
}

void Blackboard::addPlaceOfInterest(const ref_ptr<Place> &place)  {
	places_[static_cast<int>(place->placeType())].push_back(place);
}

void Blackboard::unsetCurrentPlace() {
	unsetInteractionTarget();
	unsetCurrentLocation();
	currentPlace_ = {};
}

void Blackboard::setCurrentLocation(const ref_ptr<Location> &loc) {
	unsetCurrentLocation();
	currentLocation_ = loc;
	characterObject_->setCurrentLocation(loc);
	if (currentLocation_.get()) {
		currentLocation_->addVisitor(characterObject_);
	}
}

void Blackboard::unsetCurrentLocation() {
	leaveCurrentGroup();
	if (currentLocation_.get()) {
		currentLocation_->removeVisitor(characterObject_);
		characterObject_->unsetCurrentLocation();
		currentLocation_ = {};
	}
}

void Blackboard::leaveCurrentGroup() {
	auto group = currentGroup();
	if (group.get()) {
		characterObject_->leaveCurrentGroup();
	}
}

void Blackboard::joinGroup(const ref_ptr<ObjectGroup> &group) {
	characterObject_->joinGroup(group);
}

ref_ptr<ObjectGroup> Blackboard::formGroup(ActionType groupActivity, WorldObject *other) {
	auto group = ref_ptr<ObjectGroup>::alloc(groupActivity);
	if (currentLocation_.get()) {
		group->setCurrentLocation(currentLocation_);
		currentLocation_->addGroup(group.get());
	}
	characterObject_->joinGroup(group);
	other->joinGroup(group);
	return group;
}
