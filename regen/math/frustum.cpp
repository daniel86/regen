/*
 * frustum.cpp
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#include "frustum.h"
using namespace regen;

void Frustum::set(GLfloat _aspect, GLfloat _fov, GLfloat _near, GLfloat _far)
{
  fov = _fov;
  aspect  = _aspect;
  near = _near;
  far  = _far;
  // +0.2 is important because we might get artifacts at
  // the screen borders.
  GLdouble fovR = _fov/57.2957795 + 0.2;
  nearPlane.y = tan( fovR * 0.5) * near;
  nearPlane.x = nearPlane.y * aspect;
  farPlane.y  = tan( fovR * 0.5) * far;
  farPlane.x  = farPlane.y * aspect;
}

void Frustum::update(const Vec3f &pos, const Vec3f &dir)
{
  Vec3f right = dir.cross( Vec3f::up() );
  Vec3f fc = pos + dir*far;
  Vec3f nc = pos + dir*near;
  Vec3f rw, uh, u, buf1, buf2;

  right.normalize();
  // up vector must be orthogonal to right/view
  u = right.cross(dir);
  u.normalize();

  rw = right*nearPlane.x;
  uh = u*nearPlane.y;
  buf1 = uh - rw;
  buf2 = uh + rw;
  points[0] = nc - buf1;
  points[1] = nc + buf1;
  points[2] = nc + buf2;
  points[3] = nc - buf2;

  rw = right*farPlane.x;
  uh = u*farPlane.y;
  buf1 = uh - rw;
  buf2 = uh + rw;
  points[4] = fc - buf1;
  points[5] = fc + buf1;
  points[6] = fc + buf2;
  points[7] = fc - buf2;
}

vector<Frustum*> Frustum::split(GLuint count, GLdouble splitWeight) const
{
  const GLfloat &n = near;
  const GLfloat &f = far;

  vector<Frustum*> frustas(count);
  GLdouble ratio = f/n;
  GLdouble si, lastn, currf, currn;

  lastn = n;
  for(GLuint i=1; i<count; ++i)
  {
    si = i / (GLdouble)count;

    // C_i = \lambda * C_i^{log} + (1-\lambda) * C_i^{uni}
    currn = splitWeight*(n*( pow( ratio , si ))) +
        (1-splitWeight)*(n + (f - n)*si);
    currf = currn * 1.005;

    frustas[i-1] = new Frustum;
    frustas[i-1]->set(fov, aspect, lastn, currf);

    lastn = currn;
  }
  frustas[count-1] = new Frustum;
  frustas[count-1]->set(fov, aspect, lastn, f);

  return frustas;
}


