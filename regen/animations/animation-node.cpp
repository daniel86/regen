/*
 * animation-node.cpp
 *
 *  Created on: 29.03.2012
 *      Author: daniel
 */

#include <regen/utility/logging.h>

#include "animation-node.h"
using namespace regen;

AnimationNode::AnimationNode(const string &name, const ref_ptr<AnimationNode> &parent)
: name_(name),
  parentNode_(parent),
  localTransform_(Mat4f::identity()),
  globalTransform_(Mat4f::identity()),
  offsetMatrix_(Mat4f::identity()),
  boneTransformationMatrix_(Mat4f::identity()),
  channelIndex_(-1),
  isBoneNode_(false)
{
}

const string& AnimationNode::name() const
{
  return name_;
}

const ref_ptr<AnimationNode>& AnimationNode::parent() const
{
  return parentNode_;
}

const Mat4f& AnimationNode::boneOffsetMatrix() const
{
  return offsetMatrix_;
}

void AnimationNode::set_boneOffsetMatrix(const Mat4f &offsetMatrix)
{
  offsetMatrix_ = offsetMatrix;
  isBoneNode_ = GL_TRUE;
}

const Mat4f& AnimationNode::boneTransformationMatrix() const
{
  return boneTransformationMatrix_;
}

void AnimationNode::addChild(const ref_ptr<AnimationNode> &child)
{
  nodeChilds_.push_back( child );
}
vector< ref_ptr<AnimationNode> >& AnimationNode::children()
{
  return nodeChilds_;
}

const Mat4f& AnimationNode::localTransform() const
{
  return localTransform_;
}
void AnimationNode::set_localTransform(const Mat4f &v)
{
  localTransform_ = v;
}

const Mat4f& AnimationNode::globalTransform() const
{
  return globalTransform_;
}
void AnimationNode::set_globalTransform(const Mat4f &v)
{
  globalTransform_ = v;
}

void AnimationNode::set_channelIndex(GLint channelIndex)
{
  channelIndex_ = channelIndex;
}

void AnimationNode::calculateGlobalTransform()
{
  // concatenate all parent transforms to get the global transform for this node
  globalTransform_ = localTransform_;
  for (AnimationNode* p=parentNode_.get(); p!=NULL; p=p->parentNode_.get())
  { globalTransform_.multiplyr(p->localTransform_); }
}

void AnimationNode::updateTransforms(const std::vector<Mat4f>& transforms)
{
  vector< ref_ptr<AnimationNode> >::iterator it;
  Stack<AnimationNode*> nodes;

  Mat4f *rootInverse = new Mat4f;
  if(channelIndex_!=-1) {
    globalTransform_ = transforms[channelIndex_];
  } else {
    globalTransform_ = localTransform_;
    // TODO might be identity!
    *rootInverse = globalTransform_.inverse();
  }
  if(isBoneNode_) {
    boneTransformationMatrix_ = offsetMatrix_;
  }
  for(it=nodeChilds_.begin(); it!=nodeChilds_.end(); ++it) nodes.push(it->get());

  while(!nodes.isEmpty())
  {
    AnimationNode *n = nodes.top();
    nodes.pop();

    // update node global transform
    n->globalTransform_ = (n->channelIndex_ != -1) ?
        transforms[n->channelIndex_] : n->localTransform_;
    n->globalTransform_.multiplyr(n->parentNode_->globalTransform_);

    if(n->isBoneNode_) {
      // Bone matrices transform from mesh coordinates in bind pose
      // to mesh coordinates in skinned pose
      n->boneTransformationMatrix_  = (*rootInverse) * n->globalTransform_;
      n->boneTransformationMatrix_ *= n->offsetMatrix_;
    }

    // continue for all children
    for (it=n->nodeChilds_.begin(); it!=n->nodeChilds_.end(); ++it) nodes.push(it->get());
  }
}

////////////////

