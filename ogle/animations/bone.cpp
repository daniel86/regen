/*
 * bone.cpp
 *
 *  Created on: 29.03.2012
 *      Author: daniel
 */

#include "bone.h"

struct BoneAnimationData {
  // string identifier for animation
  string animationName_;
  // flag indicating if this animation is active
  GLboolean active_;
  // milliseconds from start of animation
  GLdouble elapsedTime_;
  GLdouble ticksPerSecond_;
  GLdouble lastTime_;
  // Duration of the animation in ticks.
  GLdouble duration_;
  // local bone transformation
  vector<Mat4f> transforms_;
  // remember last frame for interpolation
  vector<Vec3ui> lastFramePosition_;
  vector<Vec3ui> startFramePosition_;
  ref_ptr< vector<BoneAnimationChannel> > channels_;
};

Bone::Bone(const string &name, ref_ptr<Bone> parent)
: parentBoneNode_(parent), name_(name), channelIndex_(-1)
{
}

const string& Bone::name() const
{
  return name_;
}

ref_ptr<Bone> Bone::parent()
{
  return parentBoneNode_;
}

const Mat4f& Bone::offsetMatrix() const
{
  return offsetMatrix_;
}

void Bone::set_offsetMatrix(const Mat4f &offsetMatrix)
{
  offsetMatrix_ = offsetMatrix;
}

const Mat4f& Bone::transformationMatrix() const
{
  return transformationMatrix_;
}

void Bone::addChild(ref_ptr<Bone> &child)
{
  boneNodeChilds_.push_back( child );
}
vector< ref_ptr<Bone> >& Bone::boneChilds()
{
  return boneNodeChilds_;
}

const Mat4f& Bone::localTransform()
{
  return localTransform_;
}
void Bone::set_localTransform(const Mat4f &v)
{
  localTransform_ = v;
}

const Mat4f& Bone::globalTransform()
{
  return globalTransform_;
}
void Bone::set_globalTransform(const Mat4f &v)
{
  globalTransform_ = v;
}

void Bone::set_channelIndex(GLint channelIndex)
{
  channelIndex_ = channelIndex;
}

void Bone::calculateGlobalTransform()
{
  // concatenate all parent transforms to get the global transform for this node
  set_globalTransform( localTransform() );
  ref_ptr<Bone> p = parent();
  while (p.get()) {
    set_globalTransform( p->localTransform() * globalTransform() );
    p = p->parent();
  }
}

void Bone::updateTransforms(const std::vector<Mat4f>& transforms)
{
  // update node local transform
  if (channelIndex_ != -1 && channelIndex_ < transforms.size()) {
    set_localTransform( transforms[channelIndex_] );
  }
  // update node global transform
  calculateGlobalTransform();

  // continue for all children
  for (vector< ref_ptr<Bone> >::iterator
      it=boneNodeChilds_.begin(); it!=boneNodeChilds_.end(); ++it)
  {
    it->get()->updateTransforms(transforms);
  }
}

void Bone::updateTransformationMatrix(const Mat4f &rootInverse)
{
  // Bone matrices transform from mesh coordinates in bind pose
  // to mesh coordinates in skinned pose
  // Therefore the formula is:
  //    offsetMatrix * boneTransform * inverseTransform
  transformationMatrix_ = rootInverse * globalTransform_ * offsetMatrix_;
  // continue for all children
  for (vector< ref_ptr<Bone> >::iterator
      it=boneNodeChilds_.begin(); it!=boneNodeChilds_.end(); ++it)
  {
    it->get()->updateTransformationMatrix(rootInverse);
  }
}

////////////////

static void loadBoneNames(Bone *n, map<string,Bone*> &nameToNode_)
{
  nameToNode_[n->name()] = n;
  for (vector< ref_ptr<Bone> >::iterator it=n->boneChilds().begin();
      it!=n->boneChilds().end(); ++it)
  {
    loadBoneNames(it->get(), nameToNode_);
  }
}

// Look for present frame number.
// Search from last position if time is after the last time, else from beginning
template <class T>
static void findFrameAfterTick(
    GLdouble tick,
    GLuint &frame,
    vector<T> &keys)
{
  while (frame < keys.size()-1) {
    if (tick-keys[++frame].time < 0.00001) {
      --frame;
      return;
    }
  }
}
template <class T>
static void findFrameBeforeTick(
    GLdouble &tick,
    GLuint &frame,
    vector<T> &keys)
{
  if(keys.size()==0) return;
  for (frame = keys.size()-1; frame > 0;) {
    if (tick-keys[--frame].time > 0.000001) return;
  }
}

