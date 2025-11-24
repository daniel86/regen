#ifndef SCENE_DISPLAY_ANIMATION_H_
#define SCENE_DISPLAY_ANIMATION_H_

#include <regen/animation/bone-tree.h>

using namespace regen;

inline BoneTree::AnimationHandle setAnimationRangeActive(
			const ref_ptr<BoneTree> &anim,
			uint32_t instanceIdx,
			const AnimationRange &animRange) {
	if (animRange.trackName.empty()) {
		return anim->startBoneAnimation(instanceIdx, animRange.trackIndex, animRange.range);
	} else {
		int32_t trackIndex = anim->getTrackIndex(animRange.trackName);
		if (trackIndex < 0) {
			return -1;
		}
		return anim->startBoneAnimation(instanceIdx, trackIndex, animRange.range);
	}
}


class RandomAnimationRangeUpdater : public EventHandler {
public:
	explicit RandomAnimationRangeUpdater(const ref_ptr<BoneTree> &anim) : EventHandler(), anim_(anim) {}

	~RandomAnimationRangeUpdater() override = default;

	void call(EventObject *ev, EventData *data) override {
		BoneTree::BoneEvent *evData = (BoneTree::BoneEvent *) data;
		static const Vec2d fullRange(-1.0, -1.0);
		int index = rand() % anim_->numAnimationTracks();
		anim_->startBoneAnimation(index, evData->instanceIdx, fullRange);
	}

protected:
	ref_ptr<BoneTree> anim_;
};

#endif /* SCENE_DISPLAY_ANIMATION_H_ */
