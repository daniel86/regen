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

namespace regen {

#define __VEC2_FUNCTIONS(cls,type,zero) \
  cls() : x(zero), y(zero) {} \
  cls(type _x, type _y) : x(_x), y(_y) {} \
  cls(type _x) : x(_x), y(_x) {} \
  cls(const cls &b) : x(b.x), y(b.y) {} \
  inline void operator=(const cls &b) \
  { x=b.x; y=b.y; } \
  inline bool operator==(const cls &b) const \
  { return x==b.x && y==b.y; } \
  inline bool operator!=(const cls &b) const \
  { return !operator==(b); } \
  inline cls operator-() const \
  { return cls(-x,-y); } \
  inline cls operator+(const cls &b) const \
  { return cls(x+b.x, y+b.y ); } \
  inline cls operator-(const cls &b) const \
  { return cls(x-b.x, y-b.y ); } \
  inline cls operator*(const cls &b) const \
  { return cls(x*b.x, y*b.y ); } \
  inline cls operator/(const cls &b) const \
  { return cls(x/b.x, y/b.y ); } \
  inline cls operator*(const type &b) const \
  { return cls(x*b, y*b ); } \
  inline cls operator/(const type &b) const \
  { return cls(x/b, y/b ); } \
  inline void operator+=(const cls &b) \
  { x+=b.x; y+=b.y; } \
  inline void operator-=(const cls &b) \
  { x-=b.x; y-=b.y; } \
  inline void operator*=(const cls &b) \
  { x*=b.x; y*=b.y; } \
  inline void operator/=(const cls &b) \
  { x/=b.x; y/=b.y; } \
  inline void operator*=(const type &b) \
  { x*=b; y*=b; } \
  inline void operator/=(const type &b) \
  { x/=b; y/=b; }

#define __VEC3_FUNCTIONS(cls,type,zero) \
  cls() : x(zero), y(zero), z(zero) {} \
  cls(type _x, type _y, type _z) : x(_x), y(_y), z(_z) {} \
  cls(type _x) : x(_x), y(_x), z(_x) {} \
  cls(const cls &b) : x(b.x), y(b.y), z(b.z) {} \
  inline void operator=(const cls &b) \
  { x=b.x; y=b.y; z=b.z; } \
  inline bool operator==(const cls &b) const \
  { return x==b.x && y==b.y && z==b.z; } \
  inline bool operator!=(const cls &b) const \
  { return !operator==(b); } \
  inline cls operator-() const \
  { return cls(-x,-y,-z); } \
  inline cls operator+(const cls &b) const \
  { return cls(x+b.x, y+b.y, z+b.z ); } \
  inline cls operator-(const cls &b) const \
  { return cls(x-b.x, y-b.y, z-b.z ); } \
  inline cls operator*(const cls &b) const \
  { return cls(x*b.x, y*b.y, z*b.z ); } \
  inline cls operator/(const cls &b) const \
  { return cls(x/b.x, y/b.y, z/b.z ); } \
  inline cls operator*(const type &b) const \
  { return cls(x*b, y*b, z*b ); } \
  inline cls operator/(const type &b) const \
  { return cls(x/b, y/b, z/b ); } \
  inline void operator+=(const cls &b) \
  { x+=b.x; y+=b.y; z+=b.z; } \
  inline void operator-=(const cls &b) \
  { x-=b.x; y-=b.y; z-=b.z; } \
  inline void operator*=(const cls &b) \
  { x*=b.x; y*=b.y; z*=b.z; } \
  inline void operator/=(const cls &b) \
  { x/=b.x; y/=b.y; z/=b.z; } \
  inline void operator*=(const type &b) \
  { x*=b; y*=b; z*=b; } \
  inline void operator/=(const type &b) \
  { x/=b; y/=b; z/=b; }

#define __VEC4_FUNCTIONS(cls,type,zero) \
  cls() : x(zero), y(zero), z(zero), w(zero) {} \
  cls(type _x, type _y, type _z, type _w) : x(_x), y(_y), z(_z), w(_w) {} \
  cls(type _x) : x(_x), y(_x), z(_x), w(_x) {} \
  cls(const cls &b) : x(b.x), y(b.y), z(b.z), w(b.w) {} \
  inline void operator=(const cls &b) \
  { x=b.x; y=b.y; z=b.z; w=b.w; } \
  inline bool operator==(const cls &b) const \
  { return x==b.x && y==b.y && z==b.z && w==b.w; } \
  inline bool operator!=(const cls &b) const \
  { return !operator==(b); } \
  inline cls operator-() const \
  { return cls(-x,-y,-z,-w); } \
  inline cls operator+(const cls &b) const \
  { return cls(x+b.x, y+b.y, z+b.z, w+b.w ); } \
  inline cls operator-(const cls &b) const \
  { return cls(x-b.x, y-b.y, z-b.z, w-b.w ); } \
  inline cls operator*(const cls &b) const \
  { return cls(x*b.x, y*b.y, z*b.z, w*b.w ); } \
  inline cls operator/(const cls &b) const \
  { return cls(x/b.x, y/b.y, z/b.z, w/b.w ); } \
  inline cls operator*(const type &b) const \
  { return cls(x*b, y*b, z*b, w*b ); } \
  inline cls operator/(const type &b) const \
  { return cls(x/b, y/b, z/b, w/b ); } \
  inline void operator+=(const cls &b) \
  { x+=b.x; y+=b.y; z+=b.z; w+=b.w; } \
  inline void operator-=(const cls &b) \
  { x-=b.x; y-=b.y; z-=b.z; w-=b.w; } \
  inline void operator*=(const cls &b) \
  { x*=b.x; y*=b.y; z*=b.z; w*=b.w; } \
  inline void operator/=(const cls &b) \
  { x/=b.x; y/=b.y; z/=b.z; w/=b.w; } \
  inline void operator*=(const type &b) \
  { x*=b; y*=b; z*=b; w*=b; } \
  inline void operator/=(const type &b) \
  { x/=b; y/=b; z/=b; w/=b; }

/**
 * \brief A 2D vector of float values.
 */
struct Vec2f {
  GLfloat x; /**< the x component. **/
  GLfloat y; /**< the y component. **/
  /** Declares some default methods for 2D vectors. */
  __VEC2_FUNCTIONS(Vec2f,GLfloat,0.0f);

