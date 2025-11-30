#include <regen/utility/logging.h>
#include "bone-tree.h"
#include <ranges>
#include "regen/gl-types/queries/elapsed-time.h"
#include "regen/scene/scene.h"

using namespace regen;

//#define BONE_TREE_DEBUG_TIME
#define REGEN_FORCE_INLINE __attribute__((always_inline)) inline

namespace regen {
	static constexpr uint32_t BONE_ROOT_NODE_IDX = 0u;
	static constexpr bool BONE_LOCAL_ROOT_IS_IDENTITY = true;
	static constexpr bool BONE_INTERPOLATE_QUATERNION_LINEAR = true;
	static constexpr bool BONE_USE_MULTITHREADING = true;
	static constexpr bool BONE_DEBUG_TREE_STRUCTURE = false;
	static constexpr bool BONE_DEBUG_CULLING = false;

	/**
	 * Data structure for bone tree traversal.
	 */
	struct BoneTreeTraversal {
		// per-node traversal data
		std::vector<int8_t> localDirty;
		std::vector<int8_t> globalDirty;

		Quaternion accRot;
		Vec3f accPos;
		Vec3f accScale;

		Mat4f rootInverse = Mat4f::identity();
		// Debugging data
		uint32_t numTraversed = 0;
		uint32_t numNodes = 0;
	};
} // namespace regen

BoneTree::BoneTree(const std::vector<ref_ptr<BoneNode>> &nodes, uint32_t numInstances)
		: Animation(false, true),
		  nodes_(nodes),
		  numInstances_(numInstances) {
	const auto numNodes = static_cast<uint32_t>(nodes.size());
	if constexpr (BONE_DEBUG_TREE_STRUCTURE) {
		for (uint32_t n = 0; n < numNodes; n++) {
			REGEN_INFO("Bone node [" << n << "] name='" << nodes_[n]->name << "'");
		}
	}
	if (numNodes == 0) {
		REGEN_WARN("Created BoneTree with zero nodes.");
		return;
	}
	if constexpr (BONE_USE_MULTITHREADING) {
		JobPool& pool = threading::getJobPool();
		const uint32_t numWorkerThreads = pool.numThreads();
		// Compute the number of instances in each slice.
		const uint32_t numSlices = std::min(numInstances_, numWorkerThreads + 1);
		instanceSlices_.resize(numSlices);
		sliceSize_ = (numInstances_ + numSlices - 1) / numSlices;
	}
	rootNode_ = nodes_[BONE_ROOT_NODE_IDX];

	// per-node data vectors
	localTransform_.resize(numNodes, Mat4f::identity());
	offsetMatrix_.resize(numNodes, Mat4f::identity());
	nodeParent_.resize(numNodes, -1);
	isBoneNode_.resize(numNodes, 0);

	// per-instance data vectors
	instanceData_.resize(numInstances_);

	// per-node+instance data vectors
	numActive_.resize(numNodes * numInstances_, 0);
	boneMatrix_.resize(numNodes * numInstances_, Mat4f::identity());
	blendedTransforms_.resize(numNodes * numInstances_, Mat4f::identity());

	// Also load the name-to-node map, and resize the per-instance matrix array.
	for (uint32_t n = 0; n < numNodes; n++) {
		auto &node = nodes_[n];
		nameToNode_[node->name] = n;
		node->nodeIdx = n;
	}
	// set parent indices
	for (uint32_t n = 1; n < numNodes; n++) {
		auto &node = nodes_[n];
		nodeParent_[n] = static_cast<int32_t>(node->parent->nodeIdx);
	}
}

ref_ptr<BoneNode> BoneTree::findNode(const std::string &name) {
	auto it = nameToNode_.find(name);
	if (it != nameToNode_.end()) {
		return nodes_[it->second];
	} else {
		return {};
	}
}

