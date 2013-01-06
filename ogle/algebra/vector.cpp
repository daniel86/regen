/*
 * vector.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "vector.h"

const Vec3f UP_VECTOR = (Vec3f) { 0.0, 1.0, 0.0 };

#define READ_VEC(v) if(in.good()){\
    string val;\
    std::getline(in, val, ',');\
    boost::algorithm::trim(val);\
    stringstream ss(val);\
    ss >> v;\
}

istream& operator>>(istream& in, Vec2f &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  return in;
}
istream& operator>>(istream& in, Vec3f &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  READ_VEC(v.z);
  return in;
}
istream& operator>>(istream& in, Vec4f &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  READ_VEC(v.z);
  READ_VEC(v.w);
  return in;
}
istream& operator>>(istream& in, Vec2d &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  return in;
}
istream& operator>>(istream& in, Vec3d &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  READ_VEC(v.z);
  return in;
}
istream& operator>>(istream& in, Vec4d &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  READ_VEC(v.z);
  READ_VEC(v.w);
  return in;
}
istream& operator>>(istream& in, Vec2i &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  return in;
}
istream& operator>>(istream& in, Vec3i &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  READ_VEC(v.z);
  return in;
}
istream& operator>>(istream& in, Vec4i &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  READ_VEC(v.z);
  READ_VEC(v.w);
  return in;
}
istream& operator>>(istream& in, Vec2ui &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  return in;
}
istream& operator>>(istream& in, Vec3ui &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  READ_VEC(v.z);
  return in;
}
istream& operator>>(istream& in, Vec4ui &v)
{
  READ_VEC(v.x);
  READ_VEC(v.y);
  READ_VEC(v.z);
  READ_VEC(v.w);
  return in;
}

#undef READ_VEC

ostream& operator<<(ostream& os, const Vec2f& v)
{
  return os << v.x << "," << v.y;
}
ostream& operator<<(ostream& os, const Vec3f& v)
{
  return os << v.x << "," << v.y << "," << v.z;
}
ostream& operator<<(ostream& os, const Vec4f& v)
{
  return os << v.x << "," << v.y << "," << v.z << "," << v.w;
}
ostream& operator<<(ostream& os, const Vec2d& v)
{
  return os << v.x << "," << v.y;
}
ostream& operator<<(ostream& os, const Vec3d& v)
{
  return os << v.x << "," << v.y << "," << v.z;
}
ostream& operator<<(ostream& os, const Vec4d& v)
{
  return os << v.x << "," << v.y << "," << v.z << "," << v.w;
}
ostream& operator<<(ostream& os, const Vec2i& v)
{
  return os << v.x << "," << v.y;
}
ostream& operator<<(ostream& os, const Vec3i& v)
{
  return os << v.x << "," << v.y << "," << v.z;
}
ostream& operator<<(ostream& os, const Vec4i& v)
{
  return os << v.x << "," << v.y << "," << v.z << "," << v.w;
}
ostream& operator<<(ostream& os, const Vec2ui& v)
{
  return os << v.x << "," << v.y;
}
ostream& operator<<(ostream& os, const Vec3ui& v)
{
  return os << v.x << "," << v.y << "," << v.z;
}
ostream& operator<<(ostream& os, const Vec4ui& v)
{
  return os << v.x << "," << v.y << "," << v.z << "," << v.w;
}

Vec4f calculateTangent(Vec3f *vertices, Vec2f *texco, Vec3f &normal)
{
  Vec3f tangent, binormal;
  // calculate vertex and uv edges
  Vec3f edge1 = vertices[1] - vertices[0]; edge1.normalize();
  Vec3f edge2 = vertices[2] - vertices[0]; edge2.normalize();
  Vec2f texEdge1 = texco[1] - texco[0]; texEdge1.normalize();
  Vec2f texEdge2 = texco[2] - texco[0]; texEdge2.normalize();
  GLfloat det = texEdge1.x * texEdge2.y - texEdge2.x * texEdge1.y;

  if(isApprox(det,0.0)) {
    tangent  = Vec3f( 1.0, 0.0, 0.0 );
    binormal  = Vec3f( 0.0, 1.0, 0.0 );
  }
  else {
    det = 1.0 / det;
    tangent = Vec3f(
      (texEdge2.y * edge1.x - texEdge1.y * edge2.x),
      (texEdge2.y * edge1.y - texEdge1.y * edge2.y),
      (texEdge2.y * edge1.z - texEdge1.y * edge2.z)
    ) * det;
    binormal = Vec3f(
      (-texEdge2.x * edge1.x + texEdge1.x * edge2.x),
      (-texEdge2.x * edge1.y + texEdge1.x * edge2.y),
      (-texEdge2.x * edge1.z + texEdge1.x * edge2.z)
    ) * det;
  }

  // Gram-Schmidt orthogonalize tangent with normal.
  tangent -= normal * normal.dot(tangent);
  tangent.normalize();

  Vec3f bitangent = normal.cross(tangent);
  // Calculate the handedness of the local tangent space.
  GLfloat handedness = (bitangent.dot(binormal) < 0.0f) ? 1.0f : -1.0f;

  return Vec4f(tangent.x, tangent.y, tangent.z, handedness);
}
