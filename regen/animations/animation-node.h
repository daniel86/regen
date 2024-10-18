/*
 * animation-node.h
 *
 *  Created on: 29.03.2012
 *      Author: daniel
 */

#ifndef ANIMATION_NODE_H_
#define ANIMATION_NODE_H_

#include <regen/utility/ref-ptr.h>
#include <regen/math/matrix.h>
#include <regen/math/quaternion.h>
#include <regen/animations/animation.h>

#include <map>
#include <vector>

namespace regen {
	/**
	 * \brief A node in a skeleton with parent and children.
	 */
	class AnimationNode {
	public:
		/**
		 * @param name the node name.
		 * @param parent the parent node.
		 */
		AnimationNode(const std::string &name, const ref_ptr<AnimationNode> &parent);

		/**
		 * @return The node name.
		 */
		const std::string &name() const;

		/**
		 * @return The parent node.
		 */
		const ref_ptr<AnimationNode> &parent() const;

		/**
		 * Add a node child.
		 * @param child
		 */
		void addChild(const ref_ptr<AnimationNode> &child);

		/**
		 * @return node children.
		 */
		std::vector<ref_ptr<AnimationNode> > &children();

		/**
		 * Sets the currently active animation channel
		 * for this node.
		 * Should be called after animation index changed.
		 */
		void set_channelIndex(GLint channelIndex);

		/**
		 * @return local transform.
		 */
		const Mat4f &localTransform() const;

		/**
		 * @param v the local transform.
		 */
		void set_localTransform(const Mat4f &v);

		/**
		 * @return global transform in world space.
		 */
		const Mat4f &globalTransform() const;

		/**
		 * @param v the global transform.
		 */
		void set_globalTransform(const Mat4f &v);

		/**
		 * @return offsetMatrix * nodeTransform * inverseTransform
		 */
		const Mat4f &boneTransformationMatrix() const;

		/**
		 * @return Matrix that transforms from mesh space to bone space in bind pose.
		 */
		const Mat4f &boneOffsetMatrix() const;

		/**
		 * @param offsetMatrix Matrix that transforms from mesh space to bone space in bind pose.
		 */
		void set_boneOffsetMatrix(const Mat4f &offsetMatrix);

		/**
		 * Recursively updates the internal node transformations from the given matrix array.
		 * @param transforms transformation matrices.
		 */
		void updateTransforms(const std::vector<Mat4f> &transforms);

		/**
		 * Concatenates all parent transforms to get the global transform for this node.
		 */
		void calculateGlobalTransform();

		/**
		 * Create a copy of this node.
		 * Used for instanced animations.
		 * @return a node copy.
		 */
		ref_ptr<AnimationNode> copy();

	protected:
		std::string name_;

		ref_ptr<AnimationNode> parentNode_;
		std::vector<ref_ptr<AnimationNode> > nodeChilds_;

		Mat4f localTransform_;
		Mat4f globalTransform_;
		Mat4f offsetMatrix_;
		Mat4f boneTransformationMatrix_;
		GLint channelIndex_;

		GLboolean isBoneNode_;
	};

	/**
	 * \brief A skeletal animation.
	 */
	class NodeAnimation : public Animation {
	public:
		/**
		 * \brief A key frame containing a 3 dimensional vector.
		 */
		struct KeyFrame3f {
			GLdouble time; /**< frame timestamp. **/
			Vec3f value; /**< frame value. **/
		};
		/**
		 * \brief Key frame of bone rotation.
		 */
		struct KeyFrameQuaternion {
			GLdouble time; /**< frame timestamp. **/
			Quaternion value; /**< frame value. **/
		};
		/**
		 * Defines behavior for first or last key frame.
		 */
		enum Behavior {
			/**
			 * The value from the default node transformation is taken.
			 */
			BEHAVIOR_DEFAULT = 0x0,
			/**
			 * The nearest key value is used without interpolation.
			 */
			BEHAVIOR_CONSTANT = 0x1,
			/**
			 * The value of the nearest two keys is linearly
			 * extrapolated for the current time value.
			 */
			BEHAVIOR_LINEAR = 0x2,
			/**
			 * The animation is repeated.
			 * If the animation key go from n to m and the current
			 * time is t, use the value at (t-n) % (|m-n|).
			 */
			BEHAVIOR_REPEAT = 0x3
		};
		/**
		 * \brief Each channel affects a single node.
		 */
		struct Channel {
			/**
			 * The name of the node affected by this animation. The node
			 * must exist and it must be unique.
			 */
			std::string nodeName_;
			/**
			 * Defines how the animation behaves after the last key was processed.
			 * The default value is ANIM_BEHAVIOR_DEFAULT
			 * (the original transformation matrix of the affected node is taken).
			 */
			Behavior postState;
			/**
			 * Defines how the animation behaves before the first key is encountered.
			 * The default value is ANIM_BEHAVIOR_DEFAULT
			 * (the original transformation matrix of the affected node is used).
			 */
			Behavior preState;
			ref_ptr<std::vector<KeyFrame3f> > scalingKeys_; /**< Scaling key frames. */
			ref_ptr<std::vector<KeyFrame3f> > positionKeys_; /**< Position key frames. */
			ref_ptr<std::vector<KeyFrameQuaternion> > rotationKeys_; /**< Rotation key frames. */
		};

