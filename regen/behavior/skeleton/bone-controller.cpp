#include "bone-controller.h"

#include "regen/behavior/world/body-part.h"

using namespace regen;

//#define BONE_CONTROLLER_DEBUG_ACTION ActionType::ATTACKING
//#define BONE_CONTROLLER_DEBUG_STATE
#if defined(BONE_CONTROLLER_DEBUG_ACTION) && !defined(BONE_CONTROLLER_DEBUG_STATE)
#define BONE_CONTROLLER_DEBUG_STATE
#endif

#ifdef BONE_CONTROLLER_DEBUG_ACTION
#define BONE_CTRL_DEBUG(act, ...) do { if (act == BONE_CONTROLLER_DEBUG_ACTION) {\
	std::stringstream ctrl_ss;\
	ctrl_ss << "[" << instanceIdx_ << "] ";\
	ctrl_ss << "Action " << act << " ";\
	ctrl_ss << __VA_ARGS__;\
	Logging::log(Logging::INFO, ctrl_ss.str(), __FILE__, __LINE__);\
	}} while(0)
#elif defined(BONE_CONTROLLER_DEBUG_STATE)
#define BONE_CTRL_DEBUG(act, ...) do {\
	std::stringstream ctrl_ss;\
	ctrl_ss << "[" << instanceIdx_ << "] ";\
	ctrl_ss << "Action " << act << " " << __VA_ARGS__;\
	Logging::log(Logging::INFO, ctrl_ss.str(), __FILE__, __LINE__);\
	} while(0)
#else
#define BONE_CTRL_DEBUG(act, ...)
#endif

BoneController::BoneController(uint32_t instanceIdx, const ref_ptr<BoneAnimationItem> &animItem)
		: instanceIdx_(instanceIdx), animItem_(animItem) {
	// Fill in the motion clips for each motion type.
	motionToData_.resize(static_cast<size_t>(MotionType::MOTION_LAST));
	for (uint32_t clipIdx = 0; clipIdx < animItem_->clips.size(); ++clipIdx) {
		auto &clip = animItem_->clips[clipIdx];
		auto &motionData = motionToData_[static_cast<int>(clip.motion)];
		motionData.clips.push_back(clipIdx);
	}
	// Initialize bone weights for motions with clips.
	for (size_t i = 0; i < motionToData_.size(); ++i) {
		auto &motionData = motionToData_[i];
		if (!motionData.clips.empty()) {
			initializeBoneWeights(motionData, static_cast<MotionType>(i));
		}
	}

	// Extract some special motion parameters that are useful for navigation.
	if (!motionToData_[static_cast<int>(MotionType::WALK)].clips.empty()) {
		auto walkClip = animItem_->clips[motionToData_[static_cast<int>(MotionType::WALK)].clips[0]];
		auto walkTPS = animItem_->boneTree->ticksPerSecond(walkClip.loop->trackIndex);
		walkTime_ = 1000.0f * (walkClip.loop->range.y - walkClip.loop->range.x) / walkTPS;
	}
	if (!motionToData_[static_cast<int>(MotionType::RUN)].clips.empty()) {
		auto runClip = animItem_->clips[motionToData_[static_cast<int>(MotionType::RUN)].clips[0]];
		auto runTPS = animItem_->boneTree->ticksPerSecond(runClip.loop->trackIndex);
		runTime_ = 1000.0f * (runClip.loop->range.y - runClip.loop->range.x) / runTPS;
	}

	// initialize the action-to-motion mapping.
	// note: behavior tree selects actions, we map here to motions, and have
	//       a set of animation clips for each motion type.
	actionToMotion_.resize(static_cast<size_t>(ActionType::LAST_ACTION));
	for (size_t i = 0; i < actionToMotion_.size(); ++i) {
		actionToMotion_[i] = getMotionTypesForAction(static_cast<ActionType>(i));
	}

	// Connect to animation stopped event such that we can restart the animation
	// in case the corresponding motion is still desired.
	/**
	const auto eventHandler = ref_ptr<LambdaEventHandler>::alloc(
		[&](EventObject *, EventData *data) {
			handleAnimationStopped(*static_cast<BoneTree::BoneEvent *>(data));
		});
	animItem_->boneTree->connect(Animation::ANIMATION_STOPPED, eventHandler);
	**/
}

