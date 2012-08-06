/*
 * vbo-morph-animation.cpp
 *
 *  Created on: 23.11.2011
 *      Author: daniel
 */

#include <string.h>

#include "vbo-morph-animation.h"

unsigned int VBOMorphAnimation::MORPH_COMPLETED = registerEvent("morphCompleted");

VBOMorphAnimation::VBOMorphAnimation(GLuint vbo, AttributeState &p)
: VBOAnimation(vbo,p),
  targets_(),
  phase_(NO_TARGET),
  morpher_()
{
}

void VBOMorphAnimation::clearTargets()
{
  targets_ = list< ref_ptr<GLfloat> >();
}

void VBOMorphAnimation::addTarget(ref_ptr<GLfloat> targetData)
{
  targets_.push_back(targetData);
  if(phase_ == NO_TARGET) {
    // make sure morph will happen
    phase_ = INIT;
  }
}
void VBOMorphAnimation::addSnapshotTarget()
{
  if(!snapshot().get()) makeSnapshot();
  addTarget( snapshot() );
}

ref_ptr<GLfloat> VBOMorphAnimation::createTargetData()
{
  if(!snapshot().get()) {
    makeSnapshot();
    if(morpher_.get()) morpher_->setSource( snapshot_ );
  }

  ref_ptr<GLfloat> targetData = ref_ptr< GLfloat >::manage(
      (GLfloat*) new GLubyte[snapshotSize()] );
  memcpy((void*)targetData.get(), (void*)snapshot().get(), snapshotSize());
  return targetData;
}

void VBOMorphAnimation::addEggTarget(GLfloat radius, GLfloat eggRadius)
{
  ref_ptr<GLfloat> targetData = createTargetData();

  // Get primitive attribute iterators
  const AttributeIteratorConst &vertsIt = mesh_.vertices();
  const AttributeIteratorConst &norsIt = mesh_.normals();
  // Map the data pointer into Struct3f
  vector< VecXf > verts = getFloatAttribute(vertsIt, targetData.get());
  vector< VecXf > nors = getFloatAttribute(norsIt, targetData.get());

  float radiusScale = eggRadius/radius;
  Vec3f eggScale = (Vec3f) {radiusScale, 1.0, radiusScale};

  // set sphere vertex data
  for(unsigned int i=0; i<verts.size(); ++i) {
    Vec3f &v = *(Vec3f*)verts[i].v;
    Vec3f &n = *(Vec3f*)nors[i].v;
    float l = length(v);
    if(l == 0) continue;

    // take normalized direction vector as normal
    n = v;
    normalize(n);
    // and scaled normal as sphere position
    // -l*1e-6 to avoid fighting
    v = n*eggScale*radius*(1.0f + l*1e-1);
  }

  addTarget(targetData);
}
void VBOMorphAnimation::addSphereTarget(GLfloat radius)
{
  addEggTarget(radius, radius);
}
void VBOMorphAnimation::addBoxTarget(GLfloat width, GLfloat height, GLfloat depth)
{
  ref_ptr<GLfloat> targetData = createTargetData();

  // Get primitive attribute iterators
  const AttributeIteratorConst &vertsIt = mesh_.vertices();
  const AttributeIteratorConst &norsIt = mesh_.normals();
  // Map the data pointer into Struct3f
  vector< VecXf > verts = getFloatAttribute(vertsIt, targetData.get());
  vector< VecXf > nors = getFloatAttribute(norsIt, targetData.get());
  Vec3f boxSize = (Vec3f) {width, height, depth};
  float radius = sqrt(0.5f);

  // set cube vertex data
  for(unsigned int i=0; i<verts.size(); ++i) {
    Vec3f &v = *(Vec3f*)verts[i].v;
    Vec3f &n = *(Vec3f*)nors[i].v;
    float l = length(v);
    if(l == 0) continue;

    // first map to sphere, a bit ugly but avoids intersection calculations
    // and scaled normal as sphere position
    Vec3f vCopy = v;
    normalize(vCopy);
    vCopy *= radius;

    // check the coordinate values to choose the right face
    float xAbs = abs(vCopy.x);
    float yAbs = abs(vCopy.y);
    float zAbs = abs(vCopy.z);
    float h, factor;
    // set the coordinate for the face to the cube size
    if(xAbs > yAbs && xAbs > zAbs) { // left/right face
      factor = (v.x<0 ? -1 : 1);
      n = ((Vec3f) {1,0,0})*factor;
      h = vCopy.x;
    } else if(yAbs > zAbs) { // top/bottom face
      factor = (v.y<0 ? -1 : 1);
      n = ((Vec3f) {0,1,0})*factor;
      h = vCopy.y;
    } else { //front/back face
      factor = (v.z<0 ? -1 : 1);
      n = ((Vec3f) {0,0,1})*factor;
      h = vCopy.z;
    }

    Vec3f r = vCopy - n*dot(n,vCopy)*2.0f;
    // reflect vector on cube face plane (-r*(factor*0.5f-h)/h) and
    // delete component of face direction (-n*0.5f , 0.5f because thats the sphere radius)
    vCopy += -r*(factor*0.5f-h)/h - n*0.5f;

    float maxDim = max(max(abs(vCopy.x),abs(vCopy.y)),abs(vCopy.z));
    // we divide by maxDim, so it is not allowed to be zero,
    // this happens for vCopy with only a single component not zero.
    if(maxDim!=0.0f) {
      // the distortion scale is calculated by dividing
      // the length of the vector pointing on the square surface
      // by the length of the vector pointing on the circle surface (equals circle radius).
      // size2/maxDim calculates scale factor for d to point on the square surface
      float distortionScale = ( length( vCopy * 0.5f/maxDim ) ) / 0.5f;
      vCopy *= distortionScale;
    }

    // -l*1e-6 to avoid fighting
    v = (vCopy+n*0.5f)*boxSize*(1.0f + l*1e-6);

    n = v;
    normalize(n);
  }

  addTarget(targetData);
}
void VBOMorphAnimation::addCubeTarget(GLfloat size)
{
  addBoxTarget(size,size,size);
}

