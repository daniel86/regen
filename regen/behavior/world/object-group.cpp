#include "object-group.h"
#include "world-object.h"

using namespace regen;

ObjectGroup::ObjectGroup(ActionType groupActivity, int32_t maxGroupSize)
		: WorldObject(REGEN_STRING(groupActivity<<"-group")),
          groupActivity_(groupActivity),
		  maxGroupSize_(maxGroupSize),
		  groupBounds_(Bounds<Vec3f>::create(Vec3f::zero(), Vec3f::zero())) {
	members_.resize(maxGroupSize_, {});
	objectType_ = ObjectType::COLLECTION;
	// position of the group is the center of its members.
	setPosition(Vec3f::zero());
	// this is the distance to group center where NPC start flocking.
	setRadius(1.0f);
}

int32_t ObjectGroup::addMember(WorldObject *member) {
	if (isFull()) {
		return -1;
	}
	if (hasMember(member)) {
		return member->currentGroupIndex();
	}
	members_[numMembers_] = member;
	member->setCurrentGroupIndex(numMembers_);
	if (numMembers_ == 0) {
		updateGroupPosition();
	}
	return numMembers_++;
}

void ObjectGroup::updateGroupPosition() {
	groupCenter_ = currentLocation()->position3D();
	auto dir = (members_[0]->position3D() - groupCenter_);
	dir.normalize();
	groupCenter_ += dir * currentLocation()->radius() * (0.5f + math::random<float>() * 0.35f);
	groupCenter_.y = members_[0]->position3D().y;
	groupCenter2D_ = Vec2f(groupCenter_.x, groupCenter_.z);
	pos_->setVertex(0, groupCenter_);
	setRadius(currentLocation()->radius() * 0.5f);
}

void ObjectGroup::removeMember(WorldObject *member) {
	// remove member by shifting others down
	int32_t memberIdx = member->currentGroupIndex();
	if (memberIdx == -1) {
		return;
	}
	for (int32_t j = memberIdx; j < numMembers_ - 1; ++j) {
		members_[j] = members_[j + 1];
		members_[j]->setCurrentGroupIndex(j);
	}
	members_[--numMembers_] = {};
	member->setCurrentGroupIndex(-1);
}

bool ObjectGroup::hasMember(const WorldObject *member) const {
	return member->currentGroup().get() == this;
}

bool ObjectGroup::isFull() const {
	return numMembers_ >= maxGroupSize_;
}

bool ObjectGroup::isSpeaker(const WorldObject *member) const {
	return (speakerIdx_ >= 0 && members_[speakerIdx_] == member);
}

void ObjectGroup::setSpeaker(const WorldObject *member, float time_s) {
	speakerIdx_ = member->currentGroupIndex();
	speakerTime_ = time_s;
}

void ObjectGroup::unsetSpeaker() {
	speakerIdx_ = -1;
	speakerTime_ = 0.0f;
}

void ObjectGroup::advanceSpeaker(float dt_s, bool randomize) {
	speakerTime_ -= dt_s;
	if (speakerTime_ <= 0.0f) {
		if (randomize) {
			speakerIdx_ = math::randomInt() % numMembers_;
			speakerTime_ = (math::random<float>() * 10.0f) + 5.0f;
		} else {
			unsetSpeaker();
		}
	}
}