static void loadNodeNames(AnimationNode *n, map<string,AnimationNode*> &nameToNode_)
{
  nameToNode_[n->name()] = n;
  for (vector< ref_ptr<AnimationNode> >::iterator it=n->children().begin();
      it!=n->children().end(); ++it)
  {
    loadNodeNames(it->get(), nameToNode_);
  }
}

// Look for present frame number.
// Search from last position if time is after the last time, else from beginning
template <class T>
static void findFrameAfterTick(
    GLdouble tick,
    GLuint &frame,
    const vector<T> &keys)
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
    T &dst,
    const GLuint &frame, const GLuint &lastFrame,
    const NodeAnimation::Channel &channel,
    const T &key, const T &first)
{
  if(frame >= lastFrame) {
    return false;
  }
  switch(channel.postState) {
  case NodeAnimation::BEHAVIOR_DEFAULT:
    dst.value = first.value;
    return true;
  case NodeAnimation::BEHAVIOR_CONSTANT:
    dst.value = key.value;
    return true;
  case NodeAnimation::BEHAVIOR_LINEAR:
  case NodeAnimation::BEHAVIOR_REPEAT:
  default:
    return false;
  }
}

//////

NodeAnimation::NodeAnimation(const ref_ptr<AnimationNode> &rootNode)
: Animation(GL_FALSE,GL_TRUE),
  rootNode_(rootNode),
  animationIndex_(-1),
  timeFactor_(1.0)
{
  loadNodeNames(rootNode_.get(), nameToNode_);
}

ref_ptr<AnimationNode> AnimationNode::copy()
{
  ref_ptr<AnimationNode> parent;
  ref_ptr<AnimationNode> ret = ref_ptr<AnimationNode>::manage(
      new AnimationNode(name_, parent));
  ret->localTransform_ = localTransform_;
  ret->globalTransform_ = globalTransform_;
  ret->offsetMatrix_ = offsetMatrix_;
  ret->boneTransformationMatrix_ = boneTransformationMatrix_;
  ret->channelIndex_ = channelIndex_;
  ret->isBoneNode_ = isBoneNode_;
  for(vector< ref_ptr<AnimationNode> >::iterator
      it=nodeChilds_.begin(); it!=nodeChilds_.end(); ++it)
  {
    ref_ptr<AnimationNode> child = (*it)->copy();
    child->parentNode_ = ret;
    ret->nodeChilds_.push_back(child);
  }
  return ret;
}
NodeAnimation* NodeAnimation::copy()
{
  ref_ptr<AnimationNode> rootNode = rootNode_->copy();
  NodeAnimation *ret = new NodeAnimation(rootNode);

  for(vector< ref_ptr<NodeAnimation::Data> >::iterator
      it=animData_.begin(); it!=animData_.end(); ++it)
  {
    ref_ptr<NodeAnimation::Data> &d = *it;
    ref_ptr<NodeAnimation::Data> data =
        ref_ptr<NodeAnimation::Data>::manage( new NodeAnimation::Data );
    data->animationName_ = d->animationName_;
    data->active_ = d->active_;
    data->elapsedTime_ = d->elapsedTime_;
    data->ticksPerSecond_ = d->ticksPerSecond_;
    data->lastTime_ = d->lastTime_;
    data->duration_ = d->duration_;
    data->channels_ = d->channels_;
    ret->animNameToIndex_[data->animationName_] = (GLint) ret->animData_.size();
    ret->animData_.push_back( data );
  }

  return ret;
}

void NodeAnimation::set_timeFactor(GLdouble timeFactor)
{
  timeFactor_ = timeFactor;
}
GLdouble NodeAnimation::timeFactor() const
{
  return timeFactor_;
}

