#include <regen/utility/logging.h>
#include "bone-tree.h"
#include <ranges>
#include "regen/gl-types/queries/elapsed-time.h"

using namespace regen;

#define INTERPOLATE_QUATERNION_LINEAR
// assume identity as local root node transform.
// not sure if this is safe.
#define LOCAL_ROOT_IS_IDENTITY
//#define BONE_TREE_DEBUG_TIME
//#define BONE_TREE_DEBUG_CULLING
//#define BONE_TREE_DEBUG_TREE_STRUCTURE

static void countNodes(BoneNode *n, uint32_t &count) {
	count++;
	for (auto & it : n->children) {
		countNodes(it.get(), count);
	}
}

#ifdef BONE_TREE_DEBUG_TREE_STRUCTURE
static void printTree(BoneNode *n, int level, std::stringstream &ss) {
	for (int i = 0; i < level; i++) ss << "  ";
	ss << n->name << "\n";
	for (auto & it : n->children) {
		printTree(it.get(), level + 1, ss);
	}
}

static void printTree(BoneNode *n) {
	std::stringstream boneTreeStr;
	printTree(n, 0, boneTreeStr);
	REGEN_INFO("Bone tree:\n" << boneTreeStr.str());
}
#endif

static void loadNodes(BoneNode *n, std::vector<BoneNode*> &vec, uint32_t &count) {
	// Build a map of node names as were loaded by the asset loader.
	// This is usually something like "head", "Joint12", etc.
	vec[count++] = n;
	for (auto & it : n->children) {
		loadNodes(it.get(), vec, count);
	}
}

BoneTree::BoneTree(const ref_ptr<BoneNode> &rootNode, uint32_t numInstances)
		: Animation(false, true),
		  rootNode_(rootNode),
		  numInstances_(numInstances) {
	instanceData_.resize(numInstances_);

	// Fill the node array
	uint32_t numNodes = 0;
	countNodes(rootNode_.get(), numNodes);
	nodes_.resize(numNodes);
	numNodes = 0;
	loadNodes(rootNode_.get(), nodes_, numNodes);
#ifdef BONE_TREE_DEBUG_TREE_STRUCTURE
	printTree(rootNode_.get());
#endif

	// Also load the name-to-node map, and resize the per-instance matrix array.
	for (uint32_t n = 0; n < numNodes; n++) {
		nameToNode_[nodes_[n]->name] = n;
		nodes_[n]->boneTransformationMatrix.resize(numInstances_, Mat4f::identity());
		nodes_[n]->numActive.resize(numInstances_, 0);
		nodes_[n]->nodeIdx = n;
	}

	// Make sure data is allocated.
	blendedTransforms_.resize(numInstances_ * nodes_.size(), Mat4f::identity());
}

int32_t BoneTree::addAnimationTrack(
		const std::string &trackName,
		ref_ptr<StaticAnimationData> &staticData,
		double duration,
		double ticksPerSecond) {
	REGEN_INFO("Loaded animation '" << trackName << "' with "
			<< staticData->channels.size() << " channels, duration " << duration
			<< " ticks, " << ticksPerSecond << " ticks/sec.");
	Track &data = animTracks_.emplace_back();
	data.trackName_ = trackName;
	data.ticksPerSecond_ = ticksPerSecond;
	data.duration_ = duration;
	const auto idx = static_cast<int32_t>(animTracks_.size()) - 1;
	trackNameToIndex_[data.trackName_] = idx;
	// Set node IDs for each channel.
	for (uint32_t a = 0; a < staticData->channels.size(); a++) {
		auto &channel = staticData->channels.data()[a];
		auto it = nameToNode_.find(channel.nodeName_);
		if (it != nameToNode_.end()) {
			channel.nodeIndex_ = it->second;
		} else {
			REGEN_WARN("Unable to find node '" << channel.nodeName_
					<< "' for animation track '" << trackName << "'.");
		}
	}
	// sort channels by node index for better cache coherence
	std::ranges::sort(staticData->channels,
		[](const AnimationChannel &a, const AnimationChannel &b) {
			// sort by node index: low to high
			return a.nodeIndex_ < b.nodeIndex_;
		});
	// fill up with empty channels for nodes that are not animated
	if (staticData->channels.size() != nodes_.size()) {
		int32_t nextChannelIdx = staticData->channels.size() - 1;
		staticData->channels.resize(nodes_.size());
		for (int32_t nodeIdx = nodes_.size() - 1; nodeIdx != -1; nodeIdx--) {
			if (nextChannelIdx >= 0) {
				auto &nextChannel = staticData->channels[nextChannelIdx];
				if (nextChannel.nodeIndex_ == static_cast<uint32_t>(nodeIdx)) {
					// this node has an animation channel
					nextChannelIdx--;
					staticData->channels[nodeIdx] = nextChannel;
					continue;
				}
			}
			// insert empty channel for this node
			auto &emptyChannel = staticData->channels[nodeIdx];
			emptyChannel.nodeIndex_ = nodeIdx;
			emptyChannel.nodeName_ = nodes_[nodeIdx]->name;
			emptyChannel.isAnimated = false;
		}
	}
	data.staticData_ = staticData;
	return idx;
}

