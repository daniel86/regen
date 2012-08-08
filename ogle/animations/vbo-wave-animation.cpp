/*
 * vbo-wave-animation.cpp
 *
 *  Created on: 23.11.2011
 *      Author: daniel
 */

#include "vbo-wave-animation.h"

VBOWaveAnimation::VBOWaveAnimation(
    AttributeState &set,
    GLboolean animateNormal)
: VBOAnimation(set), animateNormal_(animateNormal)
{
}

void VBOWaveAnimation::set_data(void *data, GLuint offset)
{
  VBOAnimation::set_data(data, offset);
  makeSnapshot();
  const AttributeIteratorConst &vertsIt = attributeState_.vertices();
  vector< VecXf > vertsSnapshot = getFloatAttribute(vertsIt, snapshot_.get());
  for(list< ref_ptr<AnimationWave> >::iterator
      it=waves_.begin(); it!=waves_.end(); ++it)
  {
    (*it)->set_snapshot(vertsSnapshot);
  }
}

void VBOWaveAnimation::addWave(ref_ptr<AnimationWave> wave)
{
  waves_.push_back(wave);
  if(snapshot().get()) {
    const AttributeIteratorConst &vertsIt = attributeState_.vertices();
    vector< VecXf > vertsSnapshot = getFloatAttribute(vertsIt, snapshot_.get());
    wave->set_snapshot(vertsSnapshot);
  }
}
void VBOWaveAnimation::removeWave(AnimationWave *wave)
{
  for(list< ref_ptr<AnimationWave> >::iterator
      it=waves_.begin(); it!=waves_.end(); ++it)
  {
    if(it->get() == wave) {
      it = waves_.erase(it);
    } else {
      ++it;
    }
  }
}

GLboolean VBOWaveAnimation::animateVBO(GLdouble dt)
{
  // no animation without waves
  if(waves_.size()==0) { return false; }

  while(!try_lock()); {
    GLdouble dtSeconds = dt/1000.0;

    // Get primitive attribute iterators
    const AttributeIteratorConst &vertsIt = attributeState_.vertices();
    const AttributeIteratorConst &norsIt = attributeState_.normals();
    // Map the data pointer into Struct3f
    vector< VecXf > verts = getFloatAttribute(vertsIt);
    vector< VecXf > nors = getFloatAttribute(norsIt);
    vector< VecXf > vertsSnapshot = getFloatAttribute(vertsIt, snapshot_.get());
    vector< VecXf > norsSnapshot = getFloatAttribute(norsIt, snapshot_.get());

    for(GLuint i=0; i<verts.size(); ++i) {
      Vec3f &v = *(Vec3f*)verts[i].v;
      Vec3f &vSnapshot = *(Vec3f*)vertsSnapshot[i].v;

      Vec3f &n = *(Vec3f*)nors[i].v;
      Vec3f &nSnapshot = *(Vec3f*)norsSnapshot[i].v;
      Vec3f newNormal = nSnapshot;

      GLdouble height = 0.0;
      Vec3f displacement(0);

      for(list< ref_ptr<AnimationWave> >::iterator
          it=waves_.begin(); it!=waves_.end(); ++it)
      {
        ref_ptr<AnimationWave> &w = *it;

        // sum up heights
        displacement += w->calculateDisplacement(vSnapshot, nSnapshot);
        // sum up normals
        if(animateNormal_) newNormal += w->calculateNormal(vSnapshot, nSnapshot);
      }

      v = vSnapshot + displacement;
      if(animateNormal_) {
        normalize(newNormal);
        n = newNormal;
      }
    }

    for(list< ref_ptr<AnimationWave> >::iterator
        it=waves_.begin(); it!=waves_.end(); )
    {
      ref_ptr<AnimationWave> &w = *it;
      if(!w->timestep(dtSeconds)) {
        it = waves_.erase(it);
      } else {
        ++it;
      }
    }
  } unlock();

  return true;
}

//////////

AnimationWave::AnimationWave()
: EventObject(),
  amplitude_(1.0),
  width_(1.0),
  offset_(0.0),
  velocity_(4.0),
  lifetime_(-1.0),
  lastX_(0.0)
{
  set_width(width_);
}

void AnimationWave::set_amplitude(GLdouble amplitude) {
  amplitude_ = amplitude;
}
GLdouble AnimationWave::amplitude() const {
  return amplitude_;
}

void AnimationWave::set_width(GLdouble width) {
  width_ = width;
  waveFactor_ = M_PI/width_;
}
GLdouble AnimationWave::width() const {
  return width_;
}

