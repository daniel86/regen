#include "regen/behavior/npc-controller.h"

using namespace regen;

//#define NAV_CTRL_DEBUG_WPS

NonPlayerCharacterController::NonPlayerCharacterController(
	const ref_ptr<Mesh> &mesh,
	const Indexed<ref_ptr<ModelTransformation> > &tfIndexed,
	const ref_ptr<BoneAnimationItem> &animItem,
	const ref_ptr<WorldModel> &world)
	: NavigationController(tfIndexed, world),
	  knowledgeBase_(tfIndexed.index, &currentPos_, &currentDir_) {
	boneController_ = ref_ptr<BoneController>::alloc(tfIndexed.index, animItem);
	animItem_ = animItem;

	// randomize a bit to avoid all NPCs deciding and perceiving in
	// the same frame.
	timeSinceLastDecision_s_ = math::random<float>() * decisionInterval_s_;
	timeSinceLastPerception_s_ = math::random<float>() * perceptionInterval_s_;

	// initialize action capabilities
	for (uint32_t actionIdx = 0; actionIdx < static_cast<uint32_t>(ActionType::LAST_ACTION); actionIdx++) {
		ActionType action = static_cast<ActionType>(actionIdx);
		if (boneController_->canPerformAction(action)) {
			knowledgeBase_.setActionCapability(action, true);
		}
	}

	pathPlanner_ = ref_ptr<PathPlanner>::alloc();
	// add all places of interest and way points from the world model
	for (const auto &p: worldModel_->places) {
		knowledgeBase_.addPlaceOfInterest(p);
	}
	for (const auto &wp: worldModel_->wayPoints) {
		addWayPoint(wp);
	}
	for (const auto &conn: worldModel_->wayPointConnections) {
		addConnection(conn.first, conn.second);
	}
	// pick a home
	auto &homePlaces = knowledgeBase_.getPlacesByType(PlaceType::HOME);
	if (!homePlaces.empty()) {
		knowledgeBase_.setHomePlace(homePlaces[math::randomInt() % homePlaces.size()]);
	}
}

NonPlayerCharacterController::~NonPlayerCharacterController() {
	if (perceptionSystem_.get()) {
		perceptionSystem_->removeMonitor(this);
	}
}

void NonPlayerCharacterController::addWayPoint(const ref_ptr<WayPoint> &wp) {
	pathPlanner_->addWayPoint(wp);
}

void NonPlayerCharacterController::addConnection(
	const ref_ptr<WayPoint> &from,
	const ref_ptr<WayPoint> &to,
	bool bidirectional) {
	pathPlanner_->addConnection(from, to, bidirectional);
}

void NonPlayerCharacterController::setPerceptionSystem(std::unique_ptr<PerceptionSystem> ps) {
	if (perceptionSystem_.get()) {
		perceptionSystem_->removeMonitor(this);
	}
	perceptionSystem_ = std::move(ps);
	perceptionSystem_->addMonitor(this);
}

void NonPlayerCharacterController::setBehaviorTree(std::unique_ptr<BehaviorTree::Node> rootNode) {
	behaviorTree_ = std::make_unique<BehaviorTree>(std::move(rootNode));
}

static ref_ptr<WayPoint> pickArrivalWP(
	const ref_ptr<Place> &target, const Vec2f &fromPos) {
	const float desiredDistanceToCenter = target->radius() * 0.75f;
	Vec2f pos = target->position2D();
	// Direction from place center to source position
	Vec2f dir = fromPos - pos;
	dir.y = 0.0f;
	float l = dir.length();
	if (l < desiredDistanceToCenter) {
		// The source position is already within the desired distance to the center.
		return {};
	} else {
		dir /= l;
	}
	// move from center towards source position
	pos += dir * desiredDistanceToCenter;

	auto arrivalWP = ref_ptr<WayPoint>::alloc(target->name(), target->position3D());
	arrivalWP->setRadius(target->radius() * 0.5f);
	return arrivalWP;
}

uint32_t NonPlayerCharacterController::findClosestWP(const std::vector<ref_ptr<WayPoint> > &wps) const {
	if (wps.empty()) return 0;
	float bestDist = std::numeric_limits<float>::max();
	uint32_t bestIdx = 0;
	for (uint32_t i = 0; i < wps.size(); i++) {
		auto &wp = wps[i];
		float dist = (wp->position2D() - Vec2f(currentPos_.x, currentPos_.z)).lengthSquared();
		if (dist < bestDist) {
			bestDist = dist;
			bestIdx = i;
		}
	}
	return bestIdx;
}

