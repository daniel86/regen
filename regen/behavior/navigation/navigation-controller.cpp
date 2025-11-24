#include <random>
#include "navigation-controller.h"

#define USE_HEIGHT_SMOOTHING
#define USE_DYNAMIC_LOOKAHEAD

using namespace regen;

static void initCollisionFrame_s(void *userData) {
	// set avoidance force to zero
	((NavigationController*)userData)->navCollisionFrameBegin();
}

static void finalizeCollisionFrame_s(void *userData) {
	// finalize avoidance after all perception events have been handled
	((NavigationController*)userData)->navCollisionFrameEnd();
}

static void handlePerception_s(const CollisionEvent &evt, void *userData) {
	// accumulate avoidance forces
	((NavigationController*)userData)->navCollisionFrameAdd(evt);
}

NavigationController::NavigationController(
	const Indexed<ref_ptr<ModelTransformation>> &tfIndexed,
	const ref_ptr<WorldModel> &world)
	: Animation(false, true),
	  CollisionMonitor(handlePerception_s, initCollisionFrame_s, finalizeCollisionFrame_s, this),
	  tf_(tfIndexed.value),
 	  tfIdx_(tfIndexed.index),
	  worldModel_(world) {
	auto currentTransform = tf_->modelMat()->getVertex(tfIdx_);
	setAnimationName(REGEN_STRING("animation-"<<tf_->modelMat()->name()));

	// initialize transform data
	currentPos_ = currentTransform.r.position();
	currentVal_ = currentTransform.r;
	initialScale_ = currentTransform.r.scaling();

	// remove scaling before computing rotation, else we get faulty results
	auto tmp = currentTransform.r;
	tmp.scale(Vec3f(
			1.0f / initialScale_.x,
			1.0f / initialScale_.y,
			1.0f / initialScale_.z));
	// Get Euler angles
	auto angles = tmp.rotation();
	baseRotation_.setEuler(angles.x, angles.y, angles.z);
	currentDir_ = Vec3f::zero();
}

void NavigationController::setHeightMap(const ref_ptr<HeightMap> &heightMap) {
	heightMap_ = heightMap;
	heightMap_->ensureTextureData();
}

float NavigationController::getHeight(const Vec2f &pos) {
	if (heightMap_.get()) {
		return heightMap_->sampleHeight(pos);
	} else {
		return floorHeight_;
	}
}

static inline float wrapPi(float a) {
	// normalize to (-PI, PI]
	while (a <= -M_PI) a += 2.0f * M_PI;
	while (a > M_PI) a -= 2.0f * M_PI;
	return a;
}

void NavigationController::stopNavigation() {
	navModeMask_ = NO_NAVIGATION;
	navGroup_ = {};
	currentPath_.clear();
	currentPathIndex_ = 0;
}

void NavigationController::startFlocking(const ref_ptr<ObjectGroup> &group) {
	navGroup_ = group;
	// set flocking mode flag
	navModeMask_ |= FLOCKING;
}

void NavigationController::stopFlocking() {
	navGroup_ = {};
	// clear flocking mode flag
	navModeMask_ &= ~FLOCKING;
}

void NavigationController::stopApproaching() {
	// clear approaching mode flag
	navModeMask_ &= ~APPROACHING;
}

void NavigationController::startApproaching(const Vec2f &source, const Vec2f &target) {
	// Direction from source to target
	Vec2f dir = target - source;
	float len = dir.length();
	if (len < 1e-6f) {
		dir = Vec2f(cos(currentDir_.x + baseOrientation_), sin(currentDir_.x + baseOrientation_));
	} else {
		dir /= len;
	}

	// Mix with orientation of next waypoint, if any.
	if (currentPathIndex_ + 2 <= currentPath_.size()) {
		Vec2f nextPos = currentPath_[currentPathIndex_ + 1]->position2D();
		Vec2f nextDir = nextPos - target;
		float nextLen = nextDir.length();
		if (nextLen > 1e-6f) {
			nextDir /= nextLen;
			// blend directions
			nextDir = math::lerp(nextDir, nextDir, 0.25f);
			float nextDirLen = nextDir.length();
			if (nextDirLen > 1e-6f) {
				dir = nextDir / nextDirLen;
			}
		}
	}

	float angle = atan2(dir.y, dir.x) - baseOrientation_;
	startApproaching(source, target, Vec3f(angle, 0.0f, 0.0f));
}