int32_t BoneTree::getTrackIndex(const std::string &trackName) const {
	auto it = trackNameToIndex_.find(trackName);
	if (it == trackNameToIndex_.end()) {
		return -1;
	}
	return it->second;
}

BoneTree::AnimationHandle BoneTree::startBoneAnimation(
			uint32_t instanceIdx,
			int32_t trackIdx,
			const Vec2d &forcedTickRange,
			float *nodeWeights) {
	auto &instance = instanceData_[instanceIdx];
	AnimationHandle animHandle = -1;
	for (uint32_t idx = 0; idx < instance.ranges_.size(); idx++) {
		auto &n = instance.ranges_[idx];
		// note: It is fine if multiple ranges on the same track are active,
		//       and also multiple ranges on different tracks.
		//       We just look for an inactive range slot here.
		if (animHandle == -1 && n.trackIdx_ == -1) {
			animHandle = static_cast<int32_t>(idx);
		}
	}
	// resize vectors if needed
	if (animHandle==-1) {
		instance.ranges_.resize(instance.numActiveRanges_ + 1);
		animHandle = static_cast<int32_t>(instance.numActiveRanges_);
	}
	instance.numActiveRanges_++;

	// Initialize the new range.
	auto &range = instance.ranges_[animHandle];
	range.tickRange_.x = -1.0;
	range.tickRange_.y = -1.0;
	range.weight_ = 1.0;
	range.trackIdx_ = trackIdx;
	range.nodeWeights_ = nodeWeights;
	setTickRange(range, forcedTickRange);

	// Count number of animations affecting a node.
	Track &anim = animTracks_[range.trackIdx_];
	for (uint32_t a = 0; a < anim.staticData_->channels.size(); a++) {
		auto &channel = anim.staticData_->channels.data()[a];
		if (channel.isAnimated) {
			nodes_[channel.nodeIndex_]->numActive[instanceIdx]++;
		}
	}

	return animHandle;
}

void BoneTree::stopBoneAnimation(uint32_t instanceIdx, AnimationHandle animHandle) {
	auto &instance = instanceData_[instanceIdx];
	auto &ar = instance.ranges_[animHandle];
	if (ar.trackIdx_ == -1) return;

	// Count number of animations affecting a node.
	Track &anim = animTracks_[ar.trackIdx_];
	for (uint32_t a = 0; a < anim.staticData_->channels.size(); a++) {
		auto &channel = anim.staticData_->channels.data()[a];
		if (channel.isAnimated) {
			nodes_[channel.nodeIndex_]->numActive[instanceIdx]--;
		}
	}

	auto currTrack = ar.trackIdx_;
	ar.trackIdx_ = -1;
	eventData_->instanceIdx = instanceIdx;
	eventData_->rangeIdx = animHandle;
	emitEvent(ANIMATION_STOPPED, eventData_);
	ar.elapsedTime_ = 0.0;
	ar.lastTime_ = 0;

	if (ar.trackIdx_ == currTrack) {
		// repeat, signal handler set animationIndex_=currIndex again
	} else {
		instanceData_[instanceIdx].numActiveRanges_--;
	}
}

