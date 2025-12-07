#ifndef REGEN_BLACKBOARD_H_
#define REGEN_BLACKBOARD_H_
#include "regen/utility/time.h"
#include "world/character-attribute.h"
#include "world/character-trait.h"
#include "world/place.h"
#include "world/location.h"

namespace regen {
	/**
	 * @brief A blackboard encapsulates NPC state, it represents what the NPC "knows" about the world.
	 * This can be used to implement more complex behaviors, e.g. using a behavior tree
	 * or a planning system.
	 */
	class Blackboard {
	public:
		/**
		 * Create a new blackboard.
		 * @param currentPos pointer to the current position of the character.
		 * @param currentDir pointer to the current direction of the character.
		 */
		Blackboard(uint32_t instanceId, const Vec3f *currentPos, const Vec3f *currentDir);

		/**
		 * Get the instance ID of the character this blackboard belongs to.
		 * @return the instance ID.
		 */
		uint32_t instanceId() const { return instanceId_; }

		/**
		 * The object representing the character in the world.
		 */
		void setCharacterObject(WorldObject *obj) { characterObject_ = obj; }

		/**
		 * @return the object representing the character in the world.
		 */
		WorldObject* characterObject() const { return characterObject_; }

		/**
		 * Get the current position of the character.
		 * @return the current position.
		 */
		const Vec3f& currentPosition() const { return *currentPos_; }

		/**
		 * Get the current direction of the character.
		 * @return the current direction.
		 */
		const Vec3f& currentDirection() const { return *currentDir_; }

		/**
		 * Set the world time.
		 * @param worldTime the world time.
		 */
		void setWorldTime(const WorldTime *worldTime) { worldTime_ = worldTime; }

		/**
		 * @return the world time.
		 */
		const WorldTime *worldTime() const { return worldTime_; }

		/**
		 * Set the time (in seconds) the NPC will linger at the current place.
		 * This is typically set when the NPC arrives at a place.
		 * @param time the time in seconds.
		 */
		void setLingerTime(double time) { lingerTime_ = time; }

		/**
		 * Get the remaining time (in seconds) the NPC will linger at the current place.
		 * @return the time in seconds.
		 */
		float lingerTime() const { return lingerTime_; }

		/**
		 * Set the time (in seconds) the NPC will spend on the current activity.
		 * This is typically set when the NPC starts a new activity.
		 * @param time the time in seconds.
		 */
		void setActivityTime(double time) { activityTime_ = time; }

		/**
		 * Get the remaining time (in seconds) the NPC will spend on the current activity.
		 * @return the time in seconds.
		 */
		float activityTime() const { return activityTime_; }

		/**
		 * Set the base time (in seconds) the NPC will spend on an activity.
		 * This is then modulated by personality traits and the type of activity.
		 * @param time the base time in seconds.
		 */
		void setBaseTimeActivity(float time) { baseTimeActivity_ = time; }

		/**
		 * Get the base time (in seconds) the NPC will spend on an activity.
		 * This is then modulated by personality traits and the type of activity.
		 * @return the base time in seconds.
		 */
		float baseTimeActivity() const { return baseTimeActivity_; }

		/**
		 * Set the base time (in seconds) the NPC will spend at a place.
		 * This is then modulated by personality traits and the type of place.
		 * @param time the base time in seconds.
		 */
		void setBaseTimePlace(float time) { baseTimePlace_ = time; }

		/**
		 * Get the base time (in seconds) the NPC will spend at a place.
		 * This is then modulated by personality traits and the type of place.
		 * @return the base time in seconds.
		 */
		float baseTimePlace() const { return baseTimePlace_; }

		/**
		 * Advance the internal time of the blackboard.
		 * @param dt_s the time step in seconds.
		 */
		void advanceTime(float dt_s);

		/**
		 * Set the strength of a personality trait.
		 * @param trait the trait.
		 * @param value the strength of the trait, from 0.0 to 1.0.
		 */
		void setTraitStrength(Trait trait, float value) {
			traits_[static_cast<int>(trait)] = value;
		}

