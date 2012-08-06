/*
 * vbo-morph-animation.h
 *
 *  Created on: 23.11.2011
 *      Author: daniel
 */

#ifndef VBO_MORPH_ANIMATION_H_
#define VBO_MORPH_ANIMATION_H_

#include <ogle/animations/vbo-animation.h>

class VBOMorpher;

class VBOMorphAnimation : public VBOAnimation {
public:
  /**
   * Emitted if target reached.
   */
  static unsigned int MORPH_COMPLETED;

  enum MorphPhase { NO_TARGET, INIT, MORPH, CONTROL, COMPLETED };

  VBOMorphAnimation(GLuint vbo, AttributeState &p);

  /**
   * Removes targets from this morph animation.
   */
  void clearTargets();
  /**
   * Adds a morph target.
   */
  void addTarget(ref_ptr<GLfloat> targetData);
  /**
   * Morph to a previously created snapshot.
   * Note: create snapshots with this.makeSnapshot() after primitive set.
   */
  void addSnapshotTarget();
  /**
   * Calculates target data for a sphere morphing target.
   * This is quite simple: take the unmorphed and normalized vertex position as normal
   * of the sphere, scale the normal by radius and you get the morphed sphere coordinate.
   * Note: if no snapshot taken before, this function takes a snapshot
   */
  void addSphereTarget(GLfloat radius);
  /**
   * Calculates target data for a cube morphing target.
   * First vertices are mapped to a sphere (like addSphereTarget() does) that contains
   * the desired cube perfectly (radius=sqrt(size)).
   * Then the sphere vertices are mapped to the cube.
   * Note: if no snapshot taken before, this function takes a snapshot
   */
  void addCubeTarget(GLfloat size);
  void addBoxTarget(GLfloat width, GLfloat height, GLfloat depth);
  /**
   * Same as addSphereTarget but with an additional scale factor
   * for the xz-plane.
   * addSphereTarget(r) equals addEggTarget(r,r)
   */
  void addEggTarget(GLfloat radius, GLfloat eggRadius);

  void set_morpher(ref_ptr<VBOMorpher> morpher);

protected:
  ref_ptr<GLfloat> createTargetData();

  // override
  virtual void doAnimate(const double &dt);

private:
  list< ref_ptr<GLfloat> > targets_;
  ref_ptr<VBOMorpher> morpher_;
  MorphPhase phase_;
};

///////////

class VBOMorpher {
public:
  VBOMorpher();
  void setAnimation(VBOMorphAnimation *animation);
  virtual void initialMorph(ref_ptr<GLfloat> target, GLuint numVertices);
  virtual void setSource(ref_ptr<GLfloat> source);
  virtual void finalizeMorph();
  virtual bool control(float dt);
  virtual bool morph(float dt) = 0;
protected:
  VBOMorphAnimation *animation_;
  ref_ptr<GLfloat> target_;
  ref_ptr<GLfloat> source_;
};

class VBOElasticMorpher : public VBOMorpher {
public:
  VBOElasticMorpher(bool morphNormals=false);
  virtual void initialMorph(ref_ptr<GLfloat> target, GLuint numVertices);
  virtual void setSource(ref_ptr<GLfloat> source);
  void setElasticParams(
      float springConstant=5.0f,
      float vertexMass=0.0001f,
      float friction=0.0001f,
      float positionThreshold=0.001f);
  virtual bool morph(float dt);
protected:
  vector< Vec3f > accelerations_;
  vector< float > distances_;
  float elasticFactor_;
  float friction_;
  float positionThreshold_;
  float accelerationThreshold_;
  bool morphNormals_;

  void setDistances();
};

#endif /* VBO_MORPH_ANIMATION_H_ */