void AnimationWave::set_normalInfluence(GLdouble normalInfluence) {
  normalInfluence_ = normalInfluence;
}
GLdouble AnimationWave::normalInfluence() const {
  return normalInfluence_;
}

void AnimationWave::set_velocity(GLdouble velocity) {
  velocity_ = velocity;
}
GLdouble AnimationWave::velocity() const {
  return velocity_;
}

void AnimationWave::set_lifetime(GLdouble lifetime) {
  lifetime_ = lifetime;
}
GLdouble AnimationWave::lifetime() const {
  return lifetime_;
}

GLboolean AnimationWave::timestep(GLdouble dt)
{
  offset_ += dt*velocity_;
  if(offset_ > 2.0*M_PI) offset_ -= 2.0*M_PI;

  if(lifetime_ >= 0.0) {
    lifetime_ -= dt;
    return lifetime_>0.0;
  } else {
    return true;
  }
}

float AnimationWave::calculateWaveHeight(const Vec3f &v, GLdouble wavePosition)
{
  lastX_ = waveFactor_*wavePosition + offset_;
  return amplitude_*sin(lastX_);
}
Vec3f AnimationWave::calculateWaveNormal(
    const Mat4f &rot,
    const Vec3f &displacementDirection,
    const Vec3f &waveDirection)
{
  Vec3f nor = displacementDirection  + waveDirection*cos(lastX_);
  nor += rotateVec3(rot, nor)*displacementDirection;
  normalize(nor);
  return nor*normalInfluence_;
}

DirectionalAnimationWave::DirectionalAnimationWave()
: AnimationWave(),
  direction_( (Vec3f) {1,1,1} ),
  displacementDirection_( (Vec3f) {0,0,0} ),
  displacementV_( (Vec3f) {0,0,0} )
{
  set_direction(direction_);
}
void DirectionalAnimationWave::updateDisplacementDirection()
{
  Vec3f s = displacementV_ - direction_*(
      dot(direction_,displacementV_)*dot(direction_,direction_) );
  displacementDirection_ = cross(s,direction_);
  normalize(displacementDirection_);
  // rotates from wave space to world space
  rot_ = xyzRotationMatrix(
      displacementDirection_.x,
      displacementDirection_.y,
      displacementDirection_.z);
}
void DirectionalAnimationWave::set_snapshot(vector<VecXf> &verts)
{
  displacementDirection_ = Vec3f(0);
  displacementV_ = Vec3f(0);

  // find a valid vertex to calculate the displacement vector.
  // invalid vertices are parallel to direction vector or zero vertices
  for(GLuint i=0; i<verts.size(); ++i) {
    Vec3f &v = *(Vec3f*)verts[i].v;
    if(length(v)<0.001f) continue; // skip zero vertex
    if(min(length(v-direction_), length(v+direction_)) < 0.001f)
    { // skip vertex parallel to direction
      continue;
    }
    displacementV_ = v;
    break;
  }

  updateDisplacementDirection();
}
Vec3f DirectionalAnimationWave::calculateDisplacement(const Vec3f &v, const Vec3f &n)
{
  Vec3f s = direction_*( dot(direction_,v)*dot(direction_,direction_) );
  return displacementDirection_*calculateWaveHeight(v, length(s));
}
Vec3f DirectionalAnimationWave::calculateNormal(const Vec3f &v, const Vec3f &n)
{
  return calculateWaveNormal(rot_, displacementDirection_, direction_);
}
void DirectionalAnimationWave::set_direction(const Vec3f &direction)
{
  direction_ = direction;
  normalize(direction_);
  if(length(displacementV_)>0.0f) updateDisplacementDirection();
}
const Vec3f& DirectionalAnimationWave::direction() const
{
  return direction_;
}

RadialAnimationWave::RadialAnimationWave()
: AnimationWave(),
  position_(0.0f)
{
}
Vec3f RadialAnimationWave::calculateDisplacement(const Vec3f &v, const Vec3f &n)
{
  float radialPosition = length(v - position_);
  return n*calculateWaveHeight(v, radialPosition);
}
Vec3f RadialAnimationWave::calculateNormal(const Vec3f &v, const Vec3f &n)
{
  Vec3f radialDirection = v - position_;
  normalize(radialDirection);
  Mat4f rot = xyzRotationMatrix(n.x, n.y, n.z);
  return calculateWaveNormal(rot, n, radialDirection);
}
void RadialAnimationWave::set_position(const Vec3f &position)
{
  position_ = position;
}
const Vec3f& RadialAnimationWave::position() const
{
  return position_;
}
