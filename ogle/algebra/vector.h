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
  Vec2f()
  : x(0.0f), y(0.0f) {}
  Vec2f(GLfloat _x, GLfloat _y)
  : x(_x), y(_y) {}
  Vec2f(GLfloat _x)
  : x(_x), y(_x) {}
};
struct Vec3f {
  GLfloat x,y,z;
  Vec3f()
  : x(0.0f), y(0.0f), z(0.0f) {}
  Vec3f(GLfloat _x, GLfloat _y, GLfloat _z)
  : x(_x), y(_y), z(_z) {}
  Vec3f(GLfloat _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4f {
  GLfloat x,y,z,w;
  Vec4f()
  : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
  Vec4f(GLfloat _x, GLfloat _y, GLfloat _z, GLfloat _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4f(GLfloat _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};
struct VecXf {
  GLfloat *v;
  GLuint size;
  VecXf()
  : v(NULL), size(0u) {}
  VecXf(GLfloat *_v, GLuint _size)
  : v(_v), size(_size) {}
};

struct Vec2d {
  GLdouble x,y;
  Vec2d()
  : x(0.0), y(0.0) {}
  Vec2d(GLdouble _x, GLdouble _y)
  : x(_x), y(_y) {}
  Vec2d(GLdouble _x)
  : x(_x), y(_x) {}
};
struct Vec3d {
  GLdouble x,y,z;
  Vec3d()
  : x(0.0), y(0.0), z(0.0) {}
  Vec3d(GLdouble _x, GLdouble _y, GLdouble _z)
  : x(_x), y(_y), z(_z) {}
  Vec3d(GLdouble _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4d {
  GLdouble x,y,z,w;
  Vec4d()
  : x(0.0), y(0.0), z(0.0), w(0.0) {}
  Vec4d(GLdouble _x, GLdouble _y, GLdouble _z, GLdouble _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4d(GLdouble _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

struct Vec2i {
  GLint x,y;
  Vec2i()
  : x(0), y(0) {}
  Vec2i(GLint _x, GLint _y)
  : x(_x), y(_y) {}
  Vec2i(GLint _x)
  : x(_x), y(_x) {}
};
struct Vec3i {
  GLint x,y,z;
  Vec3i()
  : x(0), y(0), z(0) {}
  Vec3i(GLint _x, GLint _y, GLint _z)
  : x(_x), y(_y), z(_z) {}
  Vec3i(GLint _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4i {
  GLint x,y,z,w;
  Vec4i()
  : x(0), y(0), z(0), w(0) {}
  Vec4i(GLint _x, GLint _y, GLint _z, GLint _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4i(GLint _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

struct Vec2ui {
  GLuint x,y;
  Vec2ui()
  : x(0u), y(0u) {}
  Vec2ui(GLuint _x, GLuint _y)
  : x(_x), y(_y) {}
  Vec2ui(GLuint _x)
  : x(_x), y(_x) {}
};
struct Vec3ui {
  GLuint x,y,z;
  Vec3ui()
  : x(0u), y(0u), z(0u) {}
  Vec3ui(GLuint _x, GLuint _y, GLuint _z)
  : x(_x), y(_y), z(_z) {}
  Vec3ui(GLuint _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4ui {
  GLuint x,y,z,w;
  Vec4ui()
  : x(0u), y(0u), z(0u), w(0u) {}
  Vec4ui(GLuint _x, GLuint _y, GLuint _z, GLuint _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4ui(GLuint _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

struct Vec2b {
  GLboolean x,y;
  Vec2b()
  : x(GL_FALSE), y(GL_FALSE) {}
  Vec2b(GLboolean _x, GLboolean _y)
  : x(_x), y(_y) {}
  Vec2b(GLboolean _x)
  : x(_x), y(_x) {}
};
struct Vec3b {
  GLboolean x,y,z;
  Vec3b()
  : x(GL_FALSE), y(GL_FALSE), z(GL_FALSE) {}
  Vec3b(GLboolean _x, GLboolean _y, GLboolean _z)
  : x(_x), y(_y), z(_z) {}
  Vec3b(GLboolean _x)
  : x(_x), y(_x), z(_x) {}
};
struct Vec4b {
  GLboolean x,y,z,w;
  Vec4b()
  : x(GL_FALSE), y(GL_FALSE), z(GL_FALSE), w(GL_FALSE) {}
  Vec4b(GLboolean _x, GLboolean _y, GLboolean _z, GLboolean _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4b(GLboolean _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};
struct VecXb {
  GLboolean *v;
  GLuint size;
  VecXb()
  : v(NULL), size(0u) {}
  VecXb(GLboolean *_v, GLuint _size)
  : v(_v), size(_size) {}
};

ostream& operator<<(ostream& os, const Vec2f& v);
ostream& operator<<(ostream& os, const Vec3f& v);
ostream& operator<<(ostream& os, const Vec4f& v);
ostream& operator<<(ostream& os, const Vec2d& v);
ostream& operator<<(ostream& os, const Vec3d& v);
ostream& operator<<(ostream& os, const Vec4d& v);
ostream& operator<<(ostream& os, const Vec2i& v);
ostream& operator<<(ostream& os, const Vec3i& v);
ostream& operator<<(ostream& os, const Vec4i& v);
ostream& operator<<(ostream& os, const Vec2ui& v);
ostream& operator<<(ostream& os, const Vec3ui& v);
ostream& operator<<(ostream& os, const Vec4ui& v);

istream& operator>>(istream& in, Vec2f &v);
istream& operator>>(istream& in, Vec3f &v);
istream& operator>>(istream& in, Vec4f &v);
istream& operator>>(istream& in, Vec2d &v);
istream& operator>>(istream& in, Vec3d &v);
istream& operator>>(istream& in, Vec4d &v);
istream& operator>>(istream& in, Vec2i &v);
istream& operator>>(istream& in, Vec3i &v);
istream& operator>>(istream& in, Vec4i &v);
istream& operator>>(istream& in, Vec2ui &v);
istream& operator>>(istream& in, Vec3ui &v);
istream& operator>>(istream& in, Vec4ui &v);


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

inline Vec4f operator*(const Vec4f &a, float scalar)
{
  return Vec4f( a.x*scalar, a.y*scalar, a.z*scalar, a.w*scalar );
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
inline void operator/=(Vec2f &a, float scalar)
{
  a.x/=scalar;
  a.y/=scalar;
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
inline void normalize(Vec2f &a)
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

inline Vec4f calculateTangent(Vec3f *vertices, Vec2f *texco, Vec3f &normal)
{
  Vec3f tangent, binormal;
  // calculate vertex and uv edges
  Vec3f edge1 = vertices[1] - vertices[0]; normalize(edge1);
  Vec3f edge2 = vertices[2] - vertices[0]; normalize(edge2);
  Vec2f texEdge1 = texco[1] - texco[0]; normalize(texEdge1);
  Vec2f texEdge2 = texco[2] - texco[0]; normalize(texEdge2);
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
  tangent -= normal * dot(normal, tangent);
  normalize(tangent);

  Vec3f bitangent = cross(normal, tangent);
  // Calculate the handedness of the local tangent space.
  GLfloat handedness = (dot(bitangent, binormal) < 0.0f) ? 1.0f : -1.0f;

  return Vec4f(tangent.x, tangent.y, tangent.z, handedness);
}

#define UP_DIMENSION_Y
extern const Vec3f UP_VECTOR;

#endif /* ___VECTOR_H_ */
