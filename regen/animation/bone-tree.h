#ifndef REGEN_BONE_TREE_H_
#define REGEN_BONE_TREE_H_

#include <regen/utility/ref-ptr.h>
#include <regen/math/matrix.h>
#include <regen/math/quaternion.h>
#include <regen/animation/animation.h>
#include <regen/animation/animation-range.h>

#include <unordered_map>
#include <vector>

#include "animation-channel.h"

namespace regen {
	/**
	 * \brief A node in a skeleton with parent and children.
	 */
	struct BoneNode {
		// The node name.
		std::string name;
		// The index of the node in the BoneTree node array.
		uint32_t nodeIdx = 0;
		// The parent node.
		ref_ptr<BoneNode> parent;
		// The node children.
		std::vector<ref_ptr<BoneNode>> children;

		/**
		 * @param name the node name.
		 * @param parent the parent node.
		 */
		BoneNode(const std::string &name, const ref_ptr<BoneNode> &parent)
			: name(name), parent(parent) {}

		/**
		 * Add a node child.
		 * @param child
		 */
		void addChild(const ref_ptr<BoneNode> &child) { children.push_back(child); }
	};

	struct ActiveBoneRange {
		// -1 indicates that the range is inactive
		int32_t trackIdx_ = -1;
		// config for currently active anim
		double startTick_ = 0.0;
		double duration_ = 0.0;
		Vec2d tickRange_ = Vec2d(0.0, 0.0);
		// milliseconds from start of animation
		double elapsedTime_ = 0.0;
		// last update time in ticks
		double lastTime_ = 0.0;
		// the current time in ticks
		double timeInTicks_ = 0.0;
		// Overall weight for this animation range, weights of nodes
		// are multiplied with this.
		float weight_ = 1.0;
		// Per node weights for blending.
		float *nodeWeights_;
		// Remember last frame of each channel for interpolation.
		std::vector<Vec3ui> lastFramePosition_;
		std::vector<Vec3ui> startFramePosition_;
	};

	struct BoneTreeTraversal;
	class BoneTree;

	/**
	 * \brief A slice of a bone tree for multi-threaded updates.
	 */
	struct BoneTreeSlice {
		BoneTree* boneTree;
		uint32_t startInstance;
		uint32_t endInstance;
		float dt_ms;
	};

	/**
	 * \brief A skeletal animation.
	 */
	class BoneTree : public Animation {
	public:
		/**
		 * The handle for an active animation.
		 * This is returned by startBoneAnimation() and used to
		 * identify the animation in other functions.
		 */
		using AnimationHandle = int32_t;

		/**
		 * Construct a bone tree.
		 * The root node is the first node in the nodes array.
		 * Make sure that in the array i < j for any node i being parent of node j.
		 * @param nodes the array of bone nodes.
		 * @param numInstances the number of instances.
		 */
		explicit BoneTree(const std::vector<ref_ptr<BoneNode>> &nodes, uint32_t numInstances = 1);

		/**
		 * @return the root node.
		 */
		BoneNode* rootNode() const { return rootNode_.get(); }

		/**
		 * @return the number of nodes in the bone tree.
		 */
		uint32_t numNodes() const { return static_cast<uint32_t>(nodes_.size()); }

		/**
		 * @return the number of instances.
		 */
		uint32_t numInstances() const { return numInstances_; }

		/**
		 * @param instanceID the instance index.
		 * @param nodeIdx the node index.
		 * @return the bone matrix for the given node and instance.
		 */
		const Mat4f &boneMatrix(uint32_t instanceID, uint32_t nodeIdx) const {
			const auto numNodes = static_cast<uint32_t>(nodes_.size());
			return boneMatrix_[instanceID * numNodes + nodeIdx];
		}

		/**
		 * Set the offset matrix for a given node.
		 * @param nodeIdx the node index.
		 * @param offset the offset matrix.
		 */
		void setOffsetMatrix(uint32_t nodeIdx, const Mat4f &offset) {
			offsetMatrix_[nodeIdx] = offset;
		}