BodyPart BoneController::getBodyPartType(const std::string &startNodeName) const {
	auto it = animItem_->startNodesOfBodyParts.find(startNodeName);
	if (it != animItem_->startNodesOfBodyParts.end()) {
		return it->second;
	}
	return BodyPart::LAST;
}

void BoneController::handleAnimationStopped(BoneTree::BoneEvent &eventData) {
	const BoneTree::AnimationHandle animHandle = eventData.animHandle;
	const MotionData *stoppedMotion = nullptr;
	// Find which motion this animation handle belongs to.
	for (uint32_t i=0; i < numActiveMotions_; i++) {
		auto &motionData = motionToData_[static_cast<int>(activeMotions_[i])];
		if (motionData.handle == animHandle) {
			stoppedMotion = &motionData;
			break;
		}
	}
	if (stoppedMotion == nullptr) {
		// not found
		return;
	}
	if (stoppedMotion->clipStatus == CLIP_LOOPING) {
		eventData.range->trackIdx_ = eventData.trackIdx;
	}
}

void BoneController::initializeBoneWeights(MotionData &motionData, MotionType motionType) const {
	Stack<std::pair<const BoneNode*,BodyPart>> traversalStack;
	const BoneNode *rootNode = animItem_->boneTree->rootNode();
	traversalStack.push(std::make_pair(rootNode, getBodyPartType(rootNode->name)));

	// Get the body part weights for this motion type.
	const float *partWeights = motionBodyPartWeights(motionType);
	float baseWeight = partWeights[0];
	partWeights += 1; // skip base weight
	// Initialize the bone weights array.
	motionData.boneWeights.resize(animItem_->boneTree->numNodes());

	// Here the idea is that we keep track of the current body part as we traverse
	// such that we can find the correct weight per bone which can later be indexed by node index.
	while (!traversalStack.isEmpty()) {
		auto [node,currentBodyPart] = traversalStack.top();
		traversalStack.pop();
		if (currentBodyPart == BodyPart::LAST) {
			motionData.boneWeights[node->nodeIdx] = baseWeight;
		} else {
			motionData.boneWeights[node->nodeIdx] = partWeights[static_cast<size_t>(currentBodyPart)];
		}

		for (const auto &child : node->children) {
			BodyPart childPartType = getBodyPartType(child->name);
			if (childPartType == BodyPart::LAST) childPartType = currentBodyPart;
			traversalStack.push(std::make_pair(child.get(), childPartType));
		}
	}
}

int32_t BoneController::getAnimationHandle(MotionType type) const {
	return motionToData_[static_cast<int>(type)].handle;
}

bool BoneController::isCurrentBoneAnimation(MotionType type) const {
	for (uint32_t i=0; i<numActiveMotions_; i++) {
		if (activeMotions_[i] == type) return true;
	}
	return false;
}

void BoneController::setMotionWeight(MotionData &motion, float weight) const {
	animItem_->boneTree->setAnimationWeight(instanceIdx_, motion.handle, weight);
}

void BoneController::addActiveMotion(MotionType motion) {
	if (numActiveMotions_ >= activeMotions_.size()) {
		activeMotions_.resize(activeMotions_.size() + 4);
	}
	activeMotions_[numActiveMotions_++] = motion;
}

void BoneController::updateDesiredMotion(
		MotionType desiredMotion, ActionType desiredAction, bool selectNewClip) {
	auto &motionData = motionToData_[static_cast<int>(desiredMotion)];
	motionData.desired = true;
	motionData.action = desiredAction;
	if (selectNewClip && motionData.status != MOTION_INACTIVE) {
		// Select a new clip for this motion type.
		startMotionClip(motionData, 0.0f);
		BONE_CTRL_DEBUG(desiredMotion, "selecting new clip");
	}
	if (motionData.status == MOTION_FADING_OUT) {
		// was fading out, but is now desired again, so make active again.
		// note: we can keep the current weight and fade time.
		BONE_CTRL_DEBUG(desiredMotion, "fading in");
		motionData.status = MOTION_FADING_IN;
	} else if (motionData.status == MOTION_INACTIVE) {
		// was inactive, so we can start it right away.
		BONE_CTRL_DEBUG(desiredMotion, "is now starting");
		motionData.status = MOTION_STARTING;
		addActiveMotion(desiredMotion);
	}
}

