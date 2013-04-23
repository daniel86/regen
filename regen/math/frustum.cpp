/*
 * frustum.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <math.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

#include "frustum.h"
using namespace regen;

Frustum::Frustum()
{
  fov_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fov"));
  fov_->setUniformData(45.0);
  near_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("near"));
  near_->setUniformData(1.0f);
  far_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("far"));
  far_->setUniformData(200.0f);
  aspect_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aspect"));
  aspect_->setUniformData(8.0/6.0);
}

void Frustum::setProjection(GLdouble fov, GLdouble aspect, GLdouble near, GLdouble far)
{
  if(near>far) throw range_error("near>far");
  near_->setVertex1f(0,near);
  far_->setVertex1f(0,far);
  fov_->setVertex1f(0,fov);
  aspect_->setVertex1f(0,aspect);

  // +0.2 is important because we might get artifacts at
  // the screen borders.
  GLdouble fovR = fov/57.2957795 + 0.2;

  nearPlaneHeight_ = tan( fovR * 0.5) * near;
  nearPlaneWidth_  = nearPlaneHeight_ * aspect;

  farPlaneHeight_ = tan( fovR * 0.5) * far;
  farPlaneWidth_  = farPlaneHeight_ * aspect;
}

void Frustum::computePoints(const Vec3f &center, const Vec3f &viewDir)
{
  Vec3f right = viewDir.cross( Vec3f::up() );
  Vec3f fc = center + viewDir*far_->getVertex1f(0);
  Vec3f nc = center + viewDir*near_->getVertex1f(0);
  Vec3f rw, uh, u, buf1, buf2;

  right.normalize();
  // up vector must be orthogonal to right/view
  u = right.cross(viewDir);
  u.normalize();

  rw = right*nearPlaneWidth_;
  uh = u*nearPlaneHeight_;
  buf1 = uh - rw;
  buf2 = uh + rw;
  points_[0] = nc - buf1;
  points_[1] = nc + buf1;
  points_[2] = nc + buf2;
  points_[3] = nc - buf2;

  rw = right*farPlaneWidth_;
  uh = u*farPlaneHeight_;
  buf1 = uh - rw;
  buf2 = uh + rw;
  points_[4] = fc - buf1;
  points_[5] = fc + buf1;
  points_[6] = fc + buf2;
  points_[7] = fc - buf2;
}

vector<Frustum*> Frustum::split(GLuint nFrustas, GLdouble splitWeight) const
{
  const GLfloat &n = near_->getVertex1f(0);
  const GLfloat &f = far_->getVertex1f(0);

  vector<Frustum*> frustas(nFrustas);
  GLdouble ratio = f/n;
  GLdouble si, lastn, currf, currn;

  lastn = n;
  for(GLuint i=1; i<nFrustas; ++i)
  {
    si = i / (GLdouble)nFrustas;

    // C_i = \lambda * C_i^{log} + (1-\lambda) * C_i^{uni}
    currn = splitWeight*(n*( pow( ratio , si ))) +
        (1-splitWeight)*(n + (f - n)*si);
    currf = currn * 1.005;

    frustas[i-1] = new Frustum;
    frustas[i-1]->setProjection(fov_->getVertex1f(0), aspect_->getVertex1f(0), lastn, currf);

    lastn = currn;
  }
  frustas[nFrustas-1] = new Frustum;
  frustas[nFrustas-1]->setProjection(fov_->getVertex1f(0), aspect_->getVertex1f(0), lastn, f);

  return frustas;
}

const ref_ptr<ShaderInput1f>& Frustum::fov() const
{ return fov_; }
const ref_ptr<ShaderInput1f>& Frustum::near() const
{ return near_; }
const ref_ptr<ShaderInput1f>& Frustum::far() const
{ return far_; }
const ref_ptr<ShaderInput1f>& Frustum::aspect() const
{ return aspect_; }

const Vec3f* Frustum::points() const
{ return points_; }