void NavigationController::startApproaching(
		const Vec2f &source,
		const Vec2f &target,
		const Vec3f &desiredDir) {
	curvePath_.p0 = source;
	curvePath_.p3 = target;
	// compute control points using Euler angles
	float directDistance = (curvePath_.p0 - curvePath_.p3).length();
	// add base orientation
	float angle_rad1 = currentDir_.x + baseOrientation_;
	float angle_rad2 = desiredDir.x + baseOrientation_;
	float angleDiff = wrapPi(angle_rad2 - angle_rad1);
	// shorter handles for sharper turns
	float handleScale = directDistance * 0.5f * (1.0f - 0.5f * std::abs(angleDiff) / M_PI);
	//float handleScale = directDistance * 0.25f * (1.0f - 0.5f * std::abs(angleDiff) / M_PI);
	// p1: in direction of start orientation
	// - Allow sharper turns by using a shorter handle. e.g. in case NPC makes a 180deg turn.
	curvePath_.p1 = curvePath_.p0 + Vec2f(cos(angle_rad1), sin(angle_rad1)) * handleScale * 0.25f;
	// p2: in direction of target orientation
	const Vec2f approachDir = Vec2f(cos(angle_rad2), sin(angle_rad2));
	curvePath_.p2 = curvePath_.p3 - approachDir * handleScale;
	curveLUT_ = curvePath_.buildArcLengthLUT(numLUTEntries_);
	curveEndDir_.x = approachDir.x;
	curveEndDir_.y = 0.0f;
	curveEndDir_.z = approachDir.y;

	navModeMask_ |= APPROACHING;
	// reset approach time
	approachTime_ = 0.0;
	// compute expected duration based on distance and speed
	approachDuration_ = curveLUT_.totalLength / (isWalking_ ? walkSpeed_ : runSpeed_);

	if (!isRunning()) { startAnimation(); }
}

void NavigationController::navCollisionFrameBegin() {
#ifdef USE_DYNAMIC_LOOKAHEAD
	// Dynamically decrease the lookahead distance when close to the goal.
	// This allows to better approach the goal without being pushed away by
	// things that are located at the goal.
	dynLookAheadDistance_ = std::max(personalSpace_, lookAheadDistance_ *
		std::clamp(distanceToTarget_ / lookAheadThreshold_, 0.0f, 1.0f));
#endif
	navCollision_ = Vec3f::zero();
	navCollisionCount_ = 0;
	hasNewPerception_ = true;
}

void NavigationController::navCollisionFrameEnd() {
	if (navCollisionCount_ > 0) {
		navCollision_ /= static_cast<float>(navCollisionCount_);
		collisionStrength_ = navCollision_.length();
	} else {
		collisionStrength_ = 0.0f;
	}
}

void NavigationController::navCollisionFrameAdd(const CollisionEvent &evt) {
	if (patientShape_.get() == evt.data.other) return; // skip patient
	auto *otherRes = evt.data.other->worldObject();
	bool isStaticObject = true;
	if (otherRes) {
		auto otherWO = static_cast<const WorldObject*>(otherRes);
		isStaticObject = otherWO->isStatic();
	}
	if (isStaticObject) {
		handleStaticCollision(evt.data);
	} else {
		handleCharacterCollision(evt.data);
	}
}

void NavigationController::handleCharacterCollision(const CollisionData &percept) {
	// Note: percept.distance measures center-to-center distance.
	if (percept.distance < personalSpace_ * 3.0f) {
		// The other character is close by, steer away.
		// Use inverse square law for strength falloff.
		float distSqInv = 1.0f / (percept.distance * percept.distance);
		// scale to reasonable values
		distSqInv *= personalSpace_ * characterAvoidance_;
		// Scale by motion activity -> less avoidance when standing still, but at least 10% influence
		// such that small corrections can be done when standing still.
		distSqInv *= std::clamp(currentSpeed_ / walkSpeed_, 0.1f, 1.0f);
		navCollision_.x += percept.dir.x * distSqInv;
		navCollision_.z += percept.dir.z * distSqInv;
		navCollisionCount_++;
	}
}