void BoneController::updateDesiredMotions(const ActionType *desiredActions, uint32_t numDesiredActions) {
	auto anim = animItem_->boneTree;

	for (uint32_t i = 0; i < numDesiredActions; ++i) {
		MotionType desiredMotion = MotionType::MOTION_LAST;
		ActionType desiredAction = desiredActions[i];
		bool selectNewClip = false;

		// Check if the action is currently being performed.
		for (uint32_t j = 0; j < numActiveMotions_; ++j) {
			MotionType currentMotion = activeMotions_[j];
			auto &motionData = motionToData_[static_cast<int>(currentMotion)];
			if (motionData.action == desiredAction) {
				// For some action types, allow rapid changing of motion types.
				// For now just allow that for ATTACK actions.
				// Later, this should be defined in an ontology!
				bool stickToCurrent = ((
					desiredAction != ActionType::ATTACKING &&
					desiredAction != ActionType::CONVERSING) ||
					anim->isBoneAnimationActive(instanceIdx_, motionData.handle));
				if (stickToCurrent) {
					// The current animation clip segment is still active, or should be
					// repeated.
					desiredMotion = currentMotion;
				} else {
					// Allow selecting a new motion clip for this action.
					// The last clip has already finished as !isBoneAnimationActive.
					selectNewClip = true;
					break;
				}

			}
		}
		// If not active, then select a motion for the action.
		if (desiredMotion == MotionType::MOTION_LAST) {
			auto &possibleMotions = actionToMotion_[static_cast<size_t>(desiredAction)];
			if (!possibleMotions.empty()) {
				if (possibleMotions.size() == 1) {
					desiredMotion = possibleMotions[0];
				} else {
					desiredMotion = possibleMotions[math::randomInt() % possibleMotions.size()];
				}
			} else {
				REGEN_WARN("No motion defined for action " << desiredAction << ".");
				continue;
			}
		}
		if (desiredMotion == MotionType::MOTION_LAST) {
			continue;
		}
		updateDesiredMotion(desiredMotion, desiredAction, selectNewClip);
	}
}

bool BoneController::startMotionClip(MotionData &motion, float initialWeight) {
	if (motion.clips.empty()) return false;

	// Select a clip for this motion type.
	uint32_t clipIndex;
	if (motion.clips.size() == 1) {
		clipIndex = 0;
	} else {
		clipIndex = math::randomInt() % motion.clips.size();
	}
	auto &clip = animItem_->clips[motion.clips[clipIndex]];
	// begin range is optional, if not present we loop the clip directly.
	bool hasBeginRange = (clip.begin != nullptr);

	motion.lastClip = clipIndex;
	motion.clipStatus = (hasBeginRange ? CLIP_BEGINNING : CLIP_LOOPING);

	if (initialWeight + 1e-5f >= 1.0f) {
		initialWeight = 1.0f;
		motion.status = MOTION_ACTIVE;
		motion.fadeTime = motionFadeDuration_;
	} else {
		motion.status = MOTION_FADING_IN;
		motion.fadeTime = 0.0f;
	}
	if (hasBeginRange) {
		motion.handle = startBoneAnimation(motion, clip.begin);
		BONE_CTRL_DEBUG(clip.motion, REGEN_STRING(
			"is starting clip '" << clip.begin->name << "' with begin range " <<
			" in track " << clip.begin->trackIndex));
	} else {
		motion.handle = startBoneAnimation(motion, clip.loop);
		BONE_CTRL_DEBUG(clip.motion, REGEN_STRING(
			"is starting clip '" << clip.loop->name << "' with no begin range" <<
			" in track " << clip.loop->trackIndex));
	}
	setMotionWeight(motion, initialWeight);
	animItem_->boneTree->startAnimation();

	return true;
}

void BoneController::stopMotionClip(MotionData &motion) {
	animItem_->boneTree->stopBoneAnimation(instanceIdx_, motion.handle);
	motion.handle = -1;
	motion.status = MOTION_INACTIVE;
}

BoneTree::AnimationHandle BoneController::startBoneAnimation(
		MotionData &motion, const AnimationRange *range) {
	return animItem_->boneTree->startBoneAnimation(instanceIdx_,
		range->trackIndex, range->range, motion.boneWeights.data());
}

