/*
 * projection-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef PROJECTION_STATE_H_
#define PROJECTION_STATE_H_

#include <ogle/states/state.h>

class Projection : public State
{
public:
  Projection();
protected:
  ref_ptr<UniformMat4> matrixUniform_;
};

class PerspectiveProjection : public Projection
{
public:
  PerspectiveProjection();
  void setPerspective(
      GLfloat fov,
      GLfloat near,
      GLfloat far,
      GLfloat aspect);
protected:
  ref_ptr<UniformMat4> inverseMatrixUniform_;
  ref_ptr<UniformFloat> fovUniform_;
  ref_ptr<UniformFloat> nearUniform_;
  ref_ptr<UniformFloat> farUniform_;
  GLfloat aspect_;
};

class OrthogonalProjection : public Projection
{
public:
  OrthogonalProjection(GLfloat right, GLfloat top);
  void updateProjection(GLfloat right, GLfloat top);
};

Projection::Projection()
: State()
{
  matrixUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("projectionMatrix", 1, identity4f()));
  joinStates(matrixUniform_);
}

#endif /* PROJECTION_STATE_H_ */