bool NonPlayerCharacterController::startNavigate(bool loopPath, bool advancePath) {
	auto &kb = knowledgeBase_;
	if (!currentPath_.empty()) {
		// Reached target point
		if (advancePath) {
			currentPathIndex_++;
		}
		if (loopPath && currentPathIndex_ >= currentPath_.size()) {
			currentPathIndex_ = 0;
		}
		if (currentPathIndex_ < currentPath_.size()) {
			// Move to next point on path
#ifdef NAV_CTRL_DEBUG_WPS
			REGEN_INFO("[" << tfIdx_ << "] Navigating to waypoint " <<
			           currentPath_[currentPathIndex_]->name() <<
			           " (" << (currentPathIndex_ + 1) << "/" << currentPath_.size() << ")");
#endif
			return updatePathNPC();
		} else if (advancePath) {
			currentPathIndex_ = 0;
			currentPath_.clear();
			return false;
		}
	}
	auto &currentPlace = kb.currentPlace();
	auto &targetPlace = kb.targetPlace();
	if (currentPlace.get() && currentPlace.get() == targetPlace.get()) {
		// Already at target place.
		// For now we don't do local navigation planning within a place,
		// but just go directly to the target.
		return updatePathNPC();
	}
	if (!targetPlace) {
		if (!kb.hasNavigationTarget()) {
			REGEN_WARN("No navigation target set for NPC.");
			return false;
		} else {
			return updatePathNPC();
		}
	}
	auto currentPos2D = Vec2f(currentPos_.x, currentPos_.z);
	// Compute a path to the target.
	currentPath_ = pathPlanner_->findPath(currentPos2D, targetPlace->position2D());
	// Add a waypoint within the target place which is close to the last waypoint.
	if (currentPath_.empty()) {
		if (currentPlace.get() != targetPlace.get()) {
			// We are not at the target place yet, so add the arrival waypoint.
			auto arrivalWP = pickArrivalWP(targetPlace, currentPos2D);
			if (arrivalWP.get()) currentPath_ = {arrivalWP};
		}
	} else {
		auto &lastWP = currentPath_.back();
		auto arrivalWP = pickArrivalWP(targetPlace, lastWP->position2D());
		if (arrivalWP.get()) currentPath_.push_back(arrivalWP);
	}
	currentPathIndex_ = 0;
#ifdef NAV_CTRL_DEBUG_WPS
	REGEN_INFO("[" << tfIdx_ << "] Planned path to place " << targetPlace->name()
	           << " num WPs: " << currentPath_.size());
#endif
	return updatePathNPC();
}

Vec2f NonPlayerCharacterController::pickTravelPosition(const WorldObject &wp) const {
	// Base target position (waypoint center)
	Vec2f baseTarget = wp.position2D();
	if (wp.radius() < 0.1f) {
		return baseTarget;
	} else {
		// Pick an angle roughly facing *towards* the waypoint from the NPC's current position
		Vec2f toWP = baseTarget - Vec2f(currentPos_.x, currentPos_.z);
		float angle = atan2(toWP.y, toWP.x) + M_PI;
		// Allow a small angular variation (+-45°)
		angle += (math::random<float>() - 0.5f) * (M_PI / 2.0f);
		const float distanceToWP = wp.radius() * (0.7f * math::random<float>() + 0.1f);
		// Offset final target
		return baseTarget + Vec2f(cos(angle), sin(angle)) * distanceToWP;
	}
}

Vec2f NonPlayerCharacterController::pickTargetPosition(const Patient &navTarget) const {
	if (!navTarget.object) {
		// TODO: Pick a random position at the current place.
		REGEN_WARN("No navigation target object set.");
		return Vec2f(currentPos_.x, currentPos_.z);
	} else if (!navTarget.affordance) {
		return pickTravelPosition(*navTarget.object.get());
	} else {
		auto &slotPos =
				navTarget.affordance->slotPosition(navTarget.affordanceSlot);
		return Vec2f(slotPos.x, slotPos.z);
	}
}