void BoneController::updateMotionClip(MotionData &motion, MotionClip &clip) {
	// animation-status is ACTIVE | FADING_OUT | FADING_IN
	// clip-status is BEGINNING | LOOPING | ENDING
	auto &anim = animItem_->boneTree;
	// Check if the current segment is still active, if so we are done.
	bool isActive = anim->isBoneAnimationActive(instanceIdx_, motion.handle);
	if (isActive) return;

	bool hasEndRange = (clip.end != nullptr);
	bool isFadingOut = (motion.status == MOTION_FADING_OUT);

	if (motion.clipStatus == CLIP_BEGINNING) {
		// Beginning segment is done, now start looping segment if any...
		if (clip.loop && (!isFadingOut || !hasEndRange)) {
			// Activate the looping segment, unless we are fading out and have an end segment.
			// In that case we skip the looping segment and go directly to the end segment.
			motion.handle = startBoneAnimation(motion, clip.loop);
			motion.clipStatus = CLIP_LOOPING;
			BONE_CTRL_DEBUG(clip.motion, "is now looping");
		} else if (clip.end) {
			// Start the ending segment directly.
			if (motion.nextHandle != -1) {
				motion.handle = motion.nextHandle;
				motion.nextHandle = -1;
			} else {
				motion.handle = startBoneAnimation(motion, clip.end);
			}
			motion.clipStatus = CLIP_ENDING;
			BONE_CTRL_DEBUG(clip.motion, "is now ending");
		} else if (!isFadingOut) {
			// Switch back to beginning.
			motion.handle = startBoneAnimation(motion, clip.begin);
			motion.clipStatus = CLIP_BEGINNING;
		} else {
			motion.status = MOTION_INACTIVE;
			BONE_CTRL_DEBUG(clip.motion, "is now inactive");
		}
	}
	else if (motion.clipStatus == CLIP_LOOPING) {
		// Looping segment is done.
		if (!isFadingOut) {
			// Restart the looping segment.
			//BONE_CTRL_DEBUG(clip.motion, "is now restarting");
			motion.handle = startBoneAnimation(motion, clip.loop);
		} else if (clip.end) {
			// Switch to ending segment if we are fading out, i.e. if the motion is no longer desired.
			if (motion.nextHandle != -1) {
				motion.handle = motion.nextHandle;
				motion.nextHandle = -1;
				BONE_CTRL_DEBUG(clip.motion, REGEN_STRING("early faded to ending, time: "
					<< motion.fadeTime / motionFadeDuration_));
			} else {
				motion.handle = startBoneAnimation(motion, clip.end);
				BONE_CTRL_DEBUG(clip.motion, "is now ending");
			}
			motion.clipStatus = CLIP_ENDING;
		} else {
			// No ending segment, so toggle the motion off.
			motion.status = MOTION_INACTIVE;
			BONE_CTRL_DEBUG(clip.motion, "is now inactive");
		}
	}
	else if (motion.clipStatus == CLIP_ENDING) {
		// Ending segment is done, so toggle the motion off.
		motion.status = MOTION_INACTIVE;
		BONE_CTRL_DEBUG(clip.motion, "is now inactive");
	}
}

MotionType BoneController::lastActiveMotion(MotionData &motion) const {
	return animItem_->clips[motion.clips[motion.lastClip]].motion;
}

bool BoneController::isClipInFinalStage(MotionData &motion) const {
	auto &clip = animItem_->clips[motion.clips[motion.lastClip]];
	return (motion.clipStatus == CLIP_ENDING ||
		(motion.clipStatus == CLIP_LOOPING && clip.end == nullptr) ||
		(motion.clipStatus == CLIP_BEGINNING && clip.loop == nullptr && clip.end == nullptr));
}

