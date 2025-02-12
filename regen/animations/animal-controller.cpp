#include <random>
#include "animal-controller.h"

using namespace regen;

#define SMOOTH_HEIGHT

AnimalController::AnimalController(
			const ref_ptr<ModelTransformation> &tf,
			const ref_ptr<NodeAnimation> &animation,
			const std::vector<scene::AnimRange> &ranges)
				: AnimationController(tf, animation, ranges),
				  territoryBounds_(Vec2f::zero(), Vec2f::zero()),
				  walkSpeed_(0.05f),
				  runSpeed_(0.1f),
				  laziness_(0.5f),
				  maxHeight_(std::numeric_limits<float>::max()),
				  minHeight_(std::numeric_limits<float>::lowest()),
				  baseOrientation_(M_PI_2),
				  floorHeight_(0.0f),
				  heightMapBounds_(Vec2f::zero(), Vec2f::zero())
{
	for (auto &range: animationRanges_) {
		if (range.name.find("walk") == 0) {
			behaviorRanges_[BEHAVIOR_WALK].push_back(&range);
		}
		else if (range.name.find("run") == 0) {
			behaviorRanges_[BEHAVIOR_RUN].push_back(&range);
		}
		else if (range.name.find("idle") == 0) {
			behaviorRanges_[BEHAVIOR_IDLE].push_back(&range);
		}
		else if (range.name.find("smell") == 0) {
			behaviorRanges_[BEHAVIOR_SMELL].push_back(&range);
		}
		else if (range.name.find("attack") == 0) {
			behaviorRanges_[BEHAVIOR_ATTACK].push_back(&range);
		}
		else if (range.name.find("up") == 0) {
			behaviorRanges_[BEHAVIOR_STAND_UP].push_back(&range);
		}
		else if (range.name.find("sleep") == 0) {
			behaviorRanges_[BEHAVIOR_SLEEP].push_back(&range);
		}
	}
}

void AnimalController::addSpecial(std::string_view special) {
	for (auto &x : animationRanges_) {
		if (x.name == special) {
			//behaviorRanges_[BEHAVIOR_SPECIAL].push_back(&x);
		}
	}
}

void AnimalController::setTerritoryBounds(const Vec2f &center, const Vec2f &size) {
	auto halfSize = size * 0.5f;
	territoryBounds_.min = center - halfSize;
	territoryBounds_.max = center + halfSize;
}

void AnimalController::setHeightMap(
				const ref_ptr<Texture2D> &heightMap,
				const Vec2f &heightMapCenter,
				const Vec2f &heightMapSize,
				float heightMapFactor) {
	heightMap_ = heightMap;
	heightMap_->ensureTextureData();
	heightMapBounds_.min = heightMapCenter - heightMapSize;
	heightMapBounds_.max = heightMapCenter + heightMapSize;
	heightMapFactor_ = heightMapFactor;
}

void AnimalController::activateRandom() {
	auto &ranges = behaviorRanges_[behavior_];
	if (ranges.empty()) { return; }
	// select a random range
	std::vector<const scene::AnimRange*> out;
    std::sample(
        ranges.begin(),
        ranges.end(),
        std::back_inserter(out),
        1,
        std::mt19937{std::random_device{}()}
    );
	auto &range = out[0];
	// set the animation range
	animation_->setAnimationActive(range->channelName, range->range);
	lastRange_ = range;
}

float AnimalController::getHeight(const Vec2f &pos) {
	float height = floorHeight_;
	if (heightMap_.get()) {
		auto mapCenter = (heightMapBounds_.max + heightMapBounds_.min) * 0.5f;
		auto mapSize = heightMapBounds_.max - heightMapBounds_.min;
		// compute UV for height map sampling
		auto uv = pos - mapCenter;
		uv /= mapSize * 0.5f;
		uv += Vec2f(0.5f);
#ifdef SMOOTH_HEIGHT
		auto texelSize = Vec2f(1.0f / heightMap_->width(), 1.0f / heightMap_->height());
		auto regionTS = texelSize*8.0f;
		auto mapValue = heightMap_->sampleAverage(uv, regionTS, heightMap_->textureData(), 1);
#else
		auto mapValue = heightMap_->sampleLinear(uv, heightMap_->textureData(), 1);
#endif
		mapValue *= heightMapFactor_;
		// increase by small bias to avoid intersection with the floor
		mapValue += 0.02f;
		height += mapValue;
	}
	return height;
}

void intersectWithBounds(Vec2f& point, const Vec2f& origin, const Bounds<Vec2f>& territoryBounds) {
        if (point.x < territoryBounds.min.x) {
            float t = (territoryBounds.min.x - origin.x) / (point.x - origin.x);
            point.x = territoryBounds.min.x;
            point.y = origin.y + t * (point.y - origin.y);
        } else if (point.x > territoryBounds.max.x) {
            float t = (territoryBounds.max.x - origin.x) / (point.x - origin.x);
            point.x = territoryBounds.max.x;
            point.y = origin.y + t * (point.y - origin.y);
        }
        if (point.y < territoryBounds.min.y) {
            float t = (territoryBounds.min.y - origin.y) / (point.y - origin.y);
            point.y = territoryBounds.min.y;
            point.x = origin.x + t * (point.x - origin.x);
        } else if (point.y > territoryBounds.max.y) {
            float t = (territoryBounds.max.y - origin.y) / (point.y - origin.y);
            point.y = territoryBounds.max.y;
            point.x = origin.x + t * (point.x - origin.x);
        }
};