		/**
		 * @param rootNode animation tree.
		 * @param autoStart is true the animation adds itself to the AnimationManager.
		 */
		explicit NodeAnimation(const ref_ptr<AnimationNode> &rootNode, GLboolean autoStart = GL_TRUE);

		/**
		 * @param autoStart is true the animation adds itself to the AnimationManager.
		 * @return a copy of this animation.
		 */
		ref_ptr<NodeAnimation> copy(GLboolean autoStart = GL_TRUE);

		/**
		 * Add an animation.
		 * @param animationName the animation name.
		 * @param channels the key frames.
		 * @param duration animation duration.
		 * @param ticksPerSecond number of animation ticks per second.
		 * @return the animation index.
		 */
		GLint addChannels(
				const std::string &animationName,
				ref_ptr<std::vector<Channel> > &channels,
				GLdouble duration,
				GLdouble ticksPerSecond
		);

		/**
		 * Sets animation tick range.
		 * @param name the animation name.
		 * @param tickRange the tick range.
		 */
		void setAnimationActive(const std::string &name, const Vec2d &tickRange);

		/**
		 * Sets animation tick range.
		 * @param index the animation index.
		 * @param tickRange the tick range.
		 */
		void setAnimationIndexActive(GLint index, const Vec2d &tickRange);

		/**
		 * Sets tick range for the currently activated
		 * animation index.
		 * @param forcedTickRange the tick range.
		 */
		void setTickRange(const Vec2d &forcedTickRange);

		/**
		 * @param timeFactor the slow down (<1.0) / speed up (>1.0) factor.
		 */
		void set_timeFactor(GLdouble timeFactor);

		/**
		 * @return the slow down (<1.0) / speed up (>1.0) factor.
		 */
		GLdouble timeFactor() const;

		/**
		 * Find node with given name.
		 * @param name the node name.
		 * @return the node or a null reference.
		 */
		ref_ptr<AnimationNode> findNode(const std::string &name);

		// override
		void animate(GLdouble dt) override;

	protected:
		// forward declaration
		struct Data {
			// string identifier for animation
			std::string animationName_;
			// flag indicating if this animation is active
			GLboolean active_;
			// milliseconds from start of animation
			GLdouble elapsedTime_;
			GLdouble ticksPerSecond_;
			GLdouble lastTime_;
			// Duration of the animation in ticks.
			GLdouble duration_;
			// local node transformation
			std::vector<Mat4f> transforms_;
			// remember last frame for interpolation
			std::vector<Vec3ui> lastFramePosition_;
			std::vector<Vec3ui> startFramePosition_;
			ref_ptr<std::vector<NodeAnimation::Channel> > channels_;
		};

		ref_ptr<AnimationNode> rootNode_;

		GLint animationIndex_;
		std::vector<ref_ptr<NodeAnimation::Data> > animData_;

		std::map<std::string, AnimationNode *> nameToNode_;
		std::map<std::string, GLint> animNameToIndex_;

		// config for currently active anim
		GLdouble startTick_;
		GLdouble duration_;
		GLdouble timeFactor_;
		Vec2d tickRange_;

		Quaternion nodeRotation(
				NodeAnimation::Data &anim,
				const Channel &channel,
				GLdouble timeInTicks,
				GLuint i);

		Vec3f nodePosition(
				NodeAnimation::Data &anim,
				const Channel &channel,
				GLdouble timeInTicks,
				GLuint i);

		Vec3f nodeScaling(
				NodeAnimation::Data &anim,
				const Channel &channel,
				GLdouble timeInTicks,
				GLuint i);

		ref_ptr<AnimationNode> findNode(ref_ptr<AnimationNode> &n, const std::string &name);

		void deallocateAnimationAtIndex(GLint animationIndex);

		void stopNodeAnimation(NodeAnimation::Data &anim);
	};

	std::ostream &operator<<(std::ostream &out, const NodeAnimation::Behavior &v);

	std::istream &operator>>(std::istream &in, NodeAnimation::Behavior &v);
} // namespace

#endif /* ANIMATION_NODE_H_ */