		/**
		 * Get the strength of a personality trait.
		 * @param trait the trait.
		 * @return the strength of the trait, from 0.0 to 1.0.
		 */
		float traitStrength(Trait trait) const {
			return traits_[static_cast<int>(trait)];
		}

		/**
		 * Get the strength of the laziness trait.
		 * @return the strength of the trait, from 0.0 to 1.0.
		 */
		float laziness() const { return traits_[static_cast<int>(Trait::LAZINESS)]; }

		/**
		 * Get the strength of the spirituality trait.
		 * @return the strength of the trait, from 0.0 to 1.0.
		 */
		float spirituality() const { return traits_[static_cast<int>(Trait::SPIRITUALITY)]; }

		/**
		 * Get the strength of the alertness trait.
		 * @return the strength of the trait, from 0.0 to 1.0.
		 */
		float alertness() const { return traits_[static_cast<int>(Trait::ALERTNESS)]; }

		/**
		 * Get the strength of the bravery trait.
		 * @return the strength of the trait, from 0.0 to 1.0.
		 */
		float bravery() const { return traits_[static_cast<int>(Trait::BRAVERY)]; }

		/**
		 * Get the strength of the sociability trait.
		 * @return the strength of the trait, from 0.0 to 1.0.
		 */
		float sociability() const { return traits_[static_cast<int>(Trait::SOCIALABILITY)]; }

		/**
		 * Set a character attribute.
		 * @param attr the attribute.
		 * @param value the value of the attribute, typically from 0.0 to 1.0.
		 */
		void setAttribute(CharacterAttribute attr, float value);

		/**
		 * Get a character attribute.
		 * @param attr the attribute.
		 * @return the value of the attribute, typically from 0.0 to 1.0.
		 */
		float getAttribute(CharacterAttribute attr) const { return attributes_[static_cast<int>(attr)]; }

		/**
		 * Get the health attribute.
		 * @return the health, from 0.0 (dead) to 1.0 (full health).
		 */
		float health() const { return getAttribute(CharacterAttribute::HEALTH); }

		/**
		 * Get the hunger attribute.
		 * @return the hunger, from 0.0 (full) to 1.0 (starving).
		 */
		float hunger() const { return getAttribute(CharacterAttribute::HUNGER); }

		/**
		 * The rate at which hunger increases over time.
		 * This is typically a small positive value, e.g. 0.01 means that hunger increases by 0.01 per second.
		 * @return the hunger rate.
		 */
		void setHungerRate(float rate) { hungerRate_ = rate; }

		/**
		 * The rate at which fear decays over time.
		 * This is typically a small positive value, e.g. 0.01 means that fear decreases by 0.01 per second.
		 * @return the fear decay rate.
		 */
		void setFearDecayRate(float rate) { fearDecayRate_ = rate; }

		/**
		 * Get the stamina attribute.
		 * @return the stamina, from 0.0 (exhausted) to 1.0 (full stamina).
		 */
		float stamina() const { return getAttribute(CharacterAttribute::STAMINA); }

		/**
		 * Get the fear attribute.
		 * @return the fear, from 0.0 (no fear) to 1.0 (terrified).
		 */
		float fear() const { return getAttribute(CharacterAttribute::FEAR); }

		/**
		 * Set the current patient object the NPC is interacting with.
		 * @param obj the patient object.
		 * @param aff the affordance being used.
		 * @param slot the slot index being used.
		 */
		void setInteractionTarget(const ref_ptr<WorldObject> &obj, const ref_ptr<Affordance> &aff, int slot);

		/**
		 * Set the current distance to the patient object the NPC is interacting with.
		 * This is typically set by the motion controller.
		 * @param distance the distance to the patient object.
		 */
		void setDistanceToPatient(float distance) { interactionTarget_.currentDistance = distance; }

		/**
		 * Get the current distance to the patient object the NPC is interacting with.
		 * @return the distance to the patient object.
		 */
		float distanceToPatient() const { return interactionTarget_.currentDistance; }

		/**
		 * Set the current patient object the NPC is interacting with.
		 * @param patient the patient object.
		 */
		void setInteractionTarget(const Patient &patient) {
			setInteractionTarget(patient.object, patient.affordance, patient.affordanceSlot);
		}