bool BoneTree::isBoneAnimationActive(uint32_t instanceIdx, AnimationHandle animHandle) const {
	auto &instance = instanceData_[instanceIdx];
	auto &ar = instance.ranges_[animHandle];
	return ar.trackIdx_ != -1;
}

float BoneTree::animationWeight(uint32_t instanceIdx, AnimationHandle animHandle) const {
	auto &instance = instanceData_[instanceIdx];
	return instance.ranges_[animHandle].weight_;
}

void BoneTree::setAnimationWeight(uint32_t instanceIdx, AnimationHandle animHandle, float weight) {
	auto &instance = instanceData_[instanceIdx];
	instance.ranges_[animHandle].weight_ = weight;
}

void BoneTree::setTickRange(uint32_t instanceIdx, AnimationHandle animHandle, const Vec2d &tickRange) {
	auto &instance = instanceData_[instanceIdx];
	setTickRange(instance.ranges_[animHandle], tickRange);
}

// Look for present frame number.
// Search from last position if time is after the last time, else from beginning

template<class T>
static void findFrameAfterTick(double tick, uint32_t &frame, const std::vector<T> &keys) {
	double dt;
	while (frame < keys.size() - 1) {
		dt = tick - keys[++frame].time;
		if (dt < 0.00001) {
			--frame;
			return;
		}
	}
}

template<class T>
static void findFrameBeforeTick(double &tick, uint32_t &frame, const std::vector<T> &keys) {
	if (keys.empty()) return;
	double dt;
	for (frame = keys.size() - 1; frame > 0;) {
		dt = tick - keys[--frame].time;
		if (dt > 0.000001) return;
	}
}

void BoneTree::setTickRange(ActiveRange &ar, const Vec2d &forcedTickRange) {
	if (ar.trackIdx_ == -1) {
		REGEN_WARN("can not set tick range without animation track set.");
		return;
	}
	Track &anim = animTracks_[ar.trackIdx_];
	const uint32_t numChannels = anim.staticData_->channels.size();

	// Ensure that startFramePosition and lastFramePosition are allocated.
	if (ar.startFramePosition_.size() != numChannels) {
		ar.startFramePosition_.resize(numChannels, Vec3ui::zero());
		ar.lastFramePosition_.resize(numChannels, Vec3ui::zero());
		ar.lastInterpolation_.resize(numChannels, Vec3f::zero());
	}

	// get first and last tick of animation
	Vec2d tickRange;
	if (forcedTickRange.x < 0.0f || forcedTickRange.y < 0.0f) {
		tickRange.x = 0.0;
		tickRange.y = anim.duration_;
	} else {
		tickRange = forcedTickRange;
	}
	if (tickRange.x == ar.tickRange_.x && tickRange.y == ar.tickRange_.y) {
		// nothing changed
		return;
	}
	ar.tickRange_ = tickRange;

	// set start frames
	if (ar.tickRange_.x < 0.00001) {
		Vec3ui *startFrameData = ar.startFramePosition_.data();
		std::memset((byte*)startFrameData, 0, sizeof(Vec3ui) * numChannels);
	} else {
		for (uint32_t a = 0; a < numChannels; a++) {
			AnimationChannel &channel = anim.staticData_->channels[a];
			Vec3ui framePos = Vec3ui::zero();
			if (channel.isAnimated) {
				findFrameBeforeTick(ar.tickRange_.x, framePos.x, channel.positionKeys_);
				findFrameBeforeTick(ar.tickRange_.x, framePos.y, channel.rotationKeys_);
				findFrameBeforeTick(ar.tickRange_.x, framePos.z, channel.scalingKeys_);
				ar.startFramePosition_[a] = framePos;
			} else {
				ar.startFramePosition_[a] = framePos;
			}
		}
	}

	// initial last frame to start frame
	std::memcpy(
		(byte*)ar.lastFramePosition_.data(),
		(byte*)ar.startFramePosition_.data(),
		sizeof(Vec3ui) * numChannels);

	// set to start pos of the new tick range
	ar.lastTime_ = 0.0;
	ar.elapsedTime_ = 0.0;

	// set start tick and duration in ticks
	ar.startTick_ = ar.tickRange_.x;
	ar.duration_ = abs(ar.tickRange_.y - ar.tickRange_.x);
}