void NavigationController::handleStaticCollision(const CollisionData &percept) {
#ifdef USE_DYNAMIC_LOOKAHEAD
	float influenceRadius = dynLookAheadDistance_;
#else
	float influenceRadius = lookAheadDistance_;
#endif
	const Vec3f &otherCenter3D = percept.other->tfOrigin();
	const Vec3f &selfCenter3D = percept.self->tfOrigin();

	// Find the closest point on the surface of the other shape.
	// It could be a huge shape, so we can't just use center-to-center direction.
	// Note: closestPointOnSurface() returns a point in world space.
	// Note: If we are very close to the wall, closestDir might be numerically unstable,
	//   so we need to handle this case.
	const Vec3f closest = percept.other->closestPointOnSurface(selfCenter3D);
	Vec3f closestDir = selfCenter3D - closest;
	const float closestDistance = closestDir.length();
	if (closestDistance > influenceRadius) return;
	if (closestDistance < 1e-4f) {
		// We are extremely close to the wall, use center-to-center direction instead.
		closestDir = percept.dir;
	} else {
		closestDir /= closestDistance;
	}

	// Compute direction from the closest point on the surface to the shape center.
	Vec3f innerWallDir = closest - otherCenter3D;
	innerWallDir.y = 0.0f;
	innerWallDir.normalize();
	float innerAngle = closestDir.dot(innerWallDir);
	if (innerAngle < 0.0f) {
		// We are inside the wall, use center direction instead.
		closestDir = percept.dir;
		innerAngle *= -1.0f;
	}

	// Compute tangent blend factor based on angle between delta and innerWallDelta. Here we set
	// tangent max influence if the approach direction coincides with dir between the closest
	// surface point and shape origin.
	const float tanBlend = std::clamp(
		wallTangentWeight_ + 0.5f*innerAngle, 0.0f, 1.0f);
	Vec3f avoidance = Vec3f(-closestDir.z, 0.0f, closestDir.x);
	if (avoidance.dot(currentVel_) < 0.0f) {
		// Pick the tangent handedness that goes in the direction of current velocity
		avoidance = -avoidance;
	}
	avoidance = closestDir * (1.0f - tanBlend) + avoidance * tanBlend;
	avoidance.normalize();
	// Scale avoidance strength based on distance.
	// Here we use a simple linear falloff.
	avoidance *= influenceRadius / (0.1f + closestDistance);
	// Other falloff options:
	//avoidance *= std::clamp(influenceRadius / (0.3f + dist), 0.0f, maxAvoidance_);
	//avoidance *= std::max(0.0f, 1.0f - dist / influenceRadius) * 10.0f;
	//avoidance *= expf(-dist * dist / (2.0f * sigma * sigma));

	// Accumulate avoidance vector.
	navCollision_ += avoidance * wallAvoidance_;
	navCollisionCount_++;
}

void NavigationController::updateNavFlocking() {
	if (!(navModeMask_ & FLOCKING)) { return; }
	const Vec2f currentPos2D(currentPos_.x, currentPos_.z);
	// Desired dir toward group center and away from other group members.
	const float minGroupDistance = personalSpace_ * (0.5f + 0.25f * navGroup_->numMembers());
	Vec2f groupDir = navGroup_->groupCenter2D() - currentPos2D;
	float tmpLen = groupDir.length();
	if (tmpLen > 1e-4f) {
		groupDir /= tmpLen;
		// spring force
		Vec2f cohesion = groupDir * (tmpLen - minGroupDistance);
		// damping: subtract velocity projected onto radial direction
		//const Vec2f vel2D(currentVel_.x, currentVel_.z);
		//cohesion -= groupDir * (vel2D.dot(groupDir) * radialDamping_);
		navFlocking_.x = cohesion.x * cohesionWeight_;
		navFlocking_.y = 0.0f;
		navFlocking_.z = cohesion.y * cohesionWeight_;
	} else {
		navFlocking_ = Vec3f::zero();
	}

	// Separation from other group members
	/**
	Vec2f memberSeparation = Vec2f::zero();
	if (navGroup_->numMembers() > 1) {
		for (uint32_t memberIdx=0; memberIdx < navGroup_->numMembers(); memberIdx++) {
			const WorldObject *member = navGroup_->member(memberIdx);
			//if (member == self) continue;
			Vec2f toMember = currentPos2D - member->position2D();
			float distSq = toMember.lengthSquared();
			// scale to reasonable values
			if (distSq > 1e-4f && distSq < personalSpaceSq_ * 2.0f) {
				distSq *= personalSpace_ * characterAvoidance_;
				memberSeparation += (toMember / distSq) * 0.1f;
			}
		}
		memberSeparation /= static_cast<float>(navGroup_->numMembers());
	}
	navFlocking_.x += memberSeparation.x * memberSeparationWeight_;
	navFlocking_.z += memberSeparation.y * memberSeparationWeight_;
	**/

	flockingStrength_ = navFlocking_.length();
}

