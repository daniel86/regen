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

Frustum::Frustum()
{
}
Frustum::~Frustum()
{
}

const Vec3f* Frustum::points() const
{
  return points_;
}

void Frustum::setProjection(
    float fov,
    float aspect,
    float near,
    float far)
{
  if(near>far) throw range_error("near>far");
  near_ = near;
  far_ = far;
  fov_ = fov;
  aspect_ = aspect;

  // +0.2 is important because we might get artifacts at
  // the screen borders.
  float fovR = fov/57.2957795 + 0.2;

  nearPlaneHeight_ = tan( fovR * 0.5) * near;
  nearPlaneWidth_  = nearPlaneHeight_ * aspect;

  farPlaneHeight_ = tan( fovR * 0.5) * far;
  farPlaneWidth_  = farPlaneHeight_ * aspect;
}

void Frustum::calculatePoints (
    const Vec3f &center,
    const Vec3f &viewDir,
    const Vec3f &up)
{
  Vec3f right = cross(viewDir, up);
  Vec3f fc = center + viewDir*far_;
  Vec3f nc = center + viewDir*near_;
  Vec3f rw, uh, u, buf1, buf2;

  normalize(right);
  // up vector must be orthogonal to right/view
  u = cross(right, viewDir);
  normalize(u);

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

vector<Frustum*> Frustum::split (
    unsigned int nFrustas,
    float splitWeight) const
{
  vector<Frustum*> frustas;
  float ratio = far_/near_;
  float si, lastn, currf, currn;

  frustas.push_back( new Frustum() );

  lastn = near_;
  for(unsigned int i=1; i<nFrustas; ++i)
  {
    si = i / (float)nFrustas;

    frustas.push_back( new Frustum() );
    // C_i = \lambda * C_i^{log} + (1-\lambda) * C_i^{uni}
    currn = splitWeight*(near_*( pow( ratio , si ))) +
        (1-splitWeight)*(near_ + (far_ - near_)*si);
    currf = currn * 1.005;

    frustas[i-1]->setProjection(fov_, aspect_, lastn, currf);

    lastn = currn;
  }
  frustas[nFrustas-1]->setProjection(fov_, aspect_, lastn, far_);

  return frustas;
}