double BoneTree::elapsedTime(uint32_t instanceIdx, AnimationHandle animHandle) const {
	auto &instance = instanceData_[instanceIdx];
	auto &ar = instance.ranges_[animHandle];
	if (ar.trackIdx_ == -1) {
		return 0.0;
	} else {
		return ar.elapsedTime_;
	}
}

double BoneTree::ticksPerSecond(uint32_t trackIdx) const {
	if (trackIdx >= animTracks_.size()) {
		return 0;
	}
	return animTracks_[trackIdx].ticksPerSecond_;
}

void BoneTree::animate(double dt_ms) {
#ifdef BONE_TREE_DEBUG_TIME
	static ElapsedTimeDebugger elapsedTime("NodeAnimation Update", 300);
	elapsedTime.beginFrame();
#endif
	const auto numNodes = static_cast<uint32_t>(nodes_.size());

	for (uint32_t instanceIdx = 0; instanceIdx < numInstances_; instanceIdx++) {
		auto &instance = instanceData_[instanceIdx];
		if (instance.numActiveRanges_ == 0) continue;

		// Advance time in active ranges
		for (AnimationHandle rangeIdx = 0;
				rangeIdx < static_cast<AnimationHandle>(instance.ranges_.size());
				rangeIdx++) {
			ActiveRange &range = instance.ranges_[rangeIdx];
			if (range.trackIdx_ == -1) continue;
			Track &track = animTracks_[range.trackIdx_];
			range.elapsedTime_ += dt_ms;
			range.lastTime_ = range.timeInTicks_;
			// map into anim's duration
			range.timeInTicks_ = std::min(range.duration_,
				range.elapsedTime_ * timeFactor_ * track.ticksPerSecond_);
			// Time runs backwards when start tick is higher then stop tick
			if (range.tickRange_.x > range.tickRange_.y) {
				range.timeInTicks_ = -range.timeInTicks_;
			}
			range.timeInTicks_ += range.startTick_;
		}

		// Update transformation for each node
		for (uint32_t nodeIdx = 0; nodeIdx < nodes_.size(); nodeIdx++) {
			// Compute per-bone effective weights
			float weightSum = 0.0f;
			for (auto &range : instance.ranges_) {
				if (range.trackIdx_ == -1) continue;
				auto &track = animTracks_[range.trackIdx_];
				if (track.staticData_->channels[nodeIdx].isAnimated == false) continue;
				if (range.nodeWeights_) {
					weightSum += range.weight_ * range.nodeWeights_[nodeIdx];
				} else {
					weightSum += range.weight_;
				}
			}
			if (weightSum < 1e-5f) {
				// fallback to local transform for this node (avoid stale data)
				Mat4f &m = blendedTransforms_[instanceIdx * numNodes + nodeIdx];
				m = nodes_[nodeIdx]->localTransform;
				continue;
			}

			accRot_ = Quaternion(0, 0, 0, 0);
			accPos_ = Vec3f::zero();
			accScale_ = Vec3f::zero();

			auto &nodeData = nodes_[nodeIdx];
			bool hasWeightChanged = (abs(nodeData->lastWeightSum - weightSum) > 1e-6f);
			nodeData->lastWeightSum = weightSum;
			bool isDirty = (hasWeightChanged);

			for (auto &range : instance.ranges_) {
				if (range.trackIdx_ == -1) continue;
				auto &track = animTracks_[range.trackIdx_];
				auto &channel = track.staticData_->channels[nodeIdx];
				// Compute normalized weight for this bone.
				float weight = range.weight_;
				if (range.nodeWeights_) {
					weight *= range.nodeWeights_[nodeIdx];
				}
				weight /= weightSum;

				if (channel.rotationKeys_.empty()) {
					accRot_ += (Quaternion(1, 0, 0, 0) * weight);
				} else if (channel.rotationKeys_.size() == 1) {
					tmpRot_.value = (channel.rotationKeys_.data()[0].value) * weight;
					if (accRot_.dot(tmpRot_.value) < 0.0f) {
						tmpRot_.value = -tmpRot_.value;
					}
					accRot_ += tmpRot_.value;
				} else {
					tmpRot_.value = nodeRotation(range, channel, isDirty, nodeIdx) * weight;
					if (accRot_.dot(tmpRot_.value) < 0.0f) {
						tmpRot_.value = -tmpRot_.value;
					}
					accRot_ += tmpRot_.value;
				}
				if (channel.scalingKeys_.empty()) {
					accScale_ += (Vec3f::one() * weight);
				} else if (channel.scalingKeys_.size() == 1) {
					accScale_ += (channel.scalingKeys_.data()[0].value * weight);
				} else {
					accScale_ += (nodeScaling(range, channel, isDirty, nodeIdx) * weight);
				}
				if (channel.positionKeys_.empty()) {
				} else if (channel.positionKeys_.size() == 1) {
					accPos_ += (channel.positionKeys_.data()[0].value * weight);
				} else {
					accPos_ += (nodePosition(range, channel, isDirty, nodeIdx) * weight);
				}
			}
			if (isDirty) {
				nodes_[nodeIdx]->localDirty = true;
				if (instance.numActiveRanges_ > 1) {
					accRot_.normalize();
				}
				Mat4f &m = blendedTransforms_[instanceIdx * numNodes + nodeIdx];
				m = accRot_.calculateMatrix();
				m.scale(accScale_);
				m.x[3] = accPos_.x;
				m.x[7] = accPos_.y;
				m.x[11] = accPos_.z;
			}
		}

		rootNode_->updateTransforms(instanceIdx,
			blendedTransforms_.data() + instanceIdx * numNodes);

		for (AnimationHandle rangeIdx = 0;
				rangeIdx < static_cast<AnimationHandle>(instance.ranges_.size()); rangeIdx++) {
			ActiveRange &range = instance.ranges_[rangeIdx];
			if (range.trackIdx_ == -1) continue;
			if ((range.timeInTicks_ - range.startTick_) >= range.duration_ - 1e-5) {
				stopBoneAnimation(instanceIdx, rangeIdx);
			}
		}
	}

#ifdef BONE_TREE_DEBUG_TIME
	elapsedTime.push("animate");
	elapsedTime.endFrame();
#endif
}