		/**
		 * Unset the current patient object the NPC is interacting with.
		 */
		void unsetInteractionTarget();

		/**
		 * Get the current patient object the NPC is interacting with.
		 * @return the patient object.
		 */
		Patient& interactionTarget() { return interactionTarget_; }

		/**
		 * Get the current patient object the NPC is interacting with.
		 * @return the patient object.
		 */
		const Patient& interactionTarget() const { return interactionTarget_; }

		/**
		 * Check if the NPC has a patient object.
		 * @return true if the NPC has a patient object, false otherwise.
		 */
		bool hasInteractionTarget() const { return interactionTarget_.object.get() != nullptr; }

		/**
		 * Set the current navigation target the NPC is moving towards.
		 * @param target the target object.
		 * @param aff the affordance being used, or nullptr if not applicable.
		 * @param slotIdx the slot index being used, or -1 if not applicable.
		 */
		void setNavigationTarget(const ref_ptr<WorldObject> &target, const ref_ptr<Affordance> &aff, int slotIdx);

		/**
		 * Set the current distance to the navigation target the NPC is moving towards.
		 * This is typically set by the motion controller.
		 * @param distance the distance to the navigation target.
		 */
		void setDistanceToTarget(float distance) { navigationTarget_.currentDistance = distance; }

		/**
		 * Get the current distance to the navigation target the NPC is moving towards.
		 * @return the distance to the navigation target.
		 */
		float distanceToTarget() const { return navigationTarget_.currentDistance; }

		/**
		 * Set the current navigation target the NPC is moving towards.
		 * @param patient the target object.
		 */
		void setNavigationTarget(const Patient &patient) {
			setNavigationTarget(patient.object, patient.affordance, patient.affordanceSlot);
		}

		/**
		 * Unset the current navigation target the NPC is moving towards.
		 */
		void unsetNavigationTarget();

		/**
		 * Get the current navigation target the NPC is moving towards.
		 * @return the navigation target.
		 */
		Patient& navigationTarget() { return navigationTarget_; }

		/**
		 * Get the current navigation target the NPC is moving towards.
		 * @return the navigation target.
		 */
		const Patient& navigationTarget() const { return navigationTarget_; }

		/**
		 * Check if the NPC has a navigation target.
		 * @return true if the NPC has a navigation target, false otherwise.
		 */
		bool hasNavigationTarget() const { return navigationTarget_.object.get() != nullptr; }

		/**
		 * Set the desired activity the NPC wants to perform.
		 * This is typically set when the NPC wants to perform an activity that requires
		 * a specific affordance, e.g. sleeping on a bed.
		 * @param action the desired activity.
		 */
		void setDesiredAction(ActionType action) { desiredAction_ = action; }

		/**
		 * Get the desired activity the NPC wants to perform.
		 * @return the desired activity.
		 */
		ActionType desiredAction() const { return desiredAction_; }

		/**
		 * Unset the desired activity the NPC wants to perform.
		 */
		void unsetDesiredAction();

		/**
		 * Set the current action the NPC is performing.
		 * This is typically set when the NPC starts an activity, e.g. navigating to a place.
		 * @param action the current action.
		 */
		void setCurrentAction(ActionType action);

		/**
		 * Get the current action the NPC is performing.
		 * @return the current action.
		 */
		const ActionType* currentActions() const { return currentActions_.data(); }

		/**
		 * Unset the current action the NPC is performing.
		 */
		void unsetCurrentActions() { numCurrentActions_ = 0; }

		/**
		 * Check if the NPC is performing a given action.
		 * @param action the action to check.
		 * @return true if the NPC is performing the given action, false otherwise.
		 */
		bool isCurrentAction(ActionType action) const;

		/**
		 * Check if the NPC has any current action.
		 * @return true if the NPC has a current action, false otherwise.
		 */
		bool hasCurrentAction() const { return numCurrentActions_ > 0; }

		/**
		 * Get the number of current actions the NPC is performing.
		 * @return the number of current actions.
		 */
		uint32_t numCurrentActions() const { return numCurrentActions_; }