		/**
		 * @param nodeIdx the node index.
		 * @return the offset matrix for the given node.
		 */
		const Mat4f &offsetMatrix(uint32_t nodeIdx) const {
			return offsetMatrix_[nodeIdx];
		}

		/**
		 * Set the local transform for a given node.
		 * @param nodeIdx the node index.
		 * @param tf the local transform.
		 */
		void setLocalTransform(uint32_t nodeIdx, const Mat4f &tf) {
			localTransform_[nodeIdx] = tf;
		}

		/**
		 * Mark a node as bone node or not.
		 * @param nodeIdx the node index.
		 * @param isBone true if the node is a bone node.
		 */
		void setIsBoneNode(uint32_t nodeIdx, bool isBone) {
			isBoneNode_[nodeIdx] = isBone;
		}

		/**
		 * @param nodeIdx the node index.
		 * @return true if the node is a bone node.
		 */
		bool isBoneNode(uint32_t nodeIdx) const {
			return isBoneNode_[nodeIdx];
		}

		/**
		 * Add an animation.
		 * @param trackName the track name.
		 * @param animData the static animation data.
		 * @param duration animation duration.
		 * @param ticksPerSecond number of animation ticks per second.
		 * @return the animation index.
		 */
		int32_t addAnimationTrack(
				const std::string &trackName,
				ref_ptr<StaticAnimationData> &animData,
				double duration,
				double ticksPerSecond);

		/**
		 * Get the index of an animation track by name.
		 * @param trackName the track name.
		 * @return the track index or -1 if not found.
		 */
		int32_t getTrackIndex(const std::string &trackName) const;

		/**
		 * Start an animation within a given track.
		 * Multiple animations can be active at the same time.
		 * @param instanceIdx the instance index.
		 * @param trackIdx the animation track index as returned by getTrackIndex().
		 * @param tickRange the tick range, negative values indicate full range.
		 * @param nodeWeights optional per-node weights for blending, if null all nodes have weight 1.0.
		 * @return the animation handle or -1 if the animation could not be started.
		 */
		AnimationHandle startBoneAnimation(
			uint32_t instanceIdx, int32_t trackIdx,
			const Vec2d &tickRange,
			float *nodeWeights=nullptr);

		/**
		 * Stop an active animation.
		 * @param instanceIdx the instance index.
		 * @param animHandle the animation handle as returned by startBoneAnimation().
		 */
		void stopBoneAnimation(uint32_t instanceIdx, AnimationHandle animHandle);

		/**
		 * @param instanceIdx the instance index.
		 * @param animHandle the animation handle as returned by startBoneAnimation().
		 * @return true if the animation is active.
		 */
		bool isBoneAnimationActive(uint32_t instanceIdx, AnimationHandle animHandle) const;

		/**
		 * @param instanceIdx the instance index.
		 * @param animHandle the animation handle as returned by startBoneAnimation().
		 * @return the weight of the animation in the range [0.0 .. 1.0].
		 */
		float animationWeight(uint32_t instanceIdx, AnimationHandle animHandle) const;

		/**
		 * Sets the weight of an active animation.
		 * This is used for blending multiple animations.
		 * @param instanceIdx the instance index.
		 * @param animHandle the animation handle as returned by startBoneAnimation().
		 * @param weight the weight of the animation in the range [0.0 .. 1.0].
		 */
		void setAnimationWeight(uint32_t instanceIdx, AnimationHandle animHandle, float weight);

		/**
		 * Sets tick range for a given animation handle.
		 * @param instanceIdx the instance index.
		 * @param animHandle the animation handle.
		 * @param tickRange the tick range, negative values indicate full range.
		 */
		void setTickRange(uint32_t instanceIdx, AnimationHandle animHandle, const Vec2d &tickRange);

		/**
		 * @param timeFactor the slow down (<1.0) / speed up (>1.0) factor.
		 */
		void setTimeFactor(double timeFactor) { timeFactor_ = timeFactor * 0.001; }

		/**
		 * @return the slow down (<1.0) / speed up (>1.0) factor.
		 */
		double timeFactor() const { return timeFactor_ * 1000.0; }