int32_t BoneTree::addAnimationTrack(
		std::string_view trackName,
		ref_ptr<StaticAnimationData> &staticData,
		double duration,
		double ticksPerSecond) {
	const auto numNodes = static_cast<uint32_t>(nodes_.size());
	REGEN_INFO("Loaded animation '" << trackName << "' with "
			<< staticData->channels.size() << " channels, duration " << duration
			<< " ticks, " << ticksPerSecond << " ticks/sec.");

	Track &data = animTracks_.emplace_back();
	data.trackName_ = trackName;
	data.ticksPerSecond_ = ticksPerSecond;
	data.duration_ = duration;

	const auto idx = static_cast<int32_t>(animTracks_.size()) - 1;
	trackNameToIndex_[data.trackName_] = idx;

	bool isMissingScaleKey = false;
	// Set node IDs for each channel.
	for (auto & channel : staticData->channels) {
		auto it = nameToNode_.find(channel.nodeName_);
		if (it != nameToNode_.end()) {
			channel.nodeIndex_ = it->second;
		} else {
			REGEN_WARN("Unable to find node '" << channel.nodeName_
					<< "' for animation track '" << trackName << "'.");
		}
		if (channel.scalingKeys_.empty()) {
			isMissingScaleKey = true;
		} else {
			hasScalingKeys_ = true;
		}
	}
	if (isMissingScaleKey && hasScalingKeys_) {
		REGEN_WARN("Animation track '" << trackName
				<< "' is missing scaling keys for some nodes.");
		hasScalingKeys_ = false;
	}

	// sort channels by node index for better cache coherence
	std::ranges::sort(staticData->channels,
		[](const AnimationChannel &a, const AnimationChannel &b) {
			// sort by node index: low to high
			return a.nodeIndex_ < b.nodeIndex_;
		});

	// fill up with empty channels for nodes that are not animated
	if (staticData->channels.size() != numNodes) {
		int32_t nextChannelIdx = static_cast<int>(staticData->channels.size()) - 1;
		staticData->channels.resize(numNodes);
		for (int32_t nodeIdx = static_cast<int>(numNodes) - 1; nodeIdx != -1; nodeIdx--) {
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
	const auto numNodes = static_cast<uint32_t>(nodes_.size());
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
		instance.handleToActiveRange_.resize(instance.numActiveRanges_ + 1);
		animHandle = static_cast<int32_t>(instance.numActiveRanges_);
	}
	instance.numActiveRanges_++;
	// also push range index when enabling:
	instance.handleToActiveRange_[animHandle] = instance.activeRanges_.size();
	instance.activeRanges_.push_back(animHandle);

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
	for (auto & channel : anim.staticData_->channels) {
		if (channel.isAnimated) {
			numActive_[instanceIdx * numNodes + channel.nodeIndex_]++;
		}
	}

	return animHandle;
}

void BoneTree::stopBoneAnimation(uint32_t instanceIdx, AnimationHandle animHandle) {
	const auto numNodes = static_cast<uint32_t>(nodes_.size());
	auto &instance = instanceData_[instanceIdx];
	auto &ar = instance.ranges_[animHandle];
	if (ar.trackIdx_ == -1) return;

	// Count number of animations affecting a node.
	Track &anim = animTracks_[ar.trackIdx_];
	for (auto & channel : anim.staticData_->channels) {
		if (channel.isAnimated) {
			numActive_[instanceIdx * numNodes + channel.nodeIndex_]--;
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
		instance.numActiveRanges_--;
		// also remove from active ranges, do this by swapping with last
		uint32_t activeRangePos = instance.handleToActiveRange_[animHandle];
		if (activeRangePos == instance.numActiveRanges_) {
			// last element, just pop
			instance.activeRanges_.pop_back();
		} else {
			// swap with last
			int32_t lastHandle = instance.activeRanges_.back();
			instance.activeRanges_[activeRangePos] = lastHandle;
			instance.handleToActiveRange_[lastHandle] = activeRangePos;
			instance.activeRanges_.pop_back();
		}
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
template<class T>
static void findFrameBeforeTick(double &tick, uint32_t &frame, const std::vector<T> &keys) {
	const uint32_t numKeys = keys.size();
	if (numKeys == 0) return;
	double dt;
	for (frame = numKeys - 1; frame > 0;) {
		dt = tick - keys[--frame].time;
		if (dt > 0.000001) return;
	}
}

void BoneTree::setTickRange(ActiveBoneRange &ar, const Vec2d &forcedTickRange) {
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
	if (tickRange.x > tickRange.y) {
		REGEN_WARN("Reverse playback not yet implemented for bone animations.");
		tickRange.y = tickRange.x;
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

namespace regen {
	struct BoneSliceJob {
		static void run(void *arg) {
			BoneTreeSlice &slice = *static_cast<BoneTreeSlice *>(arg);
			BoneTree* boneTree = slice.boneTree;
			const uint32_t numSliceInstances = slice.endInstance - slice.startInstance;

			if (boneTree->hasScalingKeys_) {
				boneTree->updateBoneTree<true>(slice.startInstance, numSliceInstances, slice.dt_ms);
			} else {
				boneTree->updateBoneTree<false>(slice.startInstance, numSliceInstances, slice.dt_ms);
			}
		}
	};
}

static void boneSliceJob(void *arg) {
	BoneSliceJob::run(arg);
}

void BoneTree::cpuUpdate(double dt_ms) {
#ifdef BONE_TREE_DEBUG_TIME
	static ElapsedTimeDebugger elapsedTime("NodeAnimation Update", 300);
	elapsedTime.beginFrame();
#endif

	jobFrame_.numPushed = 0;

	if constexpr (BONE_USE_MULTITHREADING) {
		JobPool& pool = threading::getJobPool();
		const uint32_t numSlices = instanceSlices_.size();

		auto &firstSlice = instanceSlices_[0];
		firstSlice.boneTree = this;
		firstSlice.startInstance = 0;
		firstSlice.endInstance = std::min(sliceSize_, numInstances_);
		firstSlice.dt_ms = dt_ms;

		for (uint32_t i = 1; i < numSlices; ++i) {
			auto &slice = instanceSlices_[i];
			slice.boneTree = this;
			slice.startInstance = i * sliceSize_;
			slice.endInstance = std::min(slice.startInstance + sliceSize_, numInstances_);
			slice.dt_ms = dt_ms;
			pool.addJobPreFrame(jobFrame_, Job{
				.fn = boneSliceJob,
				.arg = &slice
			});
		}

		// Execute jobs
		pool.beginFrame(jobFrame_, 1u); // one local job
		FramedJob localJob = { { boneSliceJob, &firstSlice }, &jobFrame_ };
		do {
			pool.performJob(localJob);
		} while (pool.stealJob(localJob));
		pool.endFrame(jobFrame_);
	}
	else {
		// Single-threaded update
		if (hasScalingKeys_) [[unlikely]] {
			updateBoneTree<true>(0, numInstances_, dt_ms);
		} else [[likely]] {
			updateBoneTree<false>(0, numInstances_, dt_ms);
		}
	}

#ifdef BONE_TREE_DEBUG_TIME
	elapsedTime.push("bone-tree-update");
	elapsedTime.endFrame();
#endif
}

namespace regen {
	template<typename T> static REGEN_FORCE_INLINE
	float computeBoneInterpolation(ActiveBoneRange &ar, const Stamped<T> &key, const Stamped<T> &nextKey) {
		double timeDifference = nextKey.time - key.time;
		if (timeDifference < 0.0) {
			timeDifference += ar.duration_;
		}
		float facOut = static_cast<float>((ar.timeInTicks_ - key.time) / timeDifference);
		return facOut > 0.0 ? facOut : 0.0f;
	}

	template <typename T> T boneInterpolateTyped(ActiveBoneRange&, const Stamped<T>&, const Stamped<T>&);

	template<> Vec3f boneInterpolateTyped<Vec3f>(ActiveBoneRange &ar,
				const Stamped<Vec3f> &key, const Stamped<Vec3f> &nextKey) {
		const float fac = computeBoneInterpolation<Vec3f>(ar, key, nextKey);
		return key.value + (nextKey.value - key.value) * fac;
	}

	template<> Quaternion boneInterpolateTyped<Quaternion>(ActiveBoneRange &ar,
				const Stamped<Quaternion> &key, const Stamped<Quaternion> &nextKey) {
		float fac = computeBoneInterpolation<Quaternion>(ar, key, nextKey);
		Quaternion value;
		if constexpr(BONE_INTERPOLATE_QUATERNION_LINEAR) {
			value.interpolateLinear(key.value, nextKey.value, fac);
		} else {
			value.interpolate(key.value, nextKey.value, fac);
		}
		return value;
	}

	template <typename T, int I>
	static REGEN_FORCE_INLINE  T boneInterpolate(ActiveBoneRange &ar,
				const std::vector<Stamped<T>> &keys, uint32_t i) {
		const uint32_t keyCount = keys.size();
		const uint32_t lastFrame = ar.lastFramePosition_[i][I];

		// Wrap around if needed, else start from last frame
		// Note: ar.lastTime_ stores last time in ticks.
		const uint32_t startFrame = (ar.timeInTicks_ >= ar.lastTime_ ?
				lastFrame : ar.startFramePosition_[i][I]);
		// Find present frame number.
		uint32_t frame;
		for (frame = startFrame; frame < keyCount - 1; frame++) {
			if (ar.timeInTicks_ < keys[frame + 1].time) break;
		}
		// Store active frame for next time
		ar.lastFramePosition_[i][I] = frame;

		// interpolate between this frame's value and next frame's value
		const Stamped<T> &key = keys[frame];
		const Stamped<T> &nextKey = keys[(frame + 1) % keyCount];
		return boneInterpolateTyped<T>(ar, key, nextKey);
	}
}

template <bool HasScaleKeys>
void BoneTree::updateLocalTransforms(BoneTreeTraversal &td, uint32_t itemBase, InstanceData &instance) {
	const auto numNodes = static_cast<uint32_t>(nodes_.size());
	const auto numRanges = instance.numActiveRanges_;
	const Mat4f* __restrict localTF = localTransform_.data();
	Mat4f* __restrict blendedTF = blendedTransforms_.data() + itemBase;
	int8_t* __restrict localDirty = td.localDirty.data();
	Track* animTracks = animTracks_.data();

	Quaternion &accRot = td.accRot;;
	Vec3f &accPos = td.accPos;
	Vec3f &accScale = td.accScale;

	for (uint32_t nodeIdx = 0; nodeIdx < numNodes; nodeIdx++) {
		// Compute per-bone effective weights
		float weightSum = 0.0f;
		for (uint32_t rangeIdx : instance.activeRanges_) {
			const ActiveBoneRange &range = instance.ranges_[rangeIdx];
			auto &track = animTracks[range.trackIdx_];
			if (track.staticData_->channels[nodeIdx].isAnimated == false) {
				continue;
			}
			if (range.nodeWeights_) {
				weightSum += range.weight_ * range.nodeWeights_[nodeIdx];
			} else {
				weightSum += range.weight_;
			}
		}
		if (weightSum < 1e-5f) {
			// fallback to local transform for this node (avoid stale data)
			blendedTF[nodeIdx] = localTF[nodeIdx];
			continue;
		}
		// Compute inverse of total weight
		weightSum = 1.0f / weightSum;

		accRot = Quaternion{0.0f, 0.0f, 0.0f, 0.0f};
		accPos = Vec3f{0.0f, 0.0f, 0.0f};
		if constexpr (HasScaleKeys) {
			accScale = Vec3f{0.0f, 0.0f, 0.0f};
		}

		for (uint32_t rangeIdx : instance.activeRanges_) {
			ActiveBoneRange &range = instance.ranges_[rangeIdx];
			const Track &track = animTracks[range.trackIdx_];
			const AnimationChannel &channel = track.staticData_->channels[nodeIdx];

			// Compute normalized weight for this bone.
			float weight = range.weight_;
			if (range.nodeWeights_) {
				weight *= range.nodeWeights_[nodeIdx];
			}
			weight *= weightSum;

			// Accumulate position
			accPos += boneInterpolate<Vec3f,0>(range, channel.positionKeys_, nodeIdx) * weight;
			// Accumulate rotation
			Quaternion rotation = boneInterpolate<Quaternion,1>(range, channel.rotationKeys_, nodeIdx);
			if (accRot.dot(rotation) < 0.0f) { rotation = -rotation; }
			accRot += rotation * weight;
			// Accumulate scaling
			if constexpr (HasScaleKeys) {
				accScale += boneInterpolate<Vec3f,2>(range, channel.scalingKeys_, nodeIdx) * weight;
			}
		}

		// Finally, write blended local transform
		{
			localDirty[nodeIdx] = true;
			if (numRanges > 1) {
				accRot.normalize();
			}
			Mat4f &m = blendedTF[nodeIdx];
			m = accRot.calculateMatrix();
			if constexpr (HasScaleKeys) {
				m.scale(accScale);
			}
			m.x[3] = accPos.x;
			m.x[7] = accPos.y;
			m.x[11] = accPos.z;
		}
	}
}

void BoneTree::updateGlobalTransforms(BoneTreeTraversal &td, uint32_t itemBase) {
	const auto numNodes = static_cast<uint32_t>(nodes_.size());
	const Mat4f* __restrict offsetMatrix = offsetMatrix_.data();
	Mat4f* __restrict blendedTF  = blendedTransforms_.data() + itemBase;
	Mat4f* __restrict boneMatrix = boneMatrix_.data() + itemBase;
	int32_t* __restrict nodeParent = nodeParent_.data();
	int8_t* __restrict localDirty  = td.localDirty.data();
	int8_t* __restrict globalDirty = td.globalDirty.data();
	int8_t* __restrict isBoneNode = isBoneNode_.data();

	if constexpr(!BONE_LOCAL_ROOT_IS_IDENTITY) {
		td.rootInverse = localTransform_[BONE_ROOT_NODE_IDX].inverse();
	}
	if (isBoneNode[BONE_ROOT_NODE_IDX]) {
		// for root bone, just use offset matrix
		boneMatrix[BONE_ROOT_NODE_IDX] = offsetMatrix[BONE_ROOT_NODE_IDX];
	}

	for (uint32_t nodeIdx = 1; nodeIdx < numNodes; nodeIdx++) {
		const int32_t parentIdx = nodeParent[nodeIdx];

		if constexpr (BONE_DEBUG_CULLING) { td.numNodes++; }
		// skip if both local and parent global unchanged
		if (!localDirty[nodeIdx] && !globalDirty[parentIdx]) { continue; }
		if constexpr (BONE_DEBUG_CULLING) { td.numTraversed++; }

		// update node global transform
		blendedTF[nodeIdx].multiplyr(blendedTF[parentIdx]);
		globalDirty[nodeIdx] = 1;
		localDirty[nodeIdx] = 0;

		if (isBoneNode[nodeIdx]) {
			// Bone matrices transform from mesh coordinates in bind pose
			// to mesh coordinates in skinned pose
			if constexpr(BONE_LOCAL_ROOT_IS_IDENTITY) {
				boneMatrix[nodeIdx] = (blendedTF[nodeIdx] * offsetMatrix[nodeIdx]).transpose();
			} else {
				boneMatrix[nodeIdx] = (td.rootInverse * blendedTF[nodeIdx] * offsetMatrix[nodeIdx]).transpose();
			}
		}

		// after traversal, clear dirty flag
		globalDirty[nodeIdx] = 0;
	}

	// Also clear root dirty flag
	globalDirty[BONE_ROOT_NODE_IDX] = 0;

	if constexpr (BONE_DEBUG_CULLING) {
		if (td.numNodes > 0) {
			REGEN_INFO("Traversal rate: " <<
				static_cast<float>(td.numTraversed) / static_cast<float>(td.numNodes) * 100.0f << "%" <<
				" (" << td.numTraversed << " / " << td.numNodes << ")");
		}
	}
}

template <bool HasScaleKeys>
void BoneTree::updateBoneTree(uint32_t instanceBase, uint32_t numInstances, double dt_ms) {
	const uint32_t instanceEnd = instanceBase + numInstances;
	const auto numNodes = static_cast<uint32_t>(nodes_.size());

	InstanceData* __restrict instanceData = instanceData_.data();
	Track* __restrict animTracks = animTracks_.data();

	thread_local BoneTreeTraversal td;
	// make sure traversal data arrays are large enough
	if (td.localDirty.size() < numNodes) {
		td.localDirty.resize(numNodes, false);
		td.globalDirty.resize(numNodes, false);
	}

	for (uint32_t instanceIdx = instanceBase; instanceIdx < instanceEnd; instanceIdx++) {
		const uint32_t itemBase = instanceIdx * numNodes;
		auto &instance = instanceData[instanceIdx];
		const auto numRanges = instance.numActiveRanges_;

		if (numRanges == 0) continue;

		// Pass 1: Advance time in active ranges
		for (uint32_t rangeIdx : instance.activeRanges_) {
			ActiveBoneRange &range = instance.ranges_[rangeIdx];
			const Track &track = animTracks[range.trackIdx_];
			range.elapsedTime_ += dt_ms;
			range.lastTime_ = range.timeInTicks_;
			// Map into anim's duration
			range.timeInTicks_ = std::min(range.duration_,
				range.elapsedTime_ * timeFactor_ * track.ticksPerSecond_);
			range.timeInTicks_ += range.startTick_;
		}

		// Pass 2: Update local node transforms
		updateLocalTransforms<HasScaleKeys>(td, itemBase, instance);

		// Pass 3: Update global node transforms
		updateGlobalTransforms(td, itemBase);

		// Stop finished animations
		for (uint32_t rangeIdx : instance.activeRanges_) {
			const ActiveBoneRange &range = instance.ranges_[rangeIdx];
			if ((range.timeInTicks_ - range.startTick_) >= range.duration_ - 1e-5) {
				stopBoneAnimation(instanceIdx, static_cast<AnimationHandle>(rangeIdx));
			}
		}
	}
}