		/**
		 * Check if the NPC can perform a given action.
		 * This checks if the action is valid and if the NPC is not already performing it.
		 * @param action the action to check.
		 * @return true if the NPC can perform the given action, false otherwise.
		 */
		bool canPerformAction(ActionType action) const {
			return actionCapabilities_[static_cast<int>(action)];
		}

		/**
		 * Set if the NPC can perform a given action.
		 * @param action the action to set.
		 * @param canPerform true if the NPC can perform the action, false otherwise.
		 */
		void setActionCapability(ActionType action, bool canPerform) {
			actionCapabilities_[static_cast<int>(action)] = canPerform;
		}

		/**
		 * Check if the NPC is currently performing a navigation related action.
		 * This includes navigating, patrolling, and strolling.
		 * @return true if the NPC is performing a navigation related action, false otherwise.
		 */
		bool isNavigationActive() const {
			return isCurrentAction(ActionType::NAVIGATING) ||
					isCurrentAction(ActionType::PATROLLING) ||
					isCurrentAction(ActionType::STROLLING);
		}

		/**
		 * Check if the NPC needs to have a weapon drawn for its current action.
		 * This is true if the NPC is attacking or blocking.
		 * @return true if the NPC needs a weapon, false otherwise.
		 */
		bool isWeaponRequired() const {
			return isCurrentAction(ActionType::ATTACKING) || isCurrentAction(ActionType::BLOCKING);
		}

		/**
		 * Add a place of interest for the NPC.
		 * @param place the place.
		 */
		void addPlaceOfInterest(const ref_ptr<Place> &place);

		/**
		 * Get all places of interest of a given type.
		 * @param type the place type.
		 * @return a vector of places of the given type.
		 */
		const std::vector<ref_ptr<Place>> &getPlacesByType(PlaceType type) const {
			return places_[static_cast<int>(type)];
		}

		/**
		 * The NPC's home place.
		 * @return the home place.
		 */
		const ref_ptr<Place> &homePlace() const { return homePlace_; }

		/**
		 * Set the NPC's home place.
		 * @param place the home place.
		 */
		void setHomePlace(const ref_ptr<Place> &place) { homePlace_ = place; }

		/**
		 * Check if the NPC is at home.
		 * @return true if the NPC is at home, false otherwise.
		 */
		bool atHome() const {
			return (homePlace_.get() != nullptr && currentPlace_.get() == homePlace_.get());
		}

		/**
		 * The current place the NPC is at.
		 * @return the current place, or nullptr if the NPC is not at any place.
		 */
		const ref_ptr<Place> &currentPlace() const { return currentPlace_; }

		/**
		 * Set the current place the NPC is at.
		 * @param place the current place.
		 */
		void setCurrentPlace(const ref_ptr<Place> &place) { currentPlace_ = place; }

		/**
		 * Unset the current place the NPC is at.
		 */
		void unsetCurrentPlace();

		/**
		 * Check if the NPC is at a place of a given type.
		 * @param type the place type.
		 * @return true if the NPC is at a place of the given type, false otherwise.
		 */
		bool atPlaceWithType(PlaceType type) const {
			return (currentPlace_.get() != nullptr && currentPlace_->placeType() == type);
		}

		/**
		 * The target place the NPC is going to.
		 * @return the target place, or nullptr if the NPC has no target place.
		 */
		void setTargetPlace(const ref_ptr<Place> &place) { targetPlace_ = place; }

		/**
		 * The target place the NPC is going to.
		 * @return the target place, or nullptr if the NPC has no target place.
		 */
		const ref_ptr<Place> &targetPlace() const { return targetPlace_; }

		/**
		 * @return The location where the NPC currently is (may be nullptr).
		 */
		const ref_ptr<Location> &currentLocation() const { return currentLocation_; }

		/**
		 * @return The desired location the NPC wants to go to (may be nullptr).
		 */
		const ref_ptr<Location> &desiredLocation() const { return desiredLocation_; }

