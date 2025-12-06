#ifndef REGEN_BONE_CONTROLLER_H_
#define REGEN_BONE_CONTROLLER_H_

#include <vector>

#include "motion-clip.h"
#include "regen/behavior/skeleton/motion-type.h"
#include "regen/animation/bone-tree.h"
#include "regen/behavior/blackboard.h"
#include "regen/behavior/world/body-part.h"

namespace regen {
	/** An item that contains a bone animation and its associated ranges and clips. */
	struct BoneAnimationItem {
		ref_ptr<BoneTree> boneTree;
		std::vector<AnimationRange> ranges;
		std::vector<MotionClip> clips;
		std::unordered_map<std::string,BodyPart> startNodesOfBodyParts;
	};

	/**
	 * Controls the bone animation for a single character instance.
	 * BoneController manages a set of motion types, each motion type
	 * can have multiple animation clips associated with it.
	 * The controller can blend between multiple active motions,
	 * however usually only one motion is active at a time.
	 * Motions can be started and stopped, and they will fade in and out
	 * over a configurable duration.
	 * The controller also provides a mapping from high-level actions
	 * to motion types, so that a behavior tree can select actions
	 * and the controller will map these to motions and start/stop
	 * the appropriate animations.
	 */
	class BoneController {
	public:
		/**
		 * Create a bone controller for a character instance.
		 * @param instanceIdx The instance index of the character in the BoneTree.
		 * @param animItem The bone animation item that contains the animation and clips.
		 */
		BoneController(uint32_t instanceIdx, const ref_ptr<BoneAnimationItem> &animItem);

		/**
		 * Set the duration for fading motions in and out.
		 * @param seconds The fade duration in seconds.
		 */
		void setMotionFadeDuration(float seconds) { motionFadeDuration_ = seconds; }

		/**
		 * @return The duration for fading motions in and out.
		 */
		float motionFadeDuration() const { return motionFadeDuration_; }

		/**
		 * @param type The motion type.
		 * @return true if the given motion type is currently active.
		 */
		bool isCurrentBoneAnimation(MotionType type) const;

		/**
		 * @param action The action type.
		 * @return true if the given action can be performed by this controller,
		 *         i.e. if there is at least one motion type associated with the action.
		 */
		bool canPerformAction(ActionType action) const {
			return actionToMotion_[static_cast<int>(action)].size() > 0;
		}

		/**
		 * @param type The motion type.
		 * @return The animation handle for the given motion type or -1 if not active.
		 */
		BoneTree::AnimationHandle getAnimationHandle(MotionType type) const;

		/**
		 * Update the bone controller.
		 * This will update the active motions, start/stop animations as needed,
		 * and set animation weights for blending.
		 * @param dt_s The time delta in seconds since the last update.
		 * @param kb The blackboard that provides the current actions.
		 */
		void updateBoneController(float dt_s, const Blackboard &kb);

		/**
		 * Update the bone controller.
		 * This will update the active motions, start/stop animations as needed,
		 * and set animation weights for blending.
		 * @param dt_s The time delta in seconds since the last update.
		 * @param desiredMotions The list of desired motion types.
		 * @param numDesiredMotions The number of desired motion types.
		 */
		void updateBoneController(float dt_s, const MotionType *desiredMotions, uint32_t numDesiredMotions);

		/**
		 * @return The time the character needs for a single walking cycle.
		 */
		float walkTime() const { return walkTime_; }

		/**
		 * @return The time the character needs for a single running cycle.
		 */
		float runTime() const { return runTime_; }

	protected:
		const uint32_t instanceIdx_;
		ref_ptr<BoneAnimationItem> animItem_;

		enum MotionStatus {
			MOTION_INACTIVE = 0,
			MOTION_STARTING,
			MOTION_ACTIVE,
			MOTION_FADING_OUT,
			MOTION_FADING_IN
		};
		enum ClipStatus {
			CLIP_INACTIVE = 0,
			CLIP_BEGINNING,
			CLIP_LOOPING,
			CLIP_ENDING
		};

		// Data for each motion type.
		struct MotionData {
			// All clips for this motion type.
			std::vector<uint32_t> clips;
			// The animation handle in case the motion type has an active animation.
			BoneTree::AnimationHandle handle = -1;
			// The next animation handle when transitioning between segments of a clip.
			BoneTree::AnimationHandle nextHandle = -1;
			// Flag indicating if this motion type is currently desired.
			bool desired = false;
			// Status flag for this motion type.
			MotionStatus status = MOTION_INACTIVE;
			// Current fading progress, only used when fading in or out.
			float fadeTime = 0.0f;
			// Remember the last clip index that was activated for this motion type.
			uint32_t lastClip = 0;
			// Plus the status of the last clip, for advancing it over time.
			ClipStatus clipStatus = CLIP_INACTIVE;
			// Also store the action type that triggered this motion.
			ActionType action = ActionType::LAST_ACTION;
			// Per bone weights for this motion type.
			std::vector<float> boneWeights;
		};
		// Data item for each motion type.
		std::vector<MotionData> motionToData_;
		// Mapping from actions to possible motion types.
		// ie. each action type may be executable by multiple motion types.
		std::vector<std::vector<MotionType>> actionToMotion_;

		// This is the full list of currently active motions also included
		// ones that are currently "fading out" and are not desired.
		std::vector<MotionType> activeMotions_;
		uint32_t numActiveMotions_ = 0;

		// Walking behavior parameters.
		float walkTime_ = 0.0f;
		float runTime_ = 0.0f;

		// Tunable parameters
		float motionFadeDuration_ = 0.25f; // seconds

		void addActiveMotion(MotionType motion);

		void updateBoneController(float dt_s);

		void updateDesiredMotions(const ActionType *desiredActions, uint32_t numDesiredActions);

		void updateDesiredMotion(MotionType desiredMotion, ActionType desiredAction, bool selectNewClip);

		void updateMotionClip(MotionData &motion, float dt_s);

		void updateMotionClip(MotionData &motion, MotionClip &clip);

		bool startMotionClip(MotionData &motion, float initialWeight);

		void stopMotionClip(MotionData &motion);

		void setMotionWeight(MotionData &motion, float weight) const;

		MotionType lastActiveMotion(MotionData &motion) const;

		bool isClipInFinalStage(MotionData &motion) const;

		BoneTree::AnimationHandle startBoneAnimation(MotionData &motion, const AnimationRange *range);

		BodyPart getBodyPartType(const std::string &startNodeName) const;

		void initializeBoneWeights(MotionData &motionData, MotionType type) const;

		void handleAnimationStopped(BoneTree::BoneEvent &eventData);
	};
} // namespace

#endif /* REGEN_NPC_CONTROLLER_H_ */