void BoneController::updateMotionClip(MotionData &motion, float dt_s) {
	// Each clip is made up of one or more segments that are played in sequence.
	if (motion.status != MOTION_INACTIVE && motion.status != MOTION_STARTING) {
		auto &clip = animItem_->clips[motion.clips[motion.lastClip]];
		updateMotionClip(motion, clip);
	}
	// Update the motion state machine and blend weights.
	switch (motion.status) {
		case MOTION_ACTIVE:
		case MOTION_INACTIVE:
			// nothing to do
			break;
		case MOTION_STARTING:
			if (startMotionClip(motion, 0.0f)) {
				BONE_CTRL_DEBUG(lastActiveMotion(motion), "started");
			} else {
				// Could not start motion, so set to inactive again.
				motion.status = MOTION_INACTIVE;
				BONE_CTRL_DEBUG(motion.action, "could not be started");
			}
			break;
		case MOTION_FADING_IN:
			motion.fadeTime += dt_s;
			if (motion.fadeTime >= motionFadeDuration_) {
				BONE_CTRL_DEBUG(lastActiveMotion(motion), "fully faded in");
				motion.status = MOTION_ACTIVE;
				setMotionWeight(motion, 1.0f);
			} else {
				setMotionWeight(motion, motion.fadeTime / motionFadeDuration_);
			}
			break;
		case MOTION_FADING_OUT:
			auto undesiredMotion = lastActiveMotion(motion);
			if (!isInterruptibleMotion(undesiredMotion)) {
				// Avoid interrupting some motions in the middle by fading them out
				// e.g. attack/block animations should play to the end.
				break;
			}

			motion.fadeTime -= dt_s;
			if (motion.fadeTime <= 0.0f) {
				stopMotionClip(motion);
				if (motion.nextHandle != -1) {
					// Start the next handle if any.
					motion.handle = motion.nextHandle;
					motion.nextHandle = -1;
					motion.fadeTime = motionFadeDuration_;
					motion.status = MOTION_FADING_OUT;
					motion.clipStatus = CLIP_ENDING;
					setMotionWeight(motion, 1.0f);
					BONE_CTRL_DEBUG(undesiredMotion, "fading out ending segment");
				} else {
					BONE_CTRL_DEBUG(undesiredMotion, "stopped after fade out");
				}
			} else {
				float w = motion.fadeTime / motionFadeDuration_;
				setMotionWeight(motion, w);
				if (motion.nextHandle != -1) {
					animItem_->boneTree->setAnimationWeight(instanceIdx_, motion.nextHandle, 1.0f - w);
				}
			}
			break;
	}
}

void BoneController::updateBoneController(float dt_s, const Blackboard &kb) {
	// Unset all desired flags.
	for (uint32_t activeIdx = 0; activeIdx < numActiveMotions_; ++activeIdx) {
		auto &motion = motionToData_[static_cast<int>(activeMotions_[activeIdx])];
		motion.desired = false;
	}
	// Map current actions to motions, mark only these as desired.
	if (kb.hasCurrentAction()) {
		updateDesiredMotions(kb.currentActions(), kb.numCurrentActions());
	}
	// Update the state of all active motions.
	updateBoneController(dt_s);
}

void BoneController::updateBoneController(float dt_s, const MotionType *desiredMotions, uint32_t numDesiredMotions) {
	// Note: It is best to use low weights for IDLE state to avoid blending it in too aggressively
	//       when an active motion is set to undesired (eg. due to key release event).
	// Unset all desired flags.
	for (uint32_t activeIdx = 0; activeIdx < numActiveMotions_; ++activeIdx) {
		const auto motionType = activeMotions_[activeIdx];
		auto &motion = motionToData_[static_cast<int>(motionType)];
		motion.desired = false;
	}
	// Mark only the given motions as desired.
	for (uint32_t i = 0; i < numDesiredMotions; ++i) {
		auto desiredMotion = desiredMotions[i];
		updateDesiredMotion(desiredMotion, ActionType::LAST_ACTION, false);
	}
	// Update the state of all active motions.
	updateBoneController(dt_s);
}

void BoneController::updateBoneController(float dt_s) {
	uint32_t newNumActiveMotions = 0;
	for (uint32_t activeIdx = 0; activeIdx < numActiveMotions_; activeIdx++) {
		auto &motion = motionToData_[static_cast<int>(activeMotions_[activeIdx])];
		// Flip some motion state before update
		if (!motion.desired && motion.status != MOTION_FADING_OUT) {
			// Not desired anymore, so fade out.
			BONE_CTRL_DEBUG(activeMotions_[activeIdx], "fading out");
			motion.status = MOTION_FADING_OUT;
			motion.fadeTime = motionFadeDuration_;
			// Fade into the end segment if any.
			auto &clip = animItem_->clips[motion.clips[motion.lastClip]];
			if (motion.clipStatus != CLIP_ENDING && clip.end) {
				motion.nextHandle = startBoneAnimation(motion, clip.end);
				BONE_CTRL_DEBUG(activeMotions_[activeIdx], "starting end segment");
			}
		}
		// Update the motion state, start/stop animations, set animation weights for fading etc.
		updateMotionClip(motion, dt_s);
		// Only add motions that are still active to the new active list.
		// This will remove motions that have faded out completely.
		if (motion.status != MOTION_INACTIVE) {
			activeMotions_[newNumActiveMotions++] = activeMotions_[activeIdx];
		}
	}
	numActiveMotions_ = newNumActiveMotions;
}