void NavigationController::updateNavVelocity(float desiredSpeed, const Vec3f &avoidance, float avoidanceStrength) {
	// Compute desired velocity by blending approach-dir with avoidance.
	if (avoidanceStrength > 1e-4f) {
		// Blend desired with avoidance vector to avoid collisions.
		desiredVel_ = (approachDir_ + avoidance);
		float d = desiredVel_.length();
		if (d < 1e-4f) {
			desiredVel_ = approachDir_;
		} else {
			desiredVel_ /= d;
		}
		// Slow down when avoidance is strong.
		desiredVel_ *= desiredSpeed * std::clamp(1.0f - avoidanceStrength*0.05f, 0.5f, 1.0f);
	} else {
		desiredVel_ = approachDir_ * desiredSpeed;
	}

	// First assume current vel, then blend toward desired vel
	currentVelDir_ = currentVel_;
	currentSpeed_ = currentVelDir_.length();
	if (currentSpeed_ > 1e-4f) currentVelDir_ /= currentSpeed_;

	// Update position by following the desired velocity
	float angleDiff = acos(std::clamp(desiredVel_.dot(currentVelDir_), -1.0f, 1.0f));
	float blendFactor = std::clamp(0.1f + (angleDiff / M_PIf) * 0.4f, 0.1f, 0.5f);
	// Note: avoid rapid changes in velocity by blending with current vel
	currentVel_ = math::lerp(currentVel_, desiredVel_, blendFactor);
}

void NavigationController::updateNavOrientation(const Vec2f &currentPos2D, float desiredSpeed) {
	// Blend velocity direction and path tangent to form a stable target direction.
	// Idea: when moving fast, prefer velocity; when slow, prefer path tangent.
	float vWeight = std::clamp(currentSpeed_ / (desiredSpeed + 1e-6f), 0.0f, 1.0f); // 0..1
	float velBlend = vWeight * velOrientationWeight_; // in [0,1]
	// clamp rotation speed (radians per second)
	float maxDeltaThisFrame = maxTurn_ * lastDT_;

	// Reduce influence of velocity when close to target, and
	// when we are approaching the last waypoint of the current path.
	if (currentPathIndex_ + 1 == currentPath_.size() || currentPath_.empty()) {
		const float distToGoal = (curvePath_.p3 - currentPos2D).lengthSquared();
		const float goalProximity = std::clamp(1.0f - distToGoal / personalSpaceSq_, 0.0f, 1.0f);
		if (goalProximity > 1e-4f) {
			velBlend *= (1.0f - goalProximity);
			// allow faster turning when close to goal
			maxDeltaThisFrame += turnFactorPersonalSpace_ * goalProximity * maxDeltaThisFrame;
			// Final orientation blending toward goal
			desiredDir_ = math::lerp(desiredDir_, curveEndDir_, goalProximity);
			const float tmpLen = desiredDir_.length();
			if (tmpLen > 1e-6f) desiredDir_ /= tmpLen;
		}
	}

	// compute blended direction
	Vec3f blendedDir = desiredDir_;
	if (currentSpeed_ > 1e-4f) {
		blendedDir *= (1.0f - velBlend);
		blendedDir += currentVelDir_ * velBlend;
		// safety: fallback to tangent if blended is too small
		float tmpLen = blendedDir.length();
		if (tmpLen > 1e-4f) blendedDir /= tmpLen;
	}

	// compute target yaw (world yaw)
	float targetYaw = atan2(blendedDir.z, blendedDir.x);
	// retrieve current stored yaw (undo baseOrientation_ offset that is stored in currentDir_.x)
	float prevYaw = currentDir_.x + baseOrientation_;
	// shortest angular difference
	float deltaYaw = wrapPi(targetYaw - prevYaw);
	if (deltaYaw > maxDeltaThisFrame) deltaYaw = maxDeltaThisFrame;
	else if (deltaYaw < -maxDeltaThisFrame) deltaYaw = -maxDeltaThisFrame;
	// apply the clamped rotation
	currentDir_.x = prevYaw + deltaYaw - baseOrientation_;
}