template<class T>
static inline bool handleFrameLoop(T &dst,
		const uint32_t &frame, const uint32_t &lastFrame,
		const AnimationChannel &channel,
		const T &key, const T &first) {
	if (frame < lastFrame) {
		if (channel.postState == AnimationChannelBehavior::DEFAULT) {
			dst.value = first.value;
			return true;
		}
		if (channel.postState == AnimationChannelBehavior::CONSTANT) {
			dst.value = key.value;
			return true;
		}
	}
	return false;
}

template<typename T,int I>
static inline bool computeInterpolation(
		BoneTree::ActiveRange &ar, uint32_t i,
		uint32_t frame, uint32_t lastFrame,
		const Stamped<T> &key, const Stamped<T> &nextKey,
		bool &isDirty,
		double &facOut) {
	double timeDifference = nextKey.time - key.time;
	if (timeDifference < 0.0) timeDifference += ar.duration_;
	facOut = ((ar.timeInTicks_ - key.time) / timeDifference);
	// TODO: I am not sure if it is worth doing the dirty flagging here.
	//    Maybe it is better to ust mark all channels of an active animation track as dirty.
	if (facOut <= 0) {
		if (ar.lastInterpolation_[i][I] - 1e-6f > 0.0f) {
			ar.lastInterpolation_[i][I] = 0.0f;
			isDirty = true;
		}
		return false;
	}
	// Only set dirty flag if interpolation factor or frame changed.
	if (lastFrame != frame || std::abs(ar.lastInterpolation_[i][I] - facOut) > 1e-2f) {
		isDirty = true;
		ar.lastInterpolation_[i][I] = static_cast<float>(facOut);
	}
	return true;
}