  /**
   * @return vector length.
   */
  inline GLfloat length() const
  {
    return sqrt(pow(x,2) + pow(y,2));
  }
  /**
   * Normalize this vector.
   */
  inline void normalize()
  {
    *this /= length();
  }
};

/**
 * \brief A 3D vector of float values.
 */
struct Vec3f {
  GLfloat x; /**< the x component. **/
  GLfloat y; /**< the y component. **/
  GLfloat z; /**< the z component. **/
  /** Declares some default methods for 3D vectors. */
  __VEC3_FUNCTIONS(Vec3f,GLfloat,0.0f);

  /**
   * @return vector length.
   */
  inline GLfloat length() const
  {
    return sqrt(pow(x,2) + pow(y,2) + pow(z,2));
  }
  /**
   * Normalize this vector.
   */
  inline void normalize()
  {
    *this /= length();
  }

  /**
   * Computes the cross product between two vectors.
   * The result vector is perpendicular to both of the vectors being multiplied.
   * @param b another vector.
   * @return the cross product.
   * @see http://en.wikipedia.org/wiki/Cross_product
   */
  inline Vec3f cross(const Vec3f &b) const
  {
    return Vec3f(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
  }
  /**
   * Computes the dot product between two vectors.
   * The dot product is equal to the acos of the angle
   * between those vectors.
   * @param b another vector.
   * @return the dot product.
   */
  inline GLfloat dot(const Vec3f &b) const
  {
    return x*b.x + y*b.y + z*b.z;
  }

  /**
   * Rotates this vector around x/y/z axis.
   * @param angle
   * @param x_
   * @param y_
   * @param z_
   */
  inline void rotate(GLfloat angle, GLfloat x_, GLfloat y_, GLfloat z_)
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

  /**
   * Compares vectors components.
   * @return true if all components are nearly equal.
   */
  inline GLboolean isApprox(const Vec3f &b, GLfloat delta=1e-6) const
  {
    return abs(x-b.x)<delta && abs(y-b.y)<delta && abs(z-b.z)<delta;
  }

  /**
   * @return static zero vector.
   */
  static const Vec3f& zero()
  {
    static Vec3f zero_(0.0f);
    return zero_;
  }
  /**
   * @return static one vector.
   */
  static const Vec3f& one()
  {
    static Vec3f one_(1.0f);
    return one_;
  }
  /**
   * @return static up vector.
   */
  static const Vec3f& up()
  {
    static Vec3f up_(0.0f,1.0f,0.0f);
    return up_;
  }
};

/**
 * \brief A 4D vector of float values.
 */
struct Vec4f {
  GLfloat x; /**< the x component. */
  GLfloat y; /**< the y component. */
  GLfloat z; /**< the z component. */
  GLfloat w; /**< the w component. */
  /** Declares some default methods for 4D vectors. */
  __VEC4_FUNCTIONS(Vec4f,GLfloat,0.0f);