void NavigationController::updateNavController() {
	const Vec2f currentPos2D(currentPos_.x, currentPos_.z);
	float desiredSpeed = (isWalking_ ? walkSpeed_ : runSpeed_);
	float tmpLen;

	if (navModeMask_ & APPROACHING) {
		// Compute next step along the Bezier curve, and the approach direction
		// from current position to that point.
		auto bezierSample = curvePath_.sample(curveTime_);
		approachDir_.x = bezierSample.x - currentPos_.x;
		approachDir_.y = 0.0f;
		approachDir_.z = bezierSample.y - currentPos_.z;
		tmpLen = approachDir_.length();
		if (tmpLen < 1e-4f) { return; }
		approachDir_ /= tmpLen;

		// Compute curve tangent, this is the desired direction along the path.
		const Vec2f bezierTan2 = curvePath_.tangent(curveTime_);
		desiredDir_.x = bezierTan2.x;
		desiredDir_.y = 0.0f;
		desiredDir_.z = bezierTan2.y;
		tmpLen = desiredDir_.length();
		if (tmpLen > 1e-6f) desiredDir_ /= tmpLen;
	} else if (navModeMask_ & FLOCKING) {
		approachDir_ = Vec3f::zero();
		// Orient in direction of group center.
		Vec2f groupDir = navGroup_->groupCenter2D() - currentPos2D;
		desiredDir_.x = groupDir.x;
		desiredDir_.y = 0.0f;
		desiredDir_.z = groupDir.y;
		tmpLen = desiredDir_.length();
		if (tmpLen > 1e-6f) desiredDir_ /= tmpLen;
	} else {
		approachDir_ = Vec3f::zero();
		desiredDir_ = Vec3f::zero();
	}
	currentForce_ = Vec3f::zero();

	// Compute avoidance vector
	Vec3f avoidance = navCollision_ * collisionWeight_;
	float avoidanceStrength = collisionStrength_;
	if (!(navModeMask_ & APPROACHING)) {
		// Reduce avoidance influence when not approaching.
		avoidance *= 0.05f;
		avoidanceStrength *= 0.05f;
	}
	updateNavFlocking();
	avoidance += navFlocking_;
	avoidanceStrength += flockingStrength_;

	// Compute new velocity
	updateNavVelocity(desiredSpeed, avoidance, avoidanceStrength);

	// Advance the position
	currentPos_ += currentVel_ * lastDT_;
	float desiredHeight = getHeight(currentPos2D);
#ifdef USE_HEIGHT_SMOOTHING
	// Smooth height changes over time.
	currentPos_.y = math::lerp(currentPos_.y, desiredHeight,
		std::clamp(lastDT_ * heightSmoothFactor_, 0.0, 1.0));
#else
	currentPos_.y = desiredHeight;
#endif

	// Update orientation
	updateNavOrientation(currentPos2D, desiredSpeed);
}

void NavigationController::cpuUpdate(double dt) {
	if (navModeMask_ == NO_NAVIGATION) {
		currentVel_ = Vec3f::zero();
		currentSpeed_ = 0.0f;
		return;
	}
	lastDT_ = dt / 1000.0;
	approachTime_ += static_cast<float>(lastDT_);
	if (navModeMask_ & APPROACHING) {
		// Approaching target along Bezier curve
		frameTime_ = approachTime_ / approachDuration_;
		curveTime_ = math::Bezier<Vec2f>::lookupParameter(curveLUT_, frameTime_);
	}

	if (hasNewPerception_) {
		hasNewPerception_ = false;
	} else {
		// Blend-out avoidance with velocity over time when no new perception frame arrived.
		// This avoids that avoidance "sticks" too long.
		navCollision_ *= std::max(0.0f, 1.0f - avoidanceDecay_ * static_cast<float>(lastDT_));
	}

#if 0
	const float replanThresholdSq = 100.0f;
	float distanceToBezier = (currentPos2D - bezierSample).lengthSquared();
	if (distanceToBezier > replanThresholdSq) {
		// We are too far away from the bezier curve, so re-plan.
		if (currentPathIndex_ < currentPath_.size()) {
			REGEN_WARN("[" << tfIdx_ << "] Re-planning path, distance to bezier: " << sqrtf(distanceToBezier) << "m");
			Vec2f target = currentPath_.back()->position2D();
			updatePathCurve(currentPos2D, target);
			// and return, we will update velocity next frame
			return;
		}
	}
#endif

	if ((navModeMask_ & APPROACHING) && frameTime_ >= 1.0f) {
		// Approaching duration completed
		navModeMask_ &= ~APPROACHING;
	}

	updateNavController();

	Quaternion q;
	q.setEuler(currentDir_.x, currentDir_.y, currentDir_.z);
	currentVal_ = (baseRotation_ * q).calculateMatrix();
	currentVal_.scale(initialScale_);
	currentVal_.translate(currentPos_);
	tf_->setModelMat(tfIdx_, currentVal_);
}
