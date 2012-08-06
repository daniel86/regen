/*
 * bone.h
 *
 *  Created on: 29.03.2012
 *      Author: daniel
 */

#ifndef BONE_H_
#define BONE_H_

#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/matrix.h>
#include <ogle/algebra/quaternion.h>
#include <ogle/animations/animation.h>
#include <ogle/states/attribute-state.h>

#include <map>
#include <vector>
using namespace std;

/**
 * A node in the skeleton with parent and childs.
 */
class Bone {
public:
  Bone(const string &name, ref_ptr<Bone> parent)
  : parentBoneNode_(parent), name_(name), channelIndex_(-1)
  {
  }

  /**
   * The node name.
   */
  const string& name() const {
    return name_;
  }

  /**
   * The parent node.
   */
  ref_ptr<Bone> parent() {
    return parentBoneNode_;
  }

  /**
   * Add a node child.
   */
  void addChild(ref_ptr<Bone> &child);
  /**
   * Handle to node children.
   */
  vector< ref_ptr<Bone> >& boneChilds();

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
   * Matrix that transforms from mesh space to bone space in bind pose.
   */
  const Mat4f& offsetMatrix() const {
    return offsetMatrix_;
  }
  /**
   * Matrix that transforms from mesh space to bone space in bind pose.
   */
  void set_offsetMatrix(const Mat4f &offsetMatrix) {
    offsetMatrix_ = offsetMatrix;
  }

  /**
   * Recursively updates the internal node transformations from the given matrix array
   */
  void updateTransforms(const std::vector<Mat4f> &transforms);
  /**
   * Concatenates all parent transforms to get the global transform for this node.
   */
  void calculateGlobalTransform();

protected:
  string name_;

  ref_ptr<Bone> parentBoneNode_;
  vector< ref_ptr<Bone> > boneNodeChilds_;

  Mat4f localTransform_;
  Mat4f globalTransform_;
  Mat4f offsetMatrix_;
  GLint channelIndex_;
};

// forward declaration
struct BoneAnimationData;

class BoneKeyFrame {
public:
  double time;
};

class BoneKeyFrame3f : public BoneKeyFrame {
public:
  Vec3f value;
};
/**
 * Key frame of bone scaling.
 */
typedef BoneKeyFrame3f BoneScalingKey;
/**
 * Key frame of bone position.
 */
typedef BoneKeyFrame3f BonePositionKey;

/**
 * Key frame of bone rotation.
 */
class BoneQuaternionKey : public BoneKeyFrame {
public:
  Quaternion value;
};

typedef enum {
  // The value from the default node transformation is taken.
  ANIM_BEHAVIOR_DEFAULT = 0x0,
  // The nearest key value is used without interpolation.
  ANIM_BEHAVIOR_CONSTANT = 0x1,
  // The value of the nearest two keys is linearly extrapolated for the current time value.
  ANIM_BEHAVIOR_LINEAR = 0x2,
  // The animation is repeated.
  // If the animation key go from n to m and the current time is t, use the value at (t-n) % (|m-n|).
  ANIM_BEHAVIOR_REPEAT = 0x3
}AnimationBehaviour;

/**
 * Each channel affects a single node.
 */
typedef struct {
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
  ref_ptr< vector< BoneScalingKey > > scalingKeys_;
  bool isScalingCompleted;
  ref_ptr< vector< BonePositionKey > > positionKeys_;
  bool isPositionCompleted;
  ref_ptr< vector< BoneQuaternionKey > > rotationKeys_;
  bool isRotationCompleted;
}BoneAnimationChannel;

//////////////

/**
 * Manages bone animations.
 */
class BoneAnimation : public Animation {
public:
  static unsigned int ANIMATION_STOPPED;

  BoneAnimation(
      list< AttributeState* > &meshes,
      ref_ptr< Bone > rootBoneNode
      );
  ~BoneAnimation();

  /**
   * Add an animation by specifying the channels, duration and ticks per second.
   * @return the animation index
   */
  GLint addChannels(
      const string &animationName,
      ref_ptr< vector< BoneAnimationChannel> > &channels,
      double duration,
      double ticksPerSecond
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

  void setTickRange(const Vec2d &forcedTickRange);

  void deallocateAnimationAtIndex(GLint animationIndex);

  void set_timeFactor(double timeFactor) {
    timeFactor_ = timeFactor;
  }
  double timeFactor() const {
    return timeFactor_;
  }

protected:
  list< AttributeState* > meshes_;
  ref_ptr< Bone > rootBoneNode_;

  GLint animationIndex_;
  vector< ref_ptr<BoneAnimationData> > animData_;

  map<string,Bone*> nameToNode_;
  map<string,GLint> animNameToIndex_;

  // config for currently active anim
  double startTick_;
  double duration_;
  double timeFactor_;
  Vec2d tickRange_;

  GLuint findFrame(
      GLuint lastFrame,
      double tick,
      BoneKeyFrame *keys, GLuint numKeys);

  // override
  virtual void doAnimate(const double &dt);

  inline GLuint animationFrame(
      BoneAnimationData &anim,
      BoneKeyFrame *keyFrames,
      unsigned int numKeyFrames,
      unsigned int lastFrame,
      double timeInTicks);

  Quaternion boneRotation(
      BoneAnimationData &anim,
      BoneAnimationChannel &channel,
      double timeInTicks,
      unsigned int i);
  Vec3f bonePosition(
      BoneAnimationData &anim,
      BoneAnimationChannel &channel,
      double timeInTicks,
      unsigned int i);
  Vec3f boneScaling(
      BoneAnimationData &anim,
      BoneAnimationChannel &channel,
      double timeInTicks,
      unsigned int i);
};

#endif /* BONE_H_ */