template <class T>
static inline GLdouble interpolationFactor(
    const T &key, const T &nextKey,
    GLdouble t, GLdouble duration)
{
  GLdouble timeDifference = nextKey.time - key.time;
  if (timeDifference < 0.0) timeDifference += duration;
  return ((t - key.time) / timeDifference);
}

template <class T>
static inline GLboolean handleFrameLoop(
    T &dst, GLboolean &complete,
    const GLuint &frame, const GLuint &lastFrame,
    const BoneAnimationChannel &channel,
    const T &key, const T &first)
{
  if(frame >= lastFrame) {
    complete = false;
    return false;
  }
  switch(channel.postState) {
  case ANIM_BEHAVIOR_DEFAULT:
    dst.value = first.value;
    complete = true;
    return true;
  case ANIM_BEHAVIOR_CONSTANT:
    dst.value = key.value;
    complete = true;
    return true;
  case ANIM_BEHAVIOR_LINEAR:
    complete = true;
    return false;
  case ANIM_BEHAVIOR_REPEAT:
    complete = false;
    return false;
  }
}

//////

unsigned int BoneAnimation::ANIMATION_STOPPED =
    EventObject::registerEvent("animationStopped");

BoneAnimation::BoneAnimation(
    list<AttributeState*> &meshes,
    ref_ptr<Bone> rootBoneNode
    )
: Animation(),
  meshes_(meshes),
  rootBoneNode_(rootBoneNode),
  animationIndex_(-1),
  timeFactor_(1.0)
{
  loadBoneNames(rootBoneNode_.get(), nameToNode_);
}
BoneAnimation::~BoneAnimation()
{
}

void BoneAnimation::set_timeFactor(GLdouble timeFactor)
{
  timeFactor_ = timeFactor;
}
GLdouble BoneAnimation::timeFactor() const
{
  return timeFactor_;
}

GLint BoneAnimation::addChannels(
    const string &animationName,
    ref_ptr< vector< BoneAnimationChannel> > &channels,
    GLdouble duration,
    GLdouble ticksPerSecond
    )
{
  ref_ptr<BoneAnimationData> data =
      ref_ptr<BoneAnimationData>::manage( new BoneAnimationData );
  data->animationName_ = animationName;
  data->active_ = false;
  data->elapsedTime_ = 0.0;
  data->ticksPerSecond_ = ticksPerSecond;
  data->lastTime_ = 0.0;
  data->duration_ = duration;
  data->channels_ = channels;
  animNameToIndex_[data->animationName_] = (GLint) animData_.size();
  animData_.push_back( data );

  return animData_.size();
}

#define ANIM_INDEX(i) min(animationIndex, (GLint)animData_.size()-1)

void BoneAnimation::setAnimationActive(
    const string &animationName, const Vec2d &forcedTickRange)
{
  setAnimationIndexActive( animNameToIndex_[animationName], forcedTickRange );
}
void BoneAnimation::setAnimationIndexActive(
    GLint animationIndex, const Vec2d &forcedTickRange)
{
  if(animationIndex_ == animationIndex) {
    // already active
    setTickRange(forcedTickRange);
    return;
  }

  if(animationIndex_ > 0) {
    animData_[ animationIndex_ ]->active_ = false;
  }

  animationIndex_ = ANIM_INDEX(animationIndex);
  if(animationIndex_ < 0) return;

  BoneAnimationData &anim = *animData_[animationIndex_].get();
  anim.active_ = true;
  if(anim.transforms_.size() != anim.channels_->size()) {
    anim.transforms_.resize( anim.channels_->size(), identity4f() );
  }
  if(anim.startFramePosition_.size() != anim.channels_->size()) {
    anim.startFramePosition_.resize( anim.channels_->size() );
  }

  // set matching channel index
  for (GLuint a = 0; a < anim.channels_->size(); a++)
  {
    const string nodeName = anim.channels_->data()[a].nodeName_;
    nameToNode_[ nodeName ]->set_channelIndex(a);
  }

  tickRange_.x = -1.0;
  tickRange_.y = -1.0;
  setTickRange( forcedTickRange );
}