GLint NodeAnimation::addChannels(
    const string &animationName,
    ref_ptr< vector<Channel> > &channels,
    GLdouble duration,
    GLdouble ticksPerSecond
    )
{
  ref_ptr<NodeAnimation::Data> data =
      ref_ptr<NodeAnimation::Data>::manage( new NodeAnimation::Data );
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

void NodeAnimation::setAnimationActive(
    const string &animationName, const Vec2d &forcedTickRange)
{
  setAnimationIndexActive( animNameToIndex_[animationName], forcedTickRange );
}
void NodeAnimation::setAnimationIndexActive(
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

  NodeAnimation::Data &anim = *animData_[animationIndex_].get();
  anim.active_ = true;
  if(anim.transforms_.size() != anim.channels_->size()) {
    anim.transforms_.resize( anim.channels_->size(), Mat4f::identity() );
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

void NodeAnimation::setTickRange(const Vec2d &forcedTickRange)
{
  if(animationIndex_ < 0) {
    WARN_LOG("can not set tick range without animation index set.");
    return;
  }
  NodeAnimation::Data &anim = *animData_[animationIndex_].get();

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
      Channel &channel = anim.channels_->data()[a];
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

void NodeAnimation::deallocateAnimationAtIndex(GLint animationIndex)
{
  if(animationIndex_ < 0) return;
  NodeAnimation::Data &anim = *animData_[ ANIM_INDEX(animationIndex) ].get();
  anim.active_ = false;
  anim.transforms_.resize( 0 );
  anim.startFramePosition_.resize( 0 );
  anim.lastFramePosition_.resize( 0 );
}

void NodeAnimation::stopNodeAnimation(NodeAnimation::Data &anim)
{
  GLuint currIndex = animationIndex_;
  animationIndex_ = -1;
  emitEvent(ANIMATION_STOPPED);

  anim.elapsedTime_ = 0.0;
  anim.lastTime_ = 0;

  if (animationIndex_ == (int)currIndex) {
    // repeat, signal handler set animationIndex_=currIndex again
  }
  else {
    anim.active_ = false;
    deallocateAnimationAtIndex(animationIndex_);
  }
}

void NodeAnimation::animate(GLdouble milliSeconds)
{
  if(animationIndex_ < 0) return;

  NodeAnimation::Data &anim = *animData_[animationIndex_].get();

  anim.elapsedTime_ += milliSeconds;
  if(duration_ <= 0.0) return;

  GLdouble time = anim.elapsedTime_*timeFactor_ / 1000.0;
  // map into anim's duration
  GLdouble timeInTicks = time*anim.ticksPerSecond_;
  if(timeInTicks > duration_) {
    // for repeating we could do...
    //timeInTicks = fmod(timeInTicks, duration_);
    stopNodeAnimation(anim);
    return;
  }
  timeInTicks += startTick_;

  // update transformations
  for (GLuint i = 0; i < anim.channels_->size(); i++)
  {
    const Channel &channel = anim.channels_->data()[i];
    Mat4f &m = anim.transforms_[i];

    if(channel.rotationKeys_->size() == 0) {
      m = Mat4f::identity();
    } else if(channel.rotationKeys_->size() == 1) {
      m = channel.rotationKeys_->data()[0].value.calculateMatrix();
    } else {
      m = nodeRotation(anim, channel, timeInTicks, i).calculateMatrix();
    }

    if(channel.scalingKeys_->size() == 0) {
    } else if(channel.scalingKeys_->size() == 1) {
      m.scale( channel.scalingKeys_->data()[0].value );
    } else {
      m.scale(nodeScaling(anim, channel,timeInTicks, i) );
    }

    if(channel.positionKeys_->size() == 0) {
    } else if(channel.positionKeys_->size() == 1) {
      Vec3f &pos = channel.positionKeys_->data()[0].value;
      m.x[3] = pos.x; m.x[7] = pos.y; m.x[11] = pos.z;
    } else {
      Vec3f pos = nodePosition(anim, channel, timeInTicks, i);
      m.x[3] = pos.x; m.x[7] = pos.y; m.x[11] = pos.z;
    }
  }

  anim.lastTime_ = timeInTicks;

  rootNode_->updateTransforms(anim.transforms_);
}

Vec3f NodeAnimation::nodePosition(
    NodeAnimation::Data &anim,
    const Channel &channel,
    GLdouble timeInTicks,
    GLuint i)
{
  GLuint keyCount = channel.positionKeys_->size();
  const vector<KeyFrame3f> &keys = *channel.positionKeys_.get();
  KeyFrame3f pos;

  pos.value = Vec3f(0);

  // Look for present frame number.
  GLuint lastFrame = anim.lastFramePosition_[i].x;
  GLuint frame = (timeInTicks >= anim.lastTime_ ?
      lastFrame : anim.startFramePosition_[i].x);
  findFrameAfterTick(timeInTicks, frame, keys);
  anim.lastFramePosition_[i].x = frame;
  // lookup nearest two keys
  const KeyFrame3f& key = keys[frame];
  // interpolate between this frame's value and next frame's value
  if( !handleFrameLoop( pos,
        frame, lastFrame, channel, key, keys[0]) )
  {
    const KeyFrame3f& nextKey = keys[ (frame + 1) % keyCount ];
    GLfloat fac = interpolationFactor(key, nextKey, timeInTicks, duration_);
    if (fac <= 0) return key.value;
    pos.value = key.value + (nextKey.value - key.value) * fac;
  }

  return pos.value;
}

Quaternion NodeAnimation::nodeRotation(
    NodeAnimation::Data &anim,
    const Channel &channel,
    GLdouble timeInTicks,
    GLuint i)
{
  KeyFrameQuaternion rot;
  GLuint keyCount = channel.rotationKeys_->size();
  const vector<KeyFrameQuaternion> &keys = *channel.rotationKeys_.get();

  rot.value = Quaternion(1, 0, 0, 0);

  // Look for present frame number.
  GLuint lastFrame = anim.lastFramePosition_[i].y;
  GLuint frame = (timeInTicks >= anim.lastTime_ ? lastFrame :
      anim.startFramePosition_[i].y);
  findFrameAfterTick(timeInTicks, frame, keys);
  anim.lastFramePosition_[i].y = frame;
  // lookup nearest two keys
  const KeyFrameQuaternion& key = keys[frame];
  // interpolate between this frame's value and next frame's value
  if( !handleFrameLoop( rot,
        frame, lastFrame, channel, key, keys[0]) )
  {
    const KeyFrameQuaternion& nextKey = keys[ (frame + 1) % keyCount ];
    float fac = interpolationFactor(key, nextKey, timeInTicks, duration_);
    if (fac <= 0) return key.value;
    rot.value.interpolate(key.value, nextKey.value, fac);
  }

  return rot.value;
}

Vec3f NodeAnimation::nodeScaling(
    NodeAnimation::Data &anim,
    const Channel &channel,
    GLdouble timeInTicks,
    GLuint i)
{
  const vector<KeyFrame3f> &keys = *channel.scalingKeys_.get();
  KeyFrame3f scale;

  // Look for present frame number.
  GLuint lastFrame = anim.lastFramePosition_[i].z;
  GLuint frame = (timeInTicks >= anim.lastTime_ ? lastFrame :
      anim.startFramePosition_[i].z);
  findFrameAfterTick(timeInTicks, frame, keys);
  anim.lastFramePosition_[i].z = frame;
  // lookup nearest key
  const KeyFrame3f& key = keys[frame];
  // set current value
  if( !handleFrameLoop( scale,
        frame, lastFrame, channel, key, keys[0]) )
  {
    scale.value = key.value;
  }

  return scale.value;
}

ref_ptr<AnimationNode> NodeAnimation::findNode(const string &name)
{
  return findNode(rootNode_, name);
}

ref_ptr<AnimationNode> NodeAnimation::findNode(ref_ptr<AnimationNode> &n, const string &name)
{
  if(n->name() == name) { return n; }

  for(vector< ref_ptr<AnimationNode> >::iterator
      it=n->children().begin(); it!=n->children().end(); ++it)
  {
    ref_ptr<AnimationNode> n_ = findNode(*it, name);
    if(n_.get()) { return n_; }
  }

  return ref_ptr<AnimationNode>();
}
