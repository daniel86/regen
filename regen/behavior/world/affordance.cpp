#include "affordance.h"

using namespace regen;

float Affordance::reachDistance = 1.0f;

Affordance::Affordance(const ref_ptr<WorldObject> &owner) : owner(owner) {
	// TODO: Also support dynamic world objects here, e.g. chairs that can be moved.
	//         Then it makes sense to compute affordance slot in local space instead.
}

void Affordance::initialize() {
	if (slotCount < 1) slotCount = 1;
	freeSlots = slotCount;
	users.resize(slotCount);
	slotPositions.resize(slotCount);
	for (int32_t i = 0; i < slotCount; i++) {
		users[i] = {};
		slotPositions[i] = computeSlotPosition(i);
	}
}

Vec3f Affordance::computeSlotPosition(int idx) const {
	Vec3f pos, center;
	if (owner->hasShape()) {
		auto tf = owner->shape()->transform();
		if (tf.get() && tf->hasModelMat()) {
			auto mat = tf->modelMat()->getVertex(owner->shape()->instanceID());
			pos = (mat.r ^ baseOffset).xyz();
			center = mat.r.position();
		} else {
			pos = center + baseOffset;
			center = owner->shape()->tfOrigin();
		}
	} else {
		center = owner->position3D();
		pos = center + baseOffset;
	}

	switch (layout) {
		case SlotLayout::CIRCULAR: {
			float angle = (2.0f * M_PIf * static_cast<float>(idx)) /
				static_cast<float>(std::max(1, slotCount));
			pos.x += std::cos(angle) * radius;
			pos.z += std::sin(angle) * radius;
			return pos;
		}

		case SlotLayout::CIRCULAR_GRID: {
			// Concentric circular layers
			int perRing = slotCount / numRings;
			auto ring = static_cast<float>(idx / perRing);
			auto i = static_cast<float>(idx % perRing);

			float layoutRadius = minDistance + ring * spacing;
			float angle = (2.0f * M_PIf * i) / static_cast<float>(perRing);

			pos.x += std::cos(angle) * layoutRadius;
			pos.z += std::sin(angle) * layoutRadius;
			return pos;
		}


		case SlotLayout::GRID: {
			// Compute rows & columns
			int cols = std::max(std::ceil(std::sqrt(slotCount)), 2.0);
			auto row = static_cast<float>(idx / cols);
			auto col = static_cast<float>(idx % cols);

			// Orientation
			Vec3f forward = pos - center;
			forward.y = 0.0;
			float l = forward.length();
			if (l < 1e-3f) {
				forward = Vec3f(0.0f, 0.0f, 1.0f);
			} else {
				forward /= l;
			}
			Vec3f right = forward.cross(Vec3f::up());
			right.normalize();

			// Grid center offset
			float gridWidth = static_cast<float>(cols - 1) * spacing;
			float gridHeight = std::ceil(static_cast<float>(slotCount) /
				static_cast<float>(cols) - 1.0f) * spacing;

			pos = pos - (right * gridWidth) * 0.5f + (forward * gridHeight) * 0.5f;
			return pos + right * (col * spacing) - forward * (row * spacing);
		}
	}
	return center;
}

bool Affordance::hasFreeSlot() const {
	return freeSlots > 0;
}

int Affordance::reserveSlot(const WorldObject *user, bool randomizeSlot) {
	int randomOffset = randomizeSlot ? math::randomInt() % slotCount : 0;
	for (int i = 0; i < slotCount; ++i) {
		int idx = (i + randomOffset) % slotCount;
		if (!users[idx]) {
			users[idx] = user;
			--freeSlots;
			return idx;
		}
	}
	return -1;
}

void Affordance::releaseSlot(int slotIdx) {
	if (slotIdx < 0 || slotIdx >= slotCount) return;
	if (users[slotIdx]) {
		users[slotIdx] = {};
		++freeSlots;
	}
}

std::ostream &regen::operator<<(std::ostream &out, const SlotLayout &v) {
	switch (v) {
		case SlotLayout::CIRCULAR:
			return out << "CIRCULAR";
		case SlotLayout::GRID:
			return out << "GRID";
		case SlotLayout::CIRCULAR_GRID:
			return out << "CIRCULAR_GRID";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, SlotLayout &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "CIRCULAR") v = SlotLayout::CIRCULAR;
	else if (val == "GRID") v = SlotLayout::GRID;
	else if (val == "CIRCULAR_GRID" || val == "CONCENTRIC") v = SlotLayout::CIRCULAR_GRID;
	else {
		REGEN_WARN("Unknown slot layout '" << val << "'. Using CIRCULAR.");
		v = SlotLayout::CIRCULAR;
	}
	return in;
}