template<typename T,int I>
static inline void findFrame(BoneTree::ActiveRange &ar,
		uint32_t i,
		uint32_t &frame,
		uint32_t &lastFrame,
		const std::vector<Stamped<T>> &keys) {
	lastFrame = ar.lastFramePosition_[i][I];
	// Search from last position if time is after the last time, else from beginning
	// This is a bit tricky if the animation runs backwards.
	if (ar.tickRange_.x > ar.tickRange_.y) {
		frame = (ar.timeInTicks_ <= ar.lastTime_ ? lastFrame : ar.startFramePosition_[i][I]);
		findFrameBeforeTick(ar.timeInTicks_, frame, keys);
	} else {
		frame = (ar.timeInTicks_ >= ar.lastTime_ ? lastFrame : ar.startFramePosition_[i][I]);
		findFrameAfterTick(ar.timeInTicks_, frame, keys);
	}
	ar.lastFramePosition_[i][I] = frame;
}

Vec3f BoneTree::nodePosition(ActiveRange &ar, const AnimationChannel &channel, bool &isDirty, uint32_t i) {
	const uint32_t keyCount = channel.positionKeys_.size();
	auto &keys = channel.positionKeys_;
	// Find present frame number.
	uint32_t frame, lastFrame;
	findFrame<Vec3f,0>(ar, i, frame, lastFrame, keys);

	// lookup nearest two keys
	auto &key = keys[frame];
	tmpPos_.value = Vec3f::zero();
	// interpolate between this frame's value and next frame's value
	if (!handleFrameLoop(tmpPos_, frame, lastFrame, channel, key, keys[0])) {
		auto &nextKey = keys[(frame + 1) % keyCount];
		double fac = 0.0;
		if (computeInterpolation<Vec3f,0>(ar, i, frame, lastFrame, key, nextKey, isDirty, fac)) {
			tmpPos_.value = key.value + (nextKey.value - key.value) * static_cast<float>(fac);
		} else {
			return key.value;
		}
	} else if (lastFrame != frame || (1.0f - ar.lastInterpolation_[i].x) > 1e-6f) {
		ar.lastInterpolation_[i].x = 1.0f;
		isDirty = true;
	}

	return tmpPos_.value;
}

Quaternion BoneTree::nodeRotation(ActiveRange &ar, const AnimationChannel &channel, bool &isDirty, uint32_t i) {
	const uint32_t keyCount = channel.rotationKeys_.size();
	auto &keys = channel.rotationKeys_;
	// Find present frame number.
	uint32_t frame, lastFrame;
	findFrame<Quaternion,1>(ar, i, frame, lastFrame, keys);

	// lookup nearest two keys
	const Stamped<Quaternion> &key = keys[frame];
	tmpRot_.value = Quaternion(1, 0, 0, 0);
	// interpolate between this frame's value and next frame's value
	if (!handleFrameLoop(tmpRot_, frame, lastFrame, channel, key, keys[0])) {
		const Stamped<Quaternion> &nextKey = keys[(frame + 1) % keyCount];
		double fac = 0.0;
		if (computeInterpolation<Quaternion,1>(ar, i, frame, lastFrame, key, nextKey, isDirty, fac)) {
#ifdef INTERPOLATE_QUATERNION_LINEAR
			tmpRot_.value.interpolateLinear(key.value, nextKey.value, static_cast<float>(fac));
#else
			tmpRot_.value.interpolate(key.value, nextKey.value, fac);
#endif
		} else {
			return key.value;
		}
	} else if (lastFrame != frame || (1.0f - ar.lastInterpolation_[i].y) > 1e-6f) {
		ar.lastInterpolation_[i].y = 1.0f;
		isDirty = true;
	}

	return tmpRot_.value;
}

Vec3f BoneTree::nodeScaling(ActiveRange &ar, const AnimationChannel &channel, bool &isDirty, uint32_t i) {
	auto &keys = channel.scalingKeys_;
	// Find present frame number.
	uint32_t frame, lastFrame;
	findFrame<Vec3f,2>(ar, i, frame, lastFrame, keys);

	// lookup nearest key
	auto &key = keys[frame];
	// set current value
	if (!handleFrameLoop(tmpScale_, frame, lastFrame, channel, key, keys[0])) {
		const Stamped<Vec3f> &nextKey = keys[(frame + 1) % keys.size()];
		double fac = 0.0;
		if (computeInterpolation<Vec3f,2>(ar, i, frame, lastFrame, key, nextKey, isDirty, fac)) {
			tmpScale_.value = key.value + (nextKey.value - key.value) * static_cast<float>(fac);
		} else {
			return key.value;
		}
	} else if (lastFrame != frame || (1.0f - ar.lastInterpolation_[i].z) > 1e-6f) {
		ar.lastInterpolation_[i].z = 1.0f;
		isDirty = true;
	}

	return tmpScale_.value;
}