		/**
		 * Set the location where the NPC currently is (may be nullptr).
		 * This is usually a constrained area within a place, e.g. a room in a building.
		 * @param loc the current location.
		 */
		void setCurrentLocation(const ref_ptr<Location> &loc);

		/**
		 * Unset the location where the NPC currently is.
		 */
		void unsetCurrentLocation();

		/**
		 * Leave the current social group, if any.
		 */
		void leaveCurrentGroup();

		/**
		 * Join a social group.
		 * If the NPC is already part of a social group, it will leave it first.
		 * @param group the social group to join.
		 */
		void joinGroup(const ref_ptr<ObjectGroup> &group);

		/**
		 * Form a new social group with another character.
		 * The NPC will leave its current social group, if any.
		 * The other character will also leave its current social group, if any.
		 * @param groupActivity the activity of the social group.
		 * @param other the other character to form the social group with.
		 * @return the new social group, or nullptr if the other character is nullptr.
		 */
		ref_ptr<ObjectGroup> formGroup(ActionType groupActivity, WorldObject *other);

		/**
		 * Get the social group the NPC is currently part of, if any.
		 * @return the social group, or nullptr if the NPC is not part of any social group.
		 */
		bool isPartOfGroup() const { return characterObject_->isPartOfGroup(); }

		/**
		 * Get the social group the NPC is currently part of, if any.
		 * @return the social group, or nullptr if the NPC is not part of any social group.
		 */
		const ref_ptr<ObjectGroup> &currentGroup() const { return characterObject_->currentGroup(); }

	protected:
		const uint32_t instanceId_ = 0;
		WorldObject *characterObject_;
		const Vec3f *currentPos_ = nullptr;
		const Vec3f *currentDir_ = nullptr;
		const WorldTime *worldTime_ = nullptr;

		// Capabilities of the NPC, i.e. which actions it can perform.
		std::vector<bool> actionCapabilities_;

		// Base times for activities and places.
		// The actual time spent is modulated by personality traits and the type of activity/place
		float baseTimeActivity_ = 20.0f; // in seconds
		float baseTimePlace_ = 240.0f; // in seconds
		float lingerTime_ = 0.0f;
		float activityTime_ = 0.0f;

		// Some high level character traits. These are used for weighting options.
		std::array<float, static_cast<int>(Trait::LAST_TRAIT)> traits_;
		// The character attributes. These represent the current state of the character.
		std::array<float, static_cast<int>(CharacterAttribute::LAST)> attributes_;
		float hungerRate_ = 0.01f; // hunger increase per second
		float fearDecayRate_ = 0.05f; // fear decay per second

		// Places of interest.
		std::array<std::vector<ref_ptr<Place>>, static_cast<int>(PlaceType::LAST)> places_;
		// NPC's home place.
		ref_ptr<Place> homePlace_;
		// The place where the NPC is going to.
		ref_ptr<Place> targetPlace_;
		// The place where the NPC currently is.
		ref_ptr<Place> currentPlace_;
		// The location where the NPC currently is (may be nullptr).
		// This is usually a constrained area within a place, e.g. a room in a building.
		ref_ptr<Location> currentLocation_;
		// The desired location the NPC wants to go to (may be nullptr).
		ref_ptr<Location> desiredLocation_;

		// Perceived objects.
		struct PerceivedObject {
			ref_ptr<WorldObject> object;
			float distance = std::numeric_limits<float>::max();
		};
		PerceivedObject closestEnemy_;
		PerceivedObject closestFriend_;
		PerceivedObject closestDanger_;
		PerceivedObject closestPlayer_;

		// The current patient the NPC is interacting with including
		// the affordance used by the agent.
		// Here this is used to imply intention of the NPC, e.g. that it wants to sleep is indicated
		// by using an affordance for sleeping offered by some kind of bed object.
		Patient interactionTarget_;
		Patient navigationTarget_;
		// The desired activity the NPC wants to perform next.
		ActionType desiredAction_ = ActionType::LAST_ACTION;
		// The actions the NPC is currently performing.
		std::vector<ActionType> currentActions_;
		uint32_t numCurrentActions_ = 0;
	};
} // namespace

#endif /* REGEN_BLACKBOARD_H_ */