void BoneAnimation::setTickRange(const Vec2d &forcedTickRange)
{
  if(animationIndex_ < 0) {
    WARN_LOG("can not set tick range without animation index set.");
    return;
  }
  BoneAnimationData &anim = *animData_[animationIndex_].get();

  // get first and last tick of animation
  Vec2d tickRange;
  if( forcedTickRange.x < 0.0f || forcedTickRange.y < 0.0f ) {
    tickRange.x = 0.0;
    tickRange.y = anim.duration_;
  } else {
    tickRange = forcedTickRange;
  }

  if(tickRange.x == tickRange_.x &&
      tickRange.y == tickRange_.y)
  {
    // nothing changed
    return;
  }
  tickRange_ = tickRange;

  // set start frames
  if(tickRange_.x < 0.00001) {
    anim.startFramePosition_.assign(
        anim.channels_->size(), Vec3ui(0u) );
  } else {
    for (GLuint a = 0; a < anim.channels_->size(); a++)
    {
      BoneAnimationChannel &channel = anim.channels_->data()[a];
      Vec3ui framePos(0u);
      findFrameBeforeTick(tickRange_.x, framePos.x, *channel.positionKeys_.get());
      findFrameBeforeTick(tickRange_.x, framePos.y, *channel.rotationKeys_.get());
      findFrameBeforeTick(tickRange_.x, framePos.z, *channel.scalingKeys_.get());
      anim.startFramePosition_[a] = framePos;
    }
  }

  // initial last frame to start frame
  anim.lastFramePosition_ = anim.startFramePosition_;

  // set to start pos of the new tick range
  anim.lastTime_ = 0.0;
  anim.elapsedTime_ = 0.0;

  // set start tick and duration in ticks
  startTick_ = tickRange_.x;
  duration_ = tickRange_.y - tickRange_.x;
}

void BoneAnimation::deallocateAnimationAtIndex(GLint animationIndex)
{
  if(animationIndex_ < 0) return;
  BoneAnimationData &anim = *animData_[ ANIM_INDEX(animationIndex) ].get();
  anim.active_ = false;
  anim.transforms_.resize( 0 );
  anim.startFramePosition_.resize( 0 );
  anim.lastFramePosition_.resize( 0 );
}

void BoneAnimation::animate(GLdouble milliSeconds)
{
  if(animationIndex_ < 0) return;

  BoneAnimationData &anim = *animData_[animationIndex_].get();

  anim.elapsedTime_ += milliSeconds;
  if(duration_ <= 0.0) return;

  GLdouble time = anim.elapsedTime_*timeFactor_ / 1000.0;
  // map into anim's duration
  GLdouble timeInTicks = fmod(time * anim.ticksPerSecond_, duration_)+startTick_;

  GLboolean isAnimActive = false;

  // update transformations
  for (GLuint i = 0; i < anim.channels_->size(); i++)
  {
    BoneAnimationChannel &channel = anim.channels_->data()[i];
    Mat4f &m = anim.transforms_[i];

    if( !channel.isPositionCompleted ||
        !channel.isRotationCompleted ||
        !channel.isScalingCompleted )
    {
      isAnimActive = true;
    }

    if(channel.rotationKeys_->size() == 0) {
      m = identity4f();
      channel.isRotationCompleted = true;
    } else if(channel.rotationKeys_->size() == 1) {
      m = channel.rotationKeys_->data()[0].value.calculateMatrix();
      channel.isRotationCompleted = true;
    } else {
      m = boneRotation(anim, channel, timeInTicks, i).calculateMatrix();
    }

    if(channel.scalingKeys_->size() == 0) {
      channel.isScalingCompleted = true;
    } else if(channel.scalingKeys_->size() == 1) {
      scaleMat( m, channel.scalingKeys_->data()[0].value );
      channel.isScalingCompleted = true;
    } else {
      scaleMat( m, boneScaling(anim, channel,timeInTicks, i) );
    }

    if(channel.positionKeys_->size() == 0) {
      channel.isPositionCompleted = true;
    } else if(channel.positionKeys_->size() == 1) {
      Vec3f pos = channel.positionKeys_->data()[0].value;
      m.x[3] = pos.x; m.x[7] = pos.y; m.x[11] = pos.z;
      channel.isPositionCompleted = true;
    } else {
      Vec3f pos = bonePosition(anim, channel, timeInTicks, i);
      m.x[3] = pos.x; m.x[7] = pos.y; m.x[11] = pos.z;
    }
  }

  if(!isAnimActive) {
    GLuint currIndex = animationIndex_;
    animationIndex_ = -1;
    emit(ANIMATION_STOPPED);

    if (animationIndex_ == currIndex) {
      // repeat
      anim.elapsedTime_ = 0.0;
    } else {
      for (GLuint i = 0; i < anim.channels_->size(); i++)
      {
        BoneAnimationChannel &channel = anim.channels_->data()[i];
        channel.isPositionCompleted = (channel.positionKeys_->size()<2);
        channel.isRotationCompleted = (channel.rotationKeys_->size()<2);
        channel.isScalingCompleted = (channel.scalingKeys_->size()<2);
      }
      anim.active_ = false;
      anim.lastTime_ = 0;
      anim.elapsedTime_ = 0.0;
      deallocateAnimationAtIndex(animationIndex_);
      return;
    }
  }

  anim.lastTime_ = timeInTicks;

  rootBoneNode_->updateTransforms(anim.transforms_);
  // calculate the inverse global transform
  Mat4f rootBoneInverse_ = inverse(rootBoneNode_->globalTransform());
  rootBoneNode_->updateTransformationMatrix(rootBoneInverse_);
}