void AnimalController::activateMovement() {
	// pick a random point in the territory
	Vec2f target = (territoryBounds_.max - territoryBounds_.min) * Vec2f(
			static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
			static_cast<float>(rand()) / static_cast<float>(RAND_MAX)
	) + territoryBounds_.min;
	// pick a random orientation vector
	auto orientation = Vec3f(
			currentDir_.x,
			static_cast<float>((static_cast<double>(rand()) / RAND_MAX - 0.5)*M_PI),
			currentDir_.z
	);

	bezierPath_.p0 = Vec2f(currentPos_.x, currentPos_.z);
	bezierPath_.p3 = target;
    // compute control points using Euler angles
    float directDistance = (bezierPath_.p0 - bezierPath_.p3).length() * 0.75f;
    // add base orientation
    float angle_rad1 = currentDir_.y + baseOrientation_;
    float angle_rad2 = orientation.y + baseOrientation_;
    bezierPath_.p1 = bezierPath_.p0 + Vec2f(cos(angle_rad1), sin(angle_rad1)) * directDistance;
    bezierPath_.p2 = bezierPath_.p3 + Vec2f(cos(angle_rad2), sin(angle_rad2)) * directDistance;
    // make sure control points are within the map bounds
    intersectWithBounds(bezierPath_.p1, bezierPath_.p0, territoryBounds_);
    intersectWithBounds(bezierPath_.p2, bezierPath_.p3, territoryBounds_);
    float bezierLength = bezierPath_.length1();

	// compute dt based on distance and speed
	float dt = bezierLength / (behavior_==BEHAVIOR_RUN ? runSpeed_ : walkSpeed_);
	// set the target position
	setTarget(
		Vec3f(target.x, getHeight(target), target.y),
		orientation,
		dt);
	activateRandom();
}

void AnimalController::updatePose(const TransformKeyFrame &currentFrame, double t) {
	if (currentFrame.pos.has_value()) {
		//currentPos_ = math::mix(lastFrame_.pos.value(), currentFrame.pos.value(), t);
		auto sample = bezierPath_.sample(t);
		currentPos_.x = sample.x;
		currentPos_.z = sample.y;
		currentPos_.y = getHeight(sample);
	}
	if (currentFrame.rotation.has_value()) {
		auto tangent = bezierPath_.tangent(t);
		tangent.normalize();
		// Convert the tangent vector to Euler angles
        currentDir_.y = atan2(tangent.y, tangent.x) - baseOrientation_;
	}
}

AnimalController::Behavior AnimalController::selectNextBehavior() {
	if (worldTime_) {
		auto &t_ptime = worldTime_->p_time;
		auto t_seconds = t_ptime.time_of_day().total_seconds();
		auto t_hours = t_seconds / 3600;
		if (t_hours < 6 || t_hours > 18) {
			return BEHAVIOR_SLEEP;
		}
	}

	// pick a random behavior
	static const std::vector<Behavior> randomBehaviors = {
		BEHAVIOR_RUN,
		BEHAVIOR_WALK,
		BEHAVIOR_IDLE,
		BEHAVIOR_SPECIAL,
		BEHAVIOR_SMELL,
		BEHAVIOR_ATTACK
	};
	return randomBehaviors[rand() % randomBehaviors.size()];
}

void AnimalController::updateController(double dt) {
	if (it_ == frames_.end()) {
		// currently no movement.
		if (animation_->isNodeAnimationActive()) {
			if (isLastAnimationMovement_) {
				// animation is movement, deactivate
				animation_->stopNodeAnimation();
			}
			return;
		}
	}
	else {
		// movement active
		if (!animation_->isNodeAnimationActive()) {
			// animation not active, activate
			if (lastRange_) {
				animation_->setAnimationActive(lastRange_->channelName, lastRange_->range);
				animation_->startAnimation();
			}
		}
		return;
	}

	auto lastBehavior = behavior_;
	behavior_ = selectNextBehavior();
	if (lastBehavior == BEHAVIOR_SLEEP) {
		if (lastBehavior == behavior_) {
			// remain sleeping
			return;
		}
		else if (behaviorRanges_.count(BEHAVIOR_STAND_UP)>0) {
			// wake up
			behavior_ = BEHAVIOR_STAND_UP;
		}
	}
	switch (behavior_) {
		case BEHAVIOR_RUN:
		case BEHAVIOR_WALK:
			isLastAnimationMovement_ = true;
			activateMovement();
			break;
		default:
			isLastAnimationMovement_ = false;
			activateRandom();
			break;
	}
}