		/**
		 * Each track has a name and contains multiple channels.
		 * Only one track can be active at a time, however multiple ranges within
		 * one track can be active.
		 * @return the number of animation tracks.
		 */
		uint32_t numAnimationTracks() const {
			return static_cast<uint32_t>(animTracks_.size());
		}

		/**
		 * @return the number of currently active animation ranges.
		 */
		uint32_t numActiveRanges(uint32_t instanceIdx) const {
			return instanceData_[instanceIdx].numActiveRanges_;
		}

		/**
		 * @return the elapsed time of an active animation in milliseconds.
		 */
		double elapsedTime(uint32_t instanceIdx, AnimationHandle animHandle) const;

		/**
		 * @param trackIdx the animation track index.
		 * @return the ticks per second of the given animation track.
		 */
		double ticksPerSecond(uint32_t trackIdx) const;

		// override
		void cpuUpdate(double dt) override;

		/**
		 * Find node with given name.
		 * @param name the node name.
		 * @return the node or a null reference.
		 */
		ref_ptr<BoneNode> findNode(const std::string &name);

		/**
		 * Event data for node animation events.
		 */
		class BoneEvent : public EventData {
		public:
			BoneEvent() = default;
			~BoneEvent() override = default;
			uint32_t instanceIdx = 0;
			uint32_t rangeIdx = 0;
		};

	protected:
		std::vector<ref_ptr<BoneNode>> nodes_;
		ref_ptr<BoneNode> rootNode_;
		std::unordered_map<std::string, uint32_t> nameToNode_;

		uint32_t numInstances_ = 1;
		double timeFactor_ = 0.001;
		bool hasScalingKeys_ = false;

		/**
		 * A track in the bone animation.
		 * One animation may have different tracks, each with its own set of channels.
		 * Multiple tracks may be active at the same time, in which case their
		 * they are blended according to their weights.
		 */
		struct Track {
			// string identifier for animation
			std::string trackName_;
			double ticksPerSecond_;
			// Duration of the animation in ticks.
			double duration_;
			// Static animation data
			ref_ptr<StaticAnimationData> staticData_;
		};
		std::vector<Track> animTracks_;
		std::unordered_map<std::string, int32_t> trackNameToIndex_;

		struct InstanceData {
			// An array of active ranges, however the array only grows so we may have
			// inactive ranges in between.
			std::vector<ActiveBoneRange> ranges_;
			// The number of active ranges, i.e. animations that are active in parallel.
			// This is usually a very small number.
			uint32_t numActiveRanges_ = 0;
			// Indices into ranges_ that are currently active.
			std::vector<int32_t> activeRanges_;
			std::vector<uint32_t> handleToActiveRange_;
		};
		std::vector<InstanceData> instanceData_;
		ref_ptr<BoneEvent> eventData_ = ref_ptr<BoneEvent>::alloc();

		std::vector<BoneTreeSlice> instanceSlices_;
		uint32_t sliceSize_;

		// per-node data arrays
		// Matrix that transforms from mesh space to bone space in bind pose.
		std::vector<Mat4f> localTransform_;
		std::vector<Mat4f> offsetMatrix_;
		std::vector<int32_t> nodeParent_;
		std::vector<int8_t> isBoneNode_;

		// per-instance data arrays in node-major layout, i.e. numInstances x numNodes
		std::vector<uint32_t> numActive_;
		std::vector<Mat4f> boneMatrix_;
		std::vector<Mat4f> blendedTransforms_;

		void setTickRange(ActiveBoneRange &ar, const Vec2d &forcedTickRange);

		template <bool HasScaleKeys>
		void updateBoneTree(uint32_t firstInstance, uint32_t numInstances, double dt_ms);

		template <bool HasScaleKeys>
		void updateLocalTransforms(BoneTreeTraversal &td, uint32_t instanceIdx, InstanceData &instance);

		void updateGlobalTransforms(BoneTreeTraversal &td, uint32_t instanceIdx);

		friend struct BoneSliceJob;
	};
} // namespace

#endif /* REGEN_BONE_TREE_H_ */