  /**
   * @return Vec4f casted to Vec3f.
   */
  inline Vec3f& toVec3f()
  {
    return *((Vec3f*)this);
  }
  /**
   * Compares vectors components.
   * @return true if all components are nearly equal.
   */
  inline GLboolean isApprox(const Vec4f &b, GLfloat delta=1e-6) const
  {
    return abs(x-b.x)<delta && abs(y-b.y)<delta && abs(z-b.z)<delta && abs(w-b.w)<delta;
  }
};

/**
 * \brief An n-dimensional vector of float values.
 */
struct VecXf {
  GLfloat *v; /**< Components. **/
  GLuint size; /**< Number of components. **/

  VecXf()
  : v(NULL), size(0u) {}
  /**
   * Set-component constructor.
   */
  VecXf(GLfloat *_v, GLuint _size)
  : v(_v), size(_size) {}

  /**
   * Compares vectors components.
   * @return true if all components are nearly equal.
   */
  inline GLboolean isApprox(const VecXf &b, GLfloat delta=1e-6)
  {
    if(size == b.size) return GL_FALSE;
    for(GLuint i=0; i<size; ++i) {
      if(abs(v[i]-b.v[i]) > delta) return GL_FALSE;
    }
    return true;
  }
};

/**
 * \brief A 2D vector of double values.
 */
struct Vec2d {
  GLdouble x; /**< the x component. */
  GLdouble y; /**< the y component. */
  /** Declares some default methods for 2D vectors. */
  __VEC2_FUNCTIONS(Vec2d,GLdouble,0.0);
};

/**
 * \brief A 3D vector of double values.
 */
struct Vec3d {
  GLdouble x; /**< the x component. */
  GLdouble y; /**< the y component. */
  GLdouble z; /**< the z component. */
  /** Declares some default methods for 3D vectors. */
  __VEC3_FUNCTIONS(Vec3d,GLdouble,0.0);