Vec3f BoneAnimation::bonePosition(
    BoneAnimationData &anim,
    BoneAnimationChannel &channel,
    GLdouble timeInTicks,
    GLuint i)
{
  GLuint keyCount = channel.positionKeys_->size();
  vector< BonePositionKey > &keys = *channel.positionKeys_.get();
  BonePositionKey pos;

  pos.value = Vec3f(0);

  // Look for present frame number.
  GLuint lastFrame = anim.lastFramePosition_[i].x;
  GLuint frame = (timeInTicks >= anim.lastTime_ ?
      lastFrame : anim.startFramePosition_[i].x);
  findFrameAfterTick(timeInTicks, frame, keys);
  anim.lastFramePosition_[i].x = frame;
  // lookup nearest two keys
  const BonePositionKey& key = keys[frame];
  // interpolate between this frame's value and next frame's value
  if( !handleFrameLoop( pos, channel.isPositionCompleted,
        frame, lastFrame, channel, key, keys[0]) )
  {
    const BonePositionKey& nextKey = keys[ (frame + 1) % keyCount ];
    GLfloat fac = interpolationFactor(key, nextKey, timeInTicks, duration_);
    if (fac <= 0) return key.value;
    pos.value = key.value + (nextKey.value - key.value) * fac;
  }

  return pos.value;
}

Quaternion BoneAnimation::boneRotation(
    BoneAnimationData &anim,
    BoneAnimationChannel &channel,
    GLdouble timeInTicks,
    GLuint i)
{
  BoneQuaternionKey rot;
  GLuint keyCount = channel.rotationKeys_->size();
  vector< BoneQuaternionKey > &keys = *channel.rotationKeys_.get();

  rot.value = Quaternion(1, 0, 0, 0);

  // Look for present frame number.
  GLuint lastFrame = anim.lastFramePosition_[i].y;
  GLuint frame = (timeInTicks >= anim.lastTime_ ? lastFrame :
      anim.startFramePosition_[i].y);
  findFrameAfterTick(timeInTicks, frame, keys);
  anim.lastFramePosition_[i].y = frame;
  // lookup nearest two keys
  const BoneQuaternionKey& key = keys[frame];
  // interpolate between this frame's value and next frame's value
  if( !handleFrameLoop( rot, channel.isRotationCompleted,
        frame, lastFrame, channel, key, keys[0]) )
  {
    const BoneQuaternionKey& nextKey = keys[ (frame + 1) % keyCount ];
    float fac = interpolationFactor(key, nextKey, timeInTicks, duration_);
    if (fac <= 0) return key.value;
    rot.value.interpolate(key.value, nextKey.value, fac);
  }

  return rot.value;
}

Vec3f BoneAnimation::boneScaling(
    BoneAnimationData &anim,
    BoneAnimationChannel &channel,
    GLdouble timeInTicks,
    GLuint i)
{
  GLuint keyCount = channel.scalingKeys_->size();
  vector< BoneScalingKey > &keys = *channel.scalingKeys_.get();
  BoneScalingKey scale;

  // Look for present frame number.
  GLuint lastFrame = anim.lastFramePosition_[i].z;
  GLuint frame = (timeInTicks >= anim.lastTime_ ? lastFrame :
      anim.startFramePosition_[i].z);
  findFrameAfterTick(timeInTicks, frame, keys);
  anim.lastFramePosition_[i].z = frame;
  // lookup nearest key
  const BoneScalingKey& key = keys[frame];
  // set current value
  if( !handleFrameLoop( scale, channel.isScalingCompleted,
        frame, lastFrame, channel, key, keys[0]) )
  {
    scale.value = key.value;
  }

  return scale.value;
}
