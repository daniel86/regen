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

  inline Vec2f operator+(const Vec2f &b) const
  {
    return Vec2f(x+b.x, y+b.y );
  }
  inline Vec2f operator-(const Vec2f &b) const
  {
    return Vec2f(x-b.x, y-b.y );
  }
  inline Vec2f operator*(float scalar) const
  {
    return Vec2f(x*scalar, y*scalar);
  }
  inline Vec2f operator*(const Vec2f &b) const
  {
    return Vec2f(x*b.x, y*b.y);
  }

  inline void operator+=(const Vec2f &b)
  {
    x+=b.x; y+=b.y;
  }
  inline void operator-=(const Vec2f &b)
  {
    x-=b.x; y-=b.y;
  }
  inline void operator/=(float scalar)
  {
    x/=scalar; y/=scalar;
  }

  inline float length() const
  {
    return sqrt(pow(x,2) + pow(y,2));
  }
  inline void normalize()
  {
    *this /= length();
  }
};

struct Vec3f {
  GLfloat x,y,z;
  Vec3f()
  : x(0.0f), y(0.0f), z(0.0f) {}
  Vec3f(GLfloat _x, GLfloat _y, GLfloat _z)
  : x(_x), y(_y), z(_z) {}
  Vec3f(GLfloat _x)
  : x(_x), y(_x), z(_x) {}

  static const Vec3f& zero()
  {
    static Vec3f zero_(0.0f);
    return zero_;
  }
  static const Vec3f& one()
  {
    static Vec3f one_(1.0f);
    return one_;
  }

  inline Vec3f operator-() const
  {
    return Vec3f(-x,-y,-z);
  }
  inline Vec3f operator+(const Vec3f &b) const
  {
    return Vec3f(x+b.x, y+b.y, z+b.z);
  }
  inline Vec3f operator-(const Vec3f &b) const
  {
    return Vec3f(x-b.x, y-b.y, z-b.z);
  }
  inline Vec3f operator*(float scalar) const
  {
    return Vec3f(x*scalar, y*scalar, z*scalar);
  }
  inline Vec3f operator*(const Vec3f &b) const
  {
    return Vec3f(x*b.x, y*b.y, z*b.z);
  }
  inline Vec3f operator/(float scalar) const
  {
    return Vec3f(x/scalar, y/scalar, z/scalar);
  }

  inline void operator*=(float scalar)
  {
    x*=scalar; y*=scalar; z*=scalar;
  }
  inline void operator/=(float scalar)
  {
    x/=scalar; y/=scalar; z/=scalar;
  }
  inline void operator+=(const Vec3f &b)
  {
    x+=b.x; y+=b.y; z+=b.z;
  }
  inline void operator-=(const Vec3f &b)
  {
    x-=b.x; y-=b.y; z-=b.z;
  }

  inline Vec3f cross(const Vec3f &b) const
  {
    return Vec3f(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
  }
  inline float dot(const Vec3f &b) const
  {
    return x*b.x + y*b.y + z*b.z;
  }
  inline float length() const
  {
    return sqrt(pow(x,2) + pow(y,2) + pow(z,2));
  }
  inline bool isApprox(const Vec3f &b, float delta=1e-6) const
  {
    return abs(x-b.x)<=delta && abs(y-b.y)<=delta && abs(z-b.z)<=delta;
  }

  inline void normalize()
  {
    *this /= length();
  }
  inline void rotate(float angle, float x_, float y_, float z_)
  {
    float c = cos(angle);
    float s = sin(angle);
    Vec3f rotated(
            (x_*x_*(1-c) + c)    * x
          + (x_*y_*(1-c) - z_*s) * y
          + (x_*z_*(1-c) + y_*s) * z,
            (y_*x_*(1-c) + z_*s) * x
          + (y_*y_*(1-c) + c)    * y
          + (y_*z_*(1-c) - x_*s) * z,
            (x_*z_*(1-c) - y_*s) * x
          + (y_*z_*(1-c) + x_*s) * y
          + (z_*z_*(1-c) + c)    * z
    );
    x = rotated.x;
    y = rotated.y;
    z = rotated.z;
  }
};
inline Vec3f cross(const Vec3f &a, const Vec3f &b) { return a.cross(b); }
inline float dot(const Vec3f &a, const Vec3f &b) { return a.dot(b); }

struct Vec4f {
  GLfloat x,y,z,w;
  Vec4f()
  : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
  Vec4f(GLfloat _x, GLfloat _y, GLfloat _z, GLfloat _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  Vec4f(GLfloat _x)
  : x(_x), y(_x), z(_x), w(_x) {}

  inline Vec3f toStruct3f() const
  {
    return Vec3f(x,y,z);
  }
  inline Vec4f operator*(float scalar) const
  {
    return Vec4f(x*scalar, y*scalar, z*scalar, w*scalar);
  }
  inline bool isApprox(const Vec4f &b, float delta=1e-6) const
  {
    return abs(x-b.x)<=delta && abs(y-b.y)<=delta && abs(z-b.z)<=delta && abs(w-b.w)<=delta;
  }
};

struct VecXf {
  GLfloat *v;
  GLuint size;
  VecXf()
  : v(NULL), size(0u) {}
  VecXf(GLfloat *_v, GLuint _size)
  : v(_v), size(_size) {}

  inline bool isApprox(const VecXf &b, float delta=1e-6)
  {
    if(size == b.size) return false;
    for(unsigned int i=0; i<size; ++i) {
      if( abs(v[i]-b.v[i]) > delta ) return false;
    }
    return true;
  }
};

struct Vec2d {
  GLdouble x,y;
  Vec2d()
  : x(0.0), y(0.0) {}
  Vec2d(GLdouble _x, GLdouble _y)
  : x(_x), y(_y) {}
  Vec2d(GLdouble _x)
  : x(_x), y(_x) {}

  inline Vec2d operator+(const Vec2d &b)
  {
    return Vec2d(x+b.x, y+b.y);
  }
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

  inline Vec2i operator-(const Vec2i &b)
  {
    return Vec2i(x-b.x, y-b.y);
  }
  inline void operator+=(const Vec2i &b)
  {
    x+=b.x; y+=b.y;
  }
};

struct Vec3i {
  GLint x,y,z;
  Vec3i()
  : x(0), y(0), z(0) {}
  Vec3i(GLint _x, GLint _y, GLint _z)
  : x(_x), y(_y), z(_z) {}
  Vec3i(GLint _x)
  : x(_x), y(_x), z(_x) {}

  inline Vec3i operator-(const Vec3i &b)
  {
    return Vec3i(x-b.x, y-b.y, z-b.z);
  }
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

inline bool isApprox(const float &a, const float &b, float delta=1e-6)
{
  return abs(a-b)<=delta;
}

Vec4f calculateTangent(Vec3f *vertices, Vec2f *texco, Vec3f &normal);

extern const Vec3f UP_VECTOR;

#endif /* ___VECTOR_H_ */
