/*
 * animation-node.h
 *
 *  Created on: 29.03.2012
 *      Author: daniel
 */

#ifndef ANIMATION_NODE_H_
#define ANIMATION_NODE_H_

#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/matrix.h>
#include <ogle/algebra/quaternion.h>
#include <ogle/animations/animation.h>

#include <map>
#include <vector>
using namespace std;

/**
 * A node in a skeleton with parent and children.
 */
class AnimationNode
{
public:
  AnimationNode(
      const string &name,
      ref_ptr<AnimationNode> &parent);

  /**
   * The node name.
   */
  const string& name() const;

  /**
   * The parent node.
   */
  ref_ptr<AnimationNode> parent();

  /**
   * Add a node child.
   */
  void addChild(ref_ptr<AnimationNode> &child);
  /**
   * Handle to node children.
   */
  vector< ref_ptr<AnimationNode> >& children();

  /**
   * Sets the currently active animation channel
   * for this node.
   * Should be called after animation index changed.
   */
  void set_channelIndex(GLint channelIndex);

  /**
   * most recently calculated local transform.
   */
  const Mat4f& localTransform();
  /**
   * most recently calculated local transform.
   */
  void set_localTransform(const Mat4f &v);

  /**
   * most recently calculated global transform in world space.
   */
  const Mat4f& globalTransform();
  /**
   * most recently calculated global transform in world space.
   */
  void set_globalTransform(const Mat4f &v);

  /**
   * boneOffsetMatrix * nodeTransform * inverseTransform.
   */
  const Mat4f& boneTransformationMatrix() const;

  /**
   * Matrix that transforms from mesh space to bone space in bind pose.
   */
  const Mat4f& boneOffsetMatrix() const;
  /**
   * Matrix that transforms from mesh space to bone space in bind pose.
   */
  void set_boneOffsetMatrix(const Mat4f &offsetMatrix);

  /**
   * Recursively updates the internal node transformations from the given matrix array
   */
  void updateTransforms(const std::vector<Mat4f> &transforms);
  /**
   * Recursively updates the transformation matrix of this node.
   */
  void updateBoneTransformationMatrix(const Mat4f &rootInverse);
  /**
   * Concatenates all parent transforms to get the global transform for this node.
   */
  void calculateGlobalTransform();

protected:
  string name_;

  ref_ptr<AnimationNode> parentNode_;
  vector< ref_ptr<AnimationNode> > nodeChilds_;

  Mat4f localTransform_;
  Mat4f globalTransform_;
  Mat4f offsetMatrix_;
  Mat4f boneTransformationMatrix_;
  GLint channelIndex_;

  GLboolean isBoneNode_;
};

// forward declaration
struct NodeAnimationData;

/**
 * Key frame base class. Has just a time stamp.
 */
class NodeKeyFrame
{
public:
  GLdouble time;
};

/**
 * A key frame with a 3 dimensional vector
 */
class NodeKeyFrame3f : public NodeKeyFrame
{
public:
  Vec3f value;
};
/**
 * Key frame of bone scaling.
 */
typedef NodeKeyFrame3f NodeScalingKey;
/**
 * Key frame of bone position.
 */
typedef NodeKeyFrame3f NodePositionKey;

/**
 * Key frame of bone rotation.
 */
class NodeQuaternionKey : public NodeKeyFrame {
public:
  Quaternion value;
};

enum AnimationBehaviour {
  // The value from the default node transformation is taken.
  ANIM_BEHAVIOR_DEFAULT = 0x0,
  // The nearest key value is used without interpolation.
  ANIM_BEHAVIOR_CONSTANT = 0x1,
  // The value of the nearest two keys is linearly
  // extrapolated for the current time value.
  ANIM_BEHAVIOR_LINEAR = 0x2,
  // The animation is repeated.
  // If the animation key go from n to m and the current
  // time is t, use the value at (t-n) % (|m-n|).
  ANIM_BEHAVIOR_REPEAT = 0x3
};

/**
 * Each channel affects a single node.
 */
struct NodeAnimationChannel {
  /**
   * The name of the node affected by this animation. The node
   * must exist and it must be unique.
   **/
  string nodeName_;
  // Defines how the animation behaves after the last key was processed.
  // The default value is ANIM_BEHAVIOR_DEFAULT
  // (the original transformation matrix of the affected node is taken).
  AnimationBehaviour postState;
  // Defines how the animation behaves before the first key is encountered.
  // The default value is ANIM_BEHAVIOR_DEFAULT
  // (the original transformation matrix of the affected node is used).
  AnimationBehaviour preState;
  ref_ptr< vector<NodeScalingKey> > scalingKeys_;
  ref_ptr< vector<NodePositionKey> > positionKeys_;
  ref_ptr< vector<NodeQuaternionKey> > rotationKeys_;
};

//////////////

/**
 * Skeletal animation.
 */
class NodeAnimation : public Animation
{
public:
  static GLuint ANIMATION_STOPPED;

  NodeAnimation(ref_ptr<AnimationNode> rootNode);
  ~NodeAnimation();

  /**
   * Add an animation by specifying the channels, duration and ticks per second.
   * @return the animation index
   */
  GLint addChannels(
      const string &animationName,
      ref_ptr< vector< NodeAnimationChannel> > &channels,
      GLdouble duration,
      GLdouble ticksPerSecond
      );

  /**
   * Activate an animation.
   */
  void setAnimationActive(
      const string &animationName,
      const Vec2d &forcedTickRange);
  /**
   * Activate an animation.
   */
  void setAnimationIndexActive(
      GLint animationIndex,
      const Vec2d &forcedTickRange);

  /**
   * Sets tick range for the currently activated
   * animation index.
   */
  void setTickRange(const Vec2d &forcedTickRange);

  /**
   * Slow down (<1.0) / speed up (>1.0) factor.
   */
  void set_timeFactor(GLdouble timeFactor);
  /**
   * Slow down (<1.0) / speed up (>1.0) factor.
   */
  double timeFactor() const;

  // override
  virtual void animate(GLdouble dt);

protected:
  ref_ptr<AnimationNode> rootNode_;

  GLint animationIndex_;
  vector< ref_ptr<NodeAnimationData> > animData_;

  map<string,AnimationNode*> nameToNode_;
  map<string,GLint> animNameToIndex_;

  // config for currently active anim
  GLdouble startTick_;
  GLdouble duration_;
  GLdouble timeFactor_;
  Vec2d tickRange_;

  GLuint findFrame(
      GLuint lastFrame,
      GLdouble tick,
      NodeKeyFrame *keys, GLuint numKeys);

  inline GLuint animationFrame(
      NodeAnimationData &anim,
      NodeKeyFrame *keyFrames,
      GLuint numKeyFrames,
      GLuint lastFrame,
      GLdouble timeInTicks);

  Quaternion nodeRotation(
      NodeAnimationData &anim,
      NodeAnimationChannel &channel,
      GLdouble timeInTicks,
      GLuint i);
  Vec3f nodePosition(
      NodeAnimationData &anim,
      NodeAnimationChannel &channel,
      GLdouble timeInTicks,
      GLuint i);
  Vec3f nodeScaling(
      NodeAnimationData &anim,
      NodeAnimationChannel &channel,
      GLdouble timeInTicks,
      GLuint i);

  void deallocateAnimationAtIndex(
      GLint animationIndex);
  void stopAnimation(NodeAnimationData &anim);
};

#endif /* ANIMATION_NODE_H_ */