bool NonPlayerCharacterController::updatePathNPC() {
	Vec2f source(currentPos_.x, currentPos_.z);

	if (currentPath_.size() < 2 || currentPathIndex_ + 1 >= currentPath_.size()) {
		// Navigation actions should have a navigation target set.
		auto &navTarget = knowledgeBase_.hasNavigationTarget()
			                  ? knowledgeBase_.navigationTarget()
			                  : knowledgeBase_.interactionTarget();

		if (!currentPath_.empty()) {
			auto &lastWP = currentPath_.back();
			startApproaching(source, pickTravelPosition(*lastWP.get()));
		} else if (navTarget.object->objectType() == ObjectType::PLACE) {
			// Travel to place, prefer to use pre-computed waypoint but fallback to
			//  going to a computed arrival position within the place.
			startApproaching(source, pickTravelPosition(*navTarget.object.get()));
		} else {
			// Travel to object (with affordance slot if any)
			Vec2f navPos = pickTargetPosition(navTarget);
#ifdef NAV_CTRL_DEBUG_WPS
			REGEN_INFO("[" << tfIdx_ << "] Navigating to object " << navTarget.object->name() << ".");
#endif

			// NPC shall look from affordance slot to center of attention
			// (i.e. affordance target, or owner of affordance)
			Vec2f dir = (navTarget.object->position2D() - navPos);
			float dist = dir.length();
			if (dist < affordanceSlotRadius_) {
				dir = Vec2f(cos(currentDir_.x + baseOrientation_), sin(currentDir_.x + baseOrientation_));
			} else {
				dir /= dist;
			}

			float o = atan2(dir.y, dir.x) - baseOrientation_;
			startApproaching(source, navPos, Vec3f(o, 0.0f, 0.0f));
		}
	} else {
		auto &nextWP = currentPath_[currentPathIndex_];
		startApproaching(source, pickTravelPosition(*nextWP.get()));
	}
	return true;
}

void NonPlayerCharacterController::updateNavigationBehavior() {
	auto &kb = knowledgeBase_;

	bool hasNavigatingAction = kb.isCurrentAction(ActionType::NAVIGATING);
	bool hasPatrollingAction = kb.isCurrentAction(ActionType::PATROLLING);
	bool hasStrollingAction = kb.isCurrentAction(ActionType::STROLLING);
	bool hasFlockingAction = kb.isCurrentAction(ActionType::FLOCKING);
	bool hasApproachLoop = (hasPatrollingAction || hasStrollingAction);
	bool hasApproachAction = (hasNavigatingAction || hasApproachLoop);
	bool hasAnyNavAction = hasApproachAction || hasFlockingAction;

	if (!hasAnyNavAction) {
		// No navigation action active, nothing to do.
		currentPath_.clear();
		stopNavigation();
		return;
	}

	bool hasApproachChanged = (
		hasNavigatingAction != hasNavigatingAction_ ||
		hasPatrollingAction != hasPatrollingAction_ ||
		hasStrollingAction != hasStrollingAction_
	);
	if (hasApproachChanged && hasApproachAction) {
		// Make sure to reset path in case the navigation mode changed.
		// e.g. when NAVIGATE is still active when PATROL is selected,
		// we might end up with continuing with NAVIGATE path is we don't clear
		// here.
		currentPath_.clear();
		// Also unset the navigation target in case of looped navigation
		if (hasApproachLoop) {
			kb.unsetNavigationTarget();
		}
	}
	hasNavigatingAction_ = hasNavigatingAction;
	hasPatrollingAction_ = hasPatrollingAction;
	hasStrollingAction_ = hasStrollingAction;

	if (kb.isCurrentAction(ActionType::FLOCKING)) {
		auto &group = kb.currentGroup();
		if (!group) {
			REGEN_WARN("No group for flocking action.");
		} else if (!isNavigationFlocking()) {
			startFlocking(group);
		}
	} else if (isNavigationFlocking()) {
		stopFlocking();
	}

	if (!hasApproachAction || isNavigationApproaching()) {
	} else if (hasApproachLoop && currentPath_.empty() && kb.currentPlace().get()) {
		// We are patrolling or strolling, but have no path yet, so start one.
		auto &currentPlace = kb.currentPlace();
		auto loopPath = currentPlace->getPathWays(
			hasPatrollingAction ? PathwayType::PATROL : PathwayType::STROLL);
		// Pick a random path
		if (!loopPath.empty()) {
			currentPath_ = loopPath[math::randomInt() % loopPath.size()];
			currentPathIndex_ = findClosestWP(currentPath_);
			startNavigate(true, false);
		} else {
			REGEN_WARN("No pathways of type " << (hasPatrollingAction ? "PATROL" : "STROLL") <<
				" at place " << currentPlace->name());
			setIdle();
		}
	} else {
		// The TF animation has no active frame, advance to next waypoint if any.
		if (!currentPath_.empty() && (hasApproachLoop || currentPathIndex_ < currentPath_.size())) {
			// Set next TF waypoint for movement
			startNavigate(hasApproachLoop, true);
		} else {
			// No waypoint remaining, but navigation action is still active,
			// so it seems we need to re-plan.
			currentPath_.clear();
			startNavigate(false, false);
		}
	}
}

