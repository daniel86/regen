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

/**
 * Morph vertex data.
 */
class VBOMorphAnimation : public VBOAnimation
{
public:
  /**
   * Emitted if target reached.
   */
  static GLuint MORPH_COMPLETED;

  /**
   * Signals indicating the current state of the morph animation.
   */
  enum MorphPhase { NO_TARGET, INIT, MORPH, CONTROL, COMPLETED };

  VBOMorphAnimation(AttributeState &p);

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
  /**
   * Calculates target data for a box morphing target.
   */
  void addBoxTarget(GLfloat width, GLfloat height, GLfloat depth);
  /**
   * Same as addSphereTarget but with an additional scale factor
   * for the xz-plane.
   * addSphereTarget(r) equals addEggTarget(r,r)
   */
  void addEggTarget(GLfloat radius, GLfloat eggRadius);

  /**
   * Sets the morpher implementation.
   */
  void set_morpher(ref_ptr<VBOMorpher> morpher);

  // override
  virtual GLboolean animateVBO(GLdouble dt);

protected:
  ref_ptr<GLfloat> createTargetData();

private:
  list< ref_ptr<GLfloat> > targets_;
  ref_ptr<VBOMorpher> morpher_;
  MorphPhase phase_;
};

///////////

/**
 * Base class for calculating the state of a morph animation.
 */
class VBOMorpher
{
public:
  VBOMorpher();

  void setAnimation(VBOMorphAnimation *animation);

  /**
   * Set the source data.
   */
  virtual void setSource(ref_ptr<GLfloat> source);

  /**
   * Initials a animation data.
   */
  virtual void initialMorph(ref_ptr<GLfloat> target, GLuint numVertices);

  /**
   * Make a step in the morphing animation.
   * Switches to CONTROL phase if true is returned.
   */
  virtual GLboolean morph(GLdouble dt) = 0;

  /**
   * Executed after morph phase.
   * Switches to COMPLETED phase if true is returned.
   */
  virtual GLboolean control(GLdouble dt);

  /**
   * Executed after control phase.
   */
  virtual void finalizeMorph();
protected:
  VBOMorphAnimation *animation_;
  ref_ptr<GLfloat> target_;
  ref_ptr<GLfloat> source_;
};

/**
 * Calculates vertex data using Hooke's law.
 */
class VBOElasticMorpher : public VBOMorpher
{
public:
  VBOElasticMorpher(GLboolean morphNormals=false);

  /**
   * Set physical parameters.
   */
  void setElasticParams(
      GLdouble springConstant=5.0,
      GLdouble vertexMass=0.0001,
      GLdouble friction=0.0001,
      GLdouble positionThreshold=0.001);

  // override
  virtual void initialMorph(ref_ptr<GLfloat> target, GLuint numVertices);
  virtual void setSource(ref_ptr<GLfloat> source);
  virtual GLboolean morph(GLdouble dt);

protected:
  vector<Vec3f> accelerations_;
  vector<GLdouble> distances_;
  GLdouble elasticFactor_;
  GLdouble friction_;
  GLdouble positionThreshold_;
  GLdouble accelerationThreshold_;
  GLboolean morphNormals_;

  void setDistances();
};

#endif /* VBO_MORPH_ANIMATION_H_ */
