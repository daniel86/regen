#ifndef REGEN_AFFORDANCE_H_
#define REGEN_AFFORDANCE_H_
#include "action-type.h"
#include "world-object.h"
#include "regen/shader/shader-input.h"

namespace regen {
	/**
	 * The layout of slots for a discrete affordance.
	 * Each slot represents a position where an NPC can stand
	 * when using the affordance.
	 */
	enum class SlotLayout {
		CIRCULAR = 0,
		GRID,
		CIRCULAR_GRID
	};

	/**
	 * An affordance represents an action that can be performed on or with a world object.
	 * It defines how many NPCs can use the affordance at the same time, and where they
	 * should position themselves when using it.
	 */
	struct Affordance {
		static float reachDistance;
		// The type of action that is afforded
		ActionType type = ActionType::IDLE;
		// Layout of slots around the affordance
		SlotLayout layout = SlotLayout::CIRCULAR;
		// number of NPCs that can use this affordance at the same time
		int slotCount = 1;
		// number of currently free slots
		int freeSlots = 1;
		// for concentric grid layout
		int numRings = 2;
		// Inner exclusion radius around center
		float minDistance = 0.0f;
		// Radius for circular layout
		float radius = 1.0f;
		// Spacing between slots in grid layout
		float spacing = 3.0f;
		// The owner of the affordance
		ref_ptr<WorldObject> owner;
		// Current users of this affordance, size of vector is slotCount
		std::vector<const WorldObject*> users;
		// Base offset from owner position
		Vec3f baseOffset = Vec3f::zero();
		// Name of the affordance
		std::string name;

		/**
		 * Create a new affordance for the given owner.
		 * @param owner the world object that owns this affordance.
		 */
		explicit Affordance(const ref_ptr<WorldObject> &owner);

		/** Initialize the affordance, precomputing slot positions. */
		void initialize();

		/** Check if there is at least one free slot. */
		bool hasFreeSlot() const;

		/** Reserve a slot for the given user.
		 * @param user the world object that wants to use the affordance.
		 * @param randomizeSlot if true, a random free slot will be chosen.
		 * @return the index of the reserved slot, or -1 if no slot is available.
		 */
		int reserveSlot(const WorldObject *user, bool randomizeSlot=false);

		/** Release a previously reserved slot.
		 * @param slotIdx the index of the slot to release.
		 */
		void releaseSlot(int slotIdx);

		/** Get the position of the given slot index in world space.
		 * @param idx the index of the slot.
		 * @return the position of the slot.
		 */
		const Vec3f& slotPosition(int idx) const;

	protected:
		Vec3f computeSlotPosition(int idx) const;
		uint32_t localStamp_ = 0;

		// slot positions in world space.
		mutable std::vector<Vec3f> slotPositions;
		mutable std::vector<uint32_t> slotStamps;

		uint32_t tfStamp() const {
			return owner->hasShape() ? owner->shape()->tfStamp() : localStamp_;
		}
	};

	std::ostream &operator<<(std::ostream &out, const SlotLayout &v);

	std::istream &operator>>(std::istream &in, SlotLayout &v);
} // namespace

#endif /* REGEN_AFFORDANCE_H_ */