void VBOMorphAnimation::set_morpher(ref_ptr<VBOMorpher> morpher)
{
  if(morpher_.get()) {
    morpher_->setAnimation(NULL);
    morpher_->setSource( ref_ptr<GLfloat>() );
  }
  morpher_ = morpher;
  morpher_->setAnimation(this);
  if(snapshot_.get()) morpher_->setSource( snapshot_ );
}

void VBOMorphAnimation::doAnimate(const double &dt)
{
  double dts = dt/1000.0;

  if(morpher_.get() == NULL) {
    // no morpher has been set yet.
    return;
  }

  switch(phase_) {
  case NO_TARGET:
    // nothing to do without a target
    break;
  case INIT: {
    // the current morph target
    ref_ptr<GLfloat> &currentTarget = targets_.front();
    morpher_->initialMorph(currentTarget, mesh_.numVertices());
    phase_ = MORPH;
    break;
  }
  case MORPH:
    if(morpher_->morph(dts)) {
      phase_ = CONTROL;
    }
    break;
  case CONTROL:
    if(morpher_->control(dts)) {
      phase_ = COMPLETED;
    }
    break;
  case COMPLETED:
    morpher_->setSource( targets_.front() );
    // pop first target
    targets_.erase( targets_.begin() );
    if(targets_.size() == 0) {
      // animation has nothing to do for now
      phase_ = NO_TARGET;
    } else {
      // initial next morph target
      phase_ = INIT;
    }
    morpher_->finalizeMorph();
    // let applications now the morph completed,
    // maybe someone wants to add a new morph target in event handlers
    queueEmit(MORPH_COMPLETED);
    break;
  }

  set_bufferChanged(true);
}

///////////

VBOMorpher::VBOMorpher()
: animation_(NULL),
  target_(),
  source_()
{
}
void VBOMorpher::setAnimation(VBOMorphAnimation *animation)
{
  animation_ = animation;
}
void VBOMorpher::initialMorph(ref_ptr<GLfloat> target, GLuint numVertices)
{
  target_ = target;
}
void VBOMorpher::setSource(ref_ptr<GLfloat> source)
{
  source_ = source;;
}
void VBOMorpher::finalizeMorph()
{
  target_ = ref_ptr< GLfloat >();
}
bool VBOMorpher::control(float dt)
{
  return true; // default implementation does nothing here
}

