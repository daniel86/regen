/*
 * vector.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef ___VECTOR_H_
#define ___VECTOR_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <iostream>
#include <cmath>
#include <cassert>
#include <list>
using namespace std;

struct Vec2f {
  GLfloat x,y;
  Vec2f() {}
  Vec2f(GLfloat _x, GLfloat _y)
  : x(_x), y(_y) {}
  Vec2f(GLfloat _x)
  : x(_x), y(_x) {}
};
struct Vec3f {
  GLfloat x,y,z;
  Vec3f() {}
  Vec3f(GLfloat _x, GLfloat _y, GLfloat _z)
  : x(_x), y(_y), z(_z) {}
  Vec3f(GLfloat _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4f {
  GLfloat x,y,z,w;
  Vec4f() {}
  Vec4f(GLfloat _x, GLfloat _y, GLfloat _z, GLfloat _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4f(GLfloat _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};
struct VecXf {
  GLfloat *v;
  GLuint size;
  VecXf() {}
  VecXf(GLfloat *_v, GLuint _size)
  : v(_v), size(_size) {}
};

struct Vec2d {
  GLdouble x,y;
  Vec2d() {}
  Vec2d(GLdouble _x, GLdouble _y)
  : x(_x), y(_y) {}
  Vec2d(GLdouble _x)
  : x(_x), y(_x) {}
};
struct Vec3d {
  GLdouble x,y,z;
  Vec3d() {}
  Vec3d(GLdouble _x, GLdouble _y, GLdouble _z)
  : x(_x), y(_y), z(_z) {}
  Vec3d(GLdouble _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4d {
  GLdouble x,y,z,w;
  Vec4d() {}
  Vec4d(GLdouble _x, GLdouble _y, GLdouble _z, GLdouble _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4d(GLdouble _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

struct Vec2i {
  GLint x,y;
  Vec2i() {}
  Vec2i(GLint _x, GLint _y)
  : x(_x), y(_y) {}
  Vec2i(GLint _x)
  : x(_x), y(_x) {}
};
struct Vec3i {
  GLint x,y,z;
  Vec3i() {}
  Vec3i(GLint _x, GLint _y, GLint _z)
  : x(_x), y(_y), z(_z) {}
  Vec3i(GLint _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4i {
  GLint x,y,z,w;
  Vec4i() {}
  Vec4i(GLint _x, GLint _y, GLint _z, GLint _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4i(GLint _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

struct Vec2ui {
  GLuint x,y;
  Vec2ui() {}
  Vec2ui(GLuint _x, GLuint _y)
  : x(_x), y(_y) {}
  Vec2ui(GLuint _x)
  : x(_x), y(_x) {}
};
struct Vec3ui {
  GLuint x,y,z;
  Vec3ui() {}
  Vec3ui(GLuint _x, GLuint _y, GLuint _z)
  : x(_x), y(_y), z(_z) {}
  Vec3ui(GLuint _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4ui {
  GLuint x,y,z,w;
  Vec4ui() {}
  Vec4ui(GLuint _x, GLuint _y, GLuint _z, GLuint _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4ui(GLuint _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

inline ostream& operator<<(ostream& os, const Vec2i& v)
{
  return os << v.x << ", " << v.y;
}
inline ostream& operator<<(ostream& os, const Vec2f& v)
{
  return os << v.x << ", " << v.y;
}
inline ostream& operator<<(ostream& os, const Vec3i& v)
{
  return os << v.x << ", " << v.y << ", " << v.z;
}
inline ostream& operator<<(ostream& os, const Vec3f& v)
{
  return os << v.x << ", " << v.y << ", " << v.z;
}
inline ostream& operator<<(ostream& os, const Vec4f& v)
{
  return os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
}

inline Vec3f toStruct3f(const Vec4f &o)
{
  return Vec3f( o.x, o.y, o.z );
}

inline Vec3f operator-(const Vec3f &a)
{
  return Vec3f( -a.x, -a.y, -a.z );
}

inline Vec2d operator+(const Vec2d &a, const Vec2d &b)
{
  return Vec2d( a.x+b.x, a.y+b.y );
}
inline Vec3f operator+(const Vec3f &a, const Vec3f &b)
{
  return Vec3f( a.x+b.x, a.y+b.y, a.z+b.z );
}
inline Vec3f operator-(const Vec3f &a, const Vec3f &b)
{
  return Vec3f( a.x-b.x, a.y-b.y, a.z-b.z );
}
inline Vec3i operator-(const Vec3i &a, const Vec3i &b)
{
  return Vec3i( a.x-b.x, a.y-b.y, a.z-b.z );
}
inline void operator+=(Vec3f &a, const Vec3f &b)
{
  a.x+=b.x;
  a.y+=b.y;
  a.z+=b.z;
}
inline void operator-=(Vec3f &a, const Vec3f &b)
{
  a.x-=b.x;
  a.y-=b.y;
  a.z-=b.z;
}

inline Vec2f operator+(const Vec2f &a, const Vec2f &b)
{
  return Vec2f( a.x+b.x, a.y+b.y );
}
inline Vec2f operator-(const Vec2f &a, const Vec2f &b)
{
  return Vec2f( a.x-b.x, a.y-b.y );
}
inline Vec2i operator-(const Vec2i &a, const Vec2i &b)
{
  return Vec2i( a.x-b.x, a.y-b.y );
}
inline void operator+=(Vec2f &a, const Vec2f &b)
{
  a.x+=b.x;
  a.y+=b.y;
}
inline void operator+=(Vec2i &a, const Vec2i &b)
{
  a.x+=b.x;
  a.y+=b.y;
}
inline void operator-=(Vec2f &a, const Vec2f &b)
{
  a.x-=b.x;
  a.y-=b.y;
}

inline Vec3f operator*(const Vec3f &a, float scalar)
{
  return Vec3f( a.x*scalar, a.y*scalar, a.z*scalar );
}
inline Vec2f operator*(const Vec2f &a, float scalar)
{
  return Vec2f( a.x*scalar, a.y*scalar );
}

inline Vec3f operator*(const Vec3f &a, const Vec3f &b)
{
  return Vec3f( a.x*b.x, a.y*b.y, a.z*b.z );
}
inline Vec2f operator*(const Vec2f &a, const Vec2f &b)
{
  return Vec2f( a.x*b.x, a.y*b.y );
}

inline Vec3f operator/(const Vec3f &a, float scalar)
{
  return Vec3f( a.x/scalar, a.y/scalar, a.z/scalar );
}
inline void operator*=(Vec3f &a, float scalar)
{
  a.x*=scalar;
  a.y*=scalar;
  a.z*=scalar;
}
inline void operator/=(Vec3f &a, float scalar)
{
  a.x/=scalar;
  a.y/=scalar;
  a.z/=scalar;
}

inline Vec3f cross(const Vec3f &a, const Vec3f &b)
{
  return Vec3f(
      a.y*b.z - a.z*b.y,
      a.z*b.x - a.x*b.z,
      a.x*b.y - a.y*b.x
  );
}

inline float dot(const Vec3f &a, const Vec3f &b)
{
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline float length(const Vec3f &a)
{
  return sqrt( pow(a.x,2) + pow(a.y,2) + pow(a.z,2) );
}
inline float length(const Vec2f &a)
{
  return sqrt( pow(a.x,2) + pow(a.y,2) );
}

inline void normalize(Vec3f &a)
{
  a /= length(a);
}

inline bool isApprox(
    const float &a,
    const float &b,
    float delta=1e-6)
{
  return abs(a-b)<=delta;
}
inline bool isApprox(
    const Vec3f &a,
    const Vec3f &b,
    float delta=1e-6)
{
  return abs(a.x-b.x)<=delta &&
              abs(a.y-b.y)<=delta &&
              abs(a.z-b.z)<=delta;
}
inline bool isApprox(
    const Vec4f &a,
    const Vec4f &b,
    float delta=1e-6)
{
  return abs(a.x-b.x)<=delta &&
              abs(a.y-b.y)<=delta &&
              abs(a.z-b.z)<=delta &&
              abs(a.w-b.w)<=delta;
}
inline bool isApprox(
    const VecXf &a,
    const VecXf &b,
    float delta=1e-6)
{
  assert(a.size == b.size);
  for(unsigned int i=0; i<a.size; ++i) {
    if( abs(a.v[i]-b.v[i]) > delta ) return false;
  }
  return true;
}

inline void rotateView(Vec3f &view,
    float angle, float x, float y, float z)
{
  float c = cos(angle);
  float s = sin(angle);
  view = Vec3f(
          (x*x*(1-c) + c)   * view.x
        + (x*y*(1-c) - z*s) * view.y
        + (x*z*(1-c) + y*s) * view.z,
          (y*x*(1-c) + z*s) * view.x
        + (y*y*(1-c) + c)   * view.y
        + (y*z*(1-c) - x*s) * view.z,
          (x*z*(1-c) - y*s) * view.x
        + (y*z*(1-c) + x*s) * view.y
        + (z*z*(1-c) + c)   * view.z
  );
}

#define UP_DIMENSION_Y
extern const Vec3f UP_VECTOR;

#endif /* ___VECTOR_H_ */