void NonPlayerCharacterController::updateKnowledgeBase(float dt_s) {
	auto &kb = knowledgeBase_;
	kb.advanceTime(dt_s);

	// Re-compute distance to patient and target
	if (kb.hasInteractionTarget()) {
		// Update distance to patient
		auto &target = kb.interactionTarget();
		Vec2f delta;
		if (!target.affordance || target.affordanceSlot < 0) {
			// No affordance, so just go to the center of the object
			delta = target.object->position2D() - Vec2f(currentPos_.x, currentPos_.z);
		} else {
			// Go to the affordance slot position
			auto &slotPos = target.affordance->slotPosition(target.affordanceSlot);
			delta = Vec2f(slotPos.x, slotPos.z) - Vec2f(currentPos_.x, currentPos_.z);
		}
		kb.setDistanceToPatient(delta.length());
		// Also let motion controller know the shape of the patient, such that it can
		// ignore collisions with it.
		if (patientShape_.get() != target.object->shape().get()) {
			patientShape_ = target.object->shape();
		}
	} else if (patientShape_.get()) {
		// No patient anymore
		patientShape_ = {};
		kb.setDistanceToPatient(std::numeric_limits<float>::max());
	}
	if (kb.hasNavigationTarget()) {
		auto &navTarget = kb.navigationTarget();
		if (navTarget.object.get() == kb.interactionTarget().object.get() &&
			navTarget.affordance.get() == kb.interactionTarget().affordance.get() &&
			navTarget.affordanceSlot == kb.interactionTarget().affordanceSlot) {
			// Navigation target is the same as the patient, so reuse distance to patient.
			kb.setDistanceToTarget(kb.distanceToPatient());
			} else {
				auto delta = navTarget.object->position2D() - Vec2f(currentPos_.x, currentPos_.z);
				kb.setDistanceToTarget(delta.length());
			}
		// Also make the distance accessible to motion controller, as it does not have access to the blackboard.
		distanceToTarget_ = kb.distanceToTarget();
	}
}

void NonPlayerCharacterController::setIdle() {
	auto &kb = knowledgeBase_;
	kb.setCurrentAction(ActionType::IDLE);
	kb.setActivityTime(kb.baseTimeActivity() * 0.05f * math::random<float>());
	kb.unsetInteractionTarget();
	currentPath_.clear();
}

void NonPlayerCharacterController::updateController(double dt) {
	auto &kb = knowledgeBase_;
	float dt_s = dt / 1000.0f;
	lastDT_ = dt_s;
	timeSinceLastDecision_s_ += dt_s;
	timeSinceLastPerception_s_ += dt_s;

	if (perceptionSystem_.get() && timeSinceLastPerception_s_ > perceptionInterval_s_) {
		// Insert new percepts into the knowledge base, and update existing ones.
		perceptionSystem_->update(knowledgeBase_, timeSinceLastPerception_s_);
		timeSinceLastPerception_s_ = 0.0f;
	}
	// Do some controller specific updates to the knowledge base.
	updateKnowledgeBase(dt_s);

	if (timeSinceLastDecision_s_ > decisionInterval_s_) {
		// Reset current action state in the blackboard.
		kb.unsetCurrentActions();
		// Update blackboard and make some actions current.
		auto behaviorStatus = behaviorTree_->tick(kb, timeSinceLastDecision_s_);
		if (behaviorStatus == BehaviorStatus::FAILURE) {
			REGEN_WARN("[" << tfIdx_ << "] Behavior tree returned FAILURE " <<
				" linger time: " << kb.lingerTime() <<
				" activity time: " << kb.activityTime());
			//setIdle();
		}
		timeSinceLastDecision_s_ = 0.0f;

		bool hasNavigatingAction = kb.isCurrentAction(ActionType::NAVIGATING);
		bool hasPatrollingAction = kb.isCurrentAction(ActionType::PATROLLING);
		bool hasStrollingAction = kb.isCurrentAction(ActionType::STROLLING);
		bool hasApproachAction = (hasNavigatingAction || hasPatrollingAction || hasStrollingAction);
		if (!hasApproachAction && !isStandingStill()) {
			kb.setCurrentAction(ActionType::WALKING);
		}
	}

	// Update the state of current bone animations based on current actions, intends etc.
	boneController_->updateBoneController(dt_s, knowledgeBase_);
	// Update path etc. for navigation action.
	updateNavigationBehavior();

	// Set flags for current bone animations
	if (boneController_->isCurrentBoneAnimation(MotionType::RUN)) {
		isWalking_ = false;
		isLastAnimationMovement_ = true;
	} else if (boneController_->isCurrentBoneAnimation(MotionType::WALK)) {
		isWalking_ = true;
		isLastAnimationMovement_ = true;
	} else {
		isLastAnimationMovement_ = false;
	}
}

void NonPlayerCharacterController::cpuUpdate(double dt) {
	updateController(dt);
	NavigationController::cpuUpdate(dt);
}