ref_ptr<BoneNode> BoneTree::findNode(const std::string &name) {
	return findNode(rootNode_, name);
}

ref_ptr<BoneNode> BoneTree::findNode(ref_ptr<BoneNode> &n, const std::string &name) {
	if (n->name == name) { return n; }
	for (auto & it : n->children) {
		ref_ptr<BoneNode> n_ = findNode(it, name);
		if (n_.get()) { return n_; }
	}
	return {};
}

BoneNode::BoneNode(const std::string &name, const ref_ptr<BoneNode> &parent)
		: name(name),
		  parent(parent),
		  localTransform(Mat4f::identity()),
		  globalTransform(Mat4f::identity()),
		  offsetMatrix(Mat4f::identity()),
		  isBoneNode(false) {
	boneTransformationMatrix.resize(1, Mat4f::identity());
	numActive.resize(1, 0);
}

void BoneNode::addChild(const ref_ptr<BoneNode> &child) {
	children.push_back(child);
}

void BoneNode::calculateGlobalTransform() {
	// concatenate all parent transforms to get the global transform for this node
	globalTransform = localTransform;
	for (BoneNode *p = parent.get(); p != nullptr; p = p->parent.get()) {
		globalTransform.multiplyr(p->localTransform);
	}
}

void BoneNode::updateTransforms(uint32_t instanceIdx, Mat4f *transforms) {
#ifndef LOCAL_ROOT_IS_IDENTITY
	Mat4f rootInverse = localTransform.inverse();
#endif
	if (isBoneNode) {
		boneTransformationMatrix[instanceIdx] = offsetMatrix;
	}
#ifdef BONE_TREE_DEBUG_CULLING
	uint32_t numTraversed = 0;
	uint32_t numNodes = 0;
#endif

	for (auto &child : children) {
		traversalStack.push(child.get());
	}
	while (!traversalStack.isEmpty()) {
		BoneNode *n = traversalStack.top();
		traversalStack.pop();

#ifdef BONE_TREE_DEBUG_CULLING
		numNodes++;
#endif
		// skip if both local and parent global unchanged
		if (!n->localDirty && !n->parent->globalDirty) {
			continue;
		}
#ifdef BONE_TREE_DEBUG_CULLING
		numTraversed++;
#endif

		// update node global transform
		// note: An alternative to this conditional could be to store base transform into transforms
		// array for nodes that have no active animation, but that would be more IO so might
		// not be worth it.
		n->globalTransform = n->numActive[instanceIdx] == 0 ?
			n->localTransform : transforms[n->nodeIdx];
		n->globalTransform.multiplyr(n->parent->globalTransform);
		n->globalDirty = true;
		n->localDirty = false;

		if (n->isBoneNode) {
			// Bone matrices transform from mesh coordinates in bind pose
			// to mesh coordinates in skinned pose
#ifdef LOCAL_ROOT_IS_IDENTITY
			n->boneTransformationMatrix[instanceIdx] = (n->globalTransform * n->offsetMatrix).transpose();
#else
			n->boneTransformationMatrix[instanceIdx] = (rootInverse * n->globalTransform * n->offsetMatrix).transpose();
#endif
		}

		// continue for all children
		for (auto &child : n->children) {
			traversalStack.push(child.get());
		}
		// after traversal, clear dirty flag
		n->globalDirty = false;
	}
	// Also clear root dirty flag
	globalDirty = false;
#ifdef BONE_TREE_DEBUG_CULLING
	if (numNodes > 0) {
		REGEN_INFO("Traversal rate: " <<
			(float)numTraversed / (float)numNodes * 100.0f << "%" <<
			" (" << numTraversed << " / " << numNodes << ")");
	}
#endif
}