  /**
   * @return vector length.
   */
  inline GLdouble length() const
  {
    return sqrt(pow(x,2) + pow(y,2) + pow(z,2));
  }
  /**
   * Normalize this vector.
   */
  inline void normalize()
  {
    *this /= length();
  }
};

/**
 * \brief A 4D vector of double values.
 */
struct Vec4d {
  GLdouble x; /**< the x component. */
  GLdouble y; /**< the y component. */
  GLdouble z; /**< the z component. */
  GLdouble w; /**< the w component. */
  /** Declares some default methods for 4D vectors. */
  __VEC4_FUNCTIONS(Vec4d,GLdouble,0.0f);
};

/**
 * \brief A 2D vector of int values.
 */
struct Vec2i {
  GLint x; /**< the x component. */
  GLint y; /**< the y component. */
  /** Declares some default methods for 2D vectors. */
  __VEC2_FUNCTIONS(Vec2i,GLint,0);
};

/**
 * \brief A 3D vector of int values.
 */
struct Vec3i {
  GLint x; /**< the x component. */
  GLint y; /**< the y component. */
  GLint z; /**< the z component. */
  /** Declares some default methods for 3D vectors. */
  __VEC3_FUNCTIONS(Vec3i,GLint,0.0);
};

/**
 * \brief A 4D vector of int values.
 */
struct Vec4i {
  GLint x; /**< the x component. */
  GLint y; /**< the y component. */
  GLint z; /**< the z component. */
  GLint w; /**< the w component. */
  /** Declares some default methods for 4D vectors. */
  __VEC4_FUNCTIONS(Vec4i,GLint,0.0f);
};

/**
 * \brief A 2D vector of unsigned int values.
 */
struct Vec2ui {
  GLuint x; /**< the x component. */
  GLuint y; /**< the y component. */
  /** Declares some default methods for 2D vectors. */
  __VEC2_FUNCTIONS(Vec2ui,GLuint,0u);
};

/**
 * \brief A 3D vector of unsigned int values.
 */
struct Vec3ui {
  GLuint x; /**< the x component. */
  GLuint y; /**< the y component. */
  GLuint z; /**< the z component. */
  /** Declares some default methods for 3D vectors. */
  __VEC3_FUNCTIONS(Vec3ui,GLuint,0.0);
};

/**
 * \brief A 4D vector of unsigned int values.
 */
struct Vec4ui {
  GLuint x; /**< the x component. */
  GLuint y; /**< the y component. */
  GLuint z; /**< the z component. */
  GLuint w; /**< the w component. */
  /** Declares some default methods for 4D vectors. */
  __VEC4_FUNCTIONS(Vec4ui,GLuint,0.0f);
};

/**
 * \brief A 2D vector of bool values.
 */
struct Vec2b {
  GLboolean x; /**< the x component. */
  GLboolean y; /**< the y component. */
  /** Declares some default methods for 2D vectors. */
  __VEC2_FUNCTIONS(Vec2b,GLboolean,GL_FALSE);
};

/**
 * \brief A 3D vector of bool values.
 */
struct Vec3b {
  GLboolean x; /**< the x component. */
  GLboolean y; /**< the y component. */
  GLboolean z; /**< the z component. */
  /** Declares some default methods for 3D vectors. */
  __VEC3_FUNCTIONS(Vec3b,GLboolean,0.0);
};

/**
 * \brief A 2D vector of bool values.
 */
struct Vec4b {
  GLboolean x; /**< the x component. */
  GLboolean y; /**< the y component. */
  GLboolean z; /**< the z component. */
  GLboolean w; /**< the w component. */
  /** Declares some default methods for 4D vectors. */
  __VEC4_FUNCTIONS(Vec4b,GLboolean,0.0f);
};

/**
 * \brief A n-dimensional vector of bool values.
 */
struct VecXb {
  GLboolean *v; /**< Components. **/
  GLuint size; /**< Number of components. **/

  VecXb()
  : v(NULL), size(0u) {}
  /**
   * Set-component constructor.
   */
  VecXb(GLboolean *_v, GLuint _size)
  : v(_v), size(_size) {}
};

// TODO: include in defines above

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

#define DEGREE_TO_RAD 57.29577951308232

GLdouble mix(GLdouble x, GLdouble y, GLdouble a);
GLfloat clamp(GLfloat x, GLfloat min, GLfloat max);

} // namespace

#endif /* ___VECTOR_H_ */