VBOElasticMorpher::VBOElasticMorpher(bool morphNormals)
: VBOMorpher(),
  morphNormals_(morphNormals)
{
  setElasticParams();
}
void VBOElasticMorpher::setElasticParams(
    float springConstant, float vertexMass,
    float friction,
    float positionThreshold)
{
  elasticFactor_ = springConstant/vertexMass;
  positionThreshold_ = positionThreshold;
  accelerationThreshold_ = elasticFactor_*positionThreshold_*2.0;
  friction_ = friction;
}
void VBOElasticMorpher::initialMorph(ref_ptr<GLfloat> target, GLuint numVertices)
{
  VBOMorpher::initialMorph(target, numVertices);
  accelerations_ = vector< Vec3f >(numVertices, (Vec3f) {0.0f,0.0f,0.0f});
  distances_ = vector< float >(numVertices);
  if(source_.get()) setDistances();
}
void VBOElasticMorpher::setSource(ref_ptr<GLfloat> source)
{
  VBOMorpher::setSource(source);
  if(target_.get()) setDistances();
}
void VBOElasticMorpher::setDistances()
{
  const AttributeIteratorConst &vertsIt = animation_->mesh().vertices();
  vector< VecXf > vertsTarget = animation_->getFloatAttribute(vertsIt, target_.get());
  vector< VecXf > vertsSource = animation_->getFloatAttribute(vertsIt, source_.get());
  for(unsigned int i=0; i<distances_.size(); ++i) {
    distances_[i] = length(
        *(Vec3f*)vertsTarget[i].v -
        *(Vec3f*)vertsSource[i].v
    );
  }
}
bool VBOElasticMorpher::morph(float dt)
{
  // Get primitive attribute iterators
  const AttributeIteratorConst &vertsIt = animation_->mesh().vertices();
  const AttributeIteratorConst &norsIt = animation_->mesh().normals();
  // Map the data pointer into Struct3f
  vector< VecXf > verts = animation_->getFloatAttribute(vertsIt);
  vector< VecXf > nors = animation_->getFloatAttribute(norsIt);
  vector< VecXf > vertsTarget = animation_->getFloatAttribute(vertsIt, target_.get());
  vector< VecXf > norsTarget = animation_->getFloatAttribute(norsIt, target_.get());
  vector< VecXf > norsSource = animation_->getFloatAttribute(norsIt, source_.get());
  bool thresholdReachedForAllVertices = true;

  for(unsigned int i=0; i<verts.size(); ++i) {
    Vec3f &v = *(Vec3f*)verts[i].v;
    Vec3f &vTarget = *(Vec3f*)vertsTarget[i].v;

    if(distances_[i]==0.0f) {
      v = vTarget;
      if(morphNormals_) {
        Vec3f &n = *(Vec3f*)nors[i].v;
        Vec3f &nTarget = *(Vec3f*)norsTarget[i].v;
        n = nTarget;
      }
      continue;
    }

    Vec3f direction = vTarget-v;
    Vec3f lastAcceleration = accelerations_[i];
    float aiAmount = length( accelerations_[i] );
    float remainingDistance = length(direction);

    // check if threshold reached
    if(remainingDistance < positionThreshold_ && aiAmount < accelerationThreshold_)
    {
      // a vertex completed if reached end position and has nearly no acceleration
      accelerations_[i] = Vec3f(
        accelerationThreshold_*0.01f,
        accelerationThreshold_*0.01f,
        accelerationThreshold_*0.01f);
      v = vTarget;
      if(morphNormals_) {
        Vec3f &n = *(Vec3f*)nors[i].v;
        Vec3f &nTarget = *(Vec3f*)norsTarget[i].v;
        n = nTarget;
      }
      continue;
    } else {
      thresholdReachedForAllVertices = false;
    }

    if(morphNormals_) {
      float remainingDistanceNormalized = remainingDistance/distances_[i];
      Vec3f &n = *(Vec3f*)nors[i].v;
      Vec3f &nTarget = *(Vec3f*)norsTarget[i].v;
      Vec3f &nSource = *(Vec3f*)norsSource[i].v;
      n = nSource*remainingDistanceNormalized + nTarget*(1.0f-remainingDistanceNormalized);
    }

    // calculate new acceleration
    accelerations_[i] += direction * elasticFactor_;
    accelerations_[i] -= accelerations_[i] * friction_ * dt;

    // difference in position
    Vec3f ds = accelerations_[i]*(dt*dt);

    // calculate new position
    if(direction.x/(direction.x-ds.x) < 0.0f ||
        direction.y/(direction.y-ds.y) < 0.0f ||
        direction.z/(direction.z-ds.z) < 0.0f)
    {
      accelerations_[i] = -accelerations_[i];
      v = vTarget;
    } else {
      v = v + ds;
    }
  }

  return thresholdReachedForAllVertices;
}
