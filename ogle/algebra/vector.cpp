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

