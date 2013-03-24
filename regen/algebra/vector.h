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
/**
 * \brief A 2D vector of float values.
 */
struct Vec2f {
  GLfloat x; /**< the x component. **/
  GLfloat y; /**< the y component. **/

  Vec2f()
  : x(0.0f), y(0.0f) {}
  /**
   * Set-component constructor.
   */
  Vec2f(GLfloat _x, GLfloat _y)
  : x(_x), y(_y) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec2f(GLfloat _x)
  : x(_x), y(_x) {}

  /**
   * @param b vector to add.
   * @return the vector sum.
   */
  inline Vec2f operator+(const Vec2f &b) const
  {
    return Vec2f(x+b.x, y+b.y );
  }
  /**
   * @param b vector to subtract.
   * @return the vector difference.
   */
  inline Vec2f operator-(const Vec2f &b) const
  {
    return Vec2f(x-b.x, y-b.y );
  }
  /**
   * @param scalar scalar to multiply.
   * @return the vector product.
   */
  inline Vec2f operator*(GLfloat scalar) const
  {
    return Vec2f(x*scalar, y*scalar);
  }
  /**
   * @param b scalar to multiply.
   * @return product of scalar and vector.
   */
  inline Vec2f operator*(const Vec2f &b) const
  {
    return Vec2f(x*b.x, y*b.y);
  }
  /**
   * @param b vector to add.
   */
  inline void operator+=(const Vec2f &b)
  {
    x+=b.x; y+=b.y;
  }
  /**
   * @param b vector to subtract.
   */
  inline void operator-=(const Vec2f &b)
  {
    x-=b.x; y-=b.y;
  }
  /**
   * @param scalar scalar to divide.
   */
  inline void operator/=(float scalar)
  {
    x/=scalar; y/=scalar;
  }

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

  Vec3f()
  : x(0.0f), y(0.0f), z(0.0f) {}
  /**
   * Set-component constructor.
   */
  Vec3f(GLfloat _x, GLfloat _y, GLfloat _z)
  : x(_x), y(_y), z(_z) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec3f(GLfloat _x)
  : x(_x), y(_x), z(_x) {}

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

  /**
   * @return vector with each component negated.
   */
  inline Vec3f operator-() const
  {
    return Vec3f(-x,-y,-z);
  }
  /**
   * @param b vector to add.
   * @return the vector sum.
   */
  inline Vec3f operator+(const Vec3f &b) const
  {
    return Vec3f(x+b.x, y+b.y, z+b.z);
  }
  /**
   * @param b vector to subtract.
   * @return the vector difference.
   */
  inline Vec3f operator-(const Vec3f &b) const
  {
    return Vec3f(x-b.x, y-b.y, z-b.z);
  }
  /**
   * @param scalar scalar to multiply.
   * @return product of scalar and vector.
   */
  inline Vec3f operator*(float scalar) const
  {
    return Vec3f(x*scalar, y*scalar, z*scalar);
  }
  /**
   * @param b vector to multiply.
   * @return the vector product.
   */
  inline Vec3f operator*(const Vec3f &b) const
  {
    return Vec3f(x*b.x, y*b.y, z*b.z);
  }
  /**
   * @param scalar scalar to divide.
   * @return vector divided by scalar.
   */
  inline Vec3f operator/(float scalar) const
  {
    return Vec3f(x/scalar, y/scalar, z/scalar);
  }

  /**
   * @param scalar scalar to multiply.
   */
  inline void operator*=(float scalar)
  {
    x*=scalar; y*=scalar; z*=scalar;
  }
  /**
   * @param scalar scalar to divide.
   */
  inline void operator/=(float scalar)
  {
    x/=scalar; y/=scalar; z/=scalar;
  }
  /**
   * @param b vector to add.
   */
  inline void operator+=(const Vec3f &b)
  {
    x+=b.x; y+=b.y; z+=b.z;
  }
  /**
   * @param b vector to subtract.
   */
  inline void operator-=(const Vec3f &b)
  {
    x-=b.x; y-=b.y; z-=b.z;
  }

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
};

/**
 * \brief A 4D vector of float values.
 */
struct Vec4f {
  GLfloat x; /**< the x component. */
  GLfloat y; /**< the y component. */
  GLfloat z; /**< the z component. */
  GLfloat w; /**< the w component. */

  Vec4f()
  : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
  /**
   * Set-component constructor.
   */
  Vec4f(GLfloat _x, GLfloat _y, GLfloat _z, GLfloat _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec4f(GLfloat _x)
  : x(_x), y(_x), z(_x), w(_x) {}

  /**
   * @return Vec4f casted to Vec3f.
   */
  inline Vec3f& toVec3f()
  {
    return *((Vec3f*)this);
  }
  /**
   * @param scalar multiply with scalar.
   * @return product of scalar and vector.
   */
  inline Vec4f operator*(GLfloat scalar) const
  {
    return Vec4f(x*scalar, y*scalar, z*scalar, w*scalar);
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

  Vec2d()
  : x(0.0), y(0.0) {}
  /**
   * Set-component constructor.
   */
  Vec2d(GLdouble _x, GLdouble _y)
  : x(_x), y(_y) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec2d(GLdouble _x)
  : x(_x), y(_x) {}

  /**
   * Add two vectors.
   * @param b another vector.
   * @return the vector sum.
   */
  inline Vec2d operator+(const Vec2d &b)
  {
    return Vec2d(x+b.x, y+b.y);
  }
};

/**
 * \brief A 3D vector of double values.
 */
struct Vec3d {
  GLdouble x; /**< the x component. */
  GLdouble y; /**< the y component. */
  GLdouble z; /**< the z component. */

  Vec3d()
  : x(0.0), y(0.0), z(0.0) {}
  /**
   * Set-component constructor.
   */
  Vec3d(GLdouble _x, GLdouble _y, GLdouble _z)
  : x(_x), y(_y), z(_z) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec3d(GLdouble _x)
  : x(_x), y(_x), z(_x) {}
};

/**
 * \brief A 4D vector of double values.
 */
struct Vec4d {
  GLdouble x; /**< the x component. */
  GLdouble y; /**< the y component. */
  GLdouble z; /**< the z component. */
  GLdouble w; /**< the w component. */

  Vec4d()
  : x(0.0), y(0.0), z(0.0), w(0.0) {}
  /**
   * Set-component constructor.
   */
  Vec4d(GLdouble _x, GLdouble _y, GLdouble _z, GLdouble _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec4d(GLdouble _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

/**
 * \brief A 2D vector of int values.
 */
struct Vec2i {
  GLint x; /**< the x component. */
  GLint y; /**< the y component. */

  Vec2i()
  : x(0), y(0) {}
  /**
   * Set-component constructor.
   */
  Vec2i(GLint _x, GLint _y)
  : x(_x), y(_y) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec2i(GLint _x)
  : x(_x), y(_x) {}

  /**
   * @param b vector to subtract.
   * @return the vector difference.
   */
  inline Vec2i operator-(const Vec2i &b)
  {
    return Vec2i(x-b.x, y-b.y);
  }
  /**
   * @param b vector to add.
   */
  inline void operator+=(const Vec2i &b)
  {
    x+=b.x; y+=b.y;
  }
};

/**
 * \brief A 3D vector of int values.
 */
struct Vec3i {
  GLint x; /**< the x component. */
  GLint y; /**< the y component. */
  GLint z; /**< the z component. */

  Vec3i()
  : x(0), y(0), z(0) {}
  /**
   * Set-component constructor.
   */
  Vec3i(GLint _x, GLint _y, GLint _z)
  : x(_x), y(_y), z(_z) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec3i(GLint _x)
  : x(_x), y(_x), z(_x) {}

  /**
   * @param b vector to subtract.
   * @return vector difference.
   */
  inline Vec3i operator-(const Vec3i &b)
  {
    return Vec3i(x-b.x, y-b.y, z-b.z);
  }
};

/**
 * \brief A 4D vector of int values.
 */
struct Vec4i {
  GLint x; /**< the x component. */
  GLint y; /**< the y component. */
  GLint z; /**< the z component. */
  GLint w; /**< the w component. */

  Vec4i()
  : x(0), y(0), z(0), w(0) {}
  /**
   * Set-component constructor.
   */
  Vec4i(GLint _x, GLint _y, GLint _z, GLint _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec4i(GLint _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

/**
 * \brief A 2D vector of unsigned int values.
 */
struct Vec2ui {
  GLuint x; /**< the x component. */
  GLuint y; /**< the y component. */

  Vec2ui()
  : x(0u), y(0u) {}
  /**
   * Set-component constructor.
   */
  Vec2ui(GLuint _x, GLuint _y)
  : x(_x), y(_y) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec2ui(GLuint _x)
  : x(_x), y(_x) {}
};

/**
 * \brief A 3D vector of unsigned int values.
 */
struct Vec3ui {
  GLuint x; /**< the x component. */
  GLuint y; /**< the y component. */
  GLuint z; /**< the z component. */

  Vec3ui()
  : x(0u), y(0u), z(0u) {}
  /**
   * Set-component constructor.
   */
  Vec3ui(GLuint _x, GLuint _y, GLuint _z)
  : x(_x), y(_y), z(_z) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec3ui(GLuint _x)
  : x(_x), y(_x), z(_x) {}
};

/**
 * \brief A 4D vector of unsigned int values.
 */
struct Vec4ui {
  GLuint x; /**< the x component. */
  GLuint y; /**< the y component. */
  GLuint z; /**< the z component. */
  GLuint w; /**< the w component. */

  Vec4ui()
  : x(0u), y(0u), z(0u), w(0u) {}
  /**
   * Set-component constructor.
   */
  Vec4ui(GLuint _x, GLuint _y, GLuint _z, GLuint _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec4ui(GLuint _x)
  : x(_x), y(_x), z(_x), w(_x) {}
};

/**
 * \brief A 2D vector of bool values.
 */
struct Vec2b {
  GLboolean x; /**< the x component. */
  GLboolean y; /**< the y component. */

  Vec2b()
  : x(GL_FALSE), y(GL_FALSE) {}
  /**
   * Set-component constructor.
   */
  Vec2b(GLboolean _x, GLboolean _y)
  : x(_x), y(_y) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec2b(GLboolean _x)
  : x(_x), y(_x) {}
};

/**
 * \brief A 3D vector of bool values.
 */
struct Vec3b {
  GLboolean x; /**< the x component. */
  GLboolean y; /**< the y component. */
  GLboolean z; /**< the z component. */

  Vec3b()
  : x(GL_FALSE), y(GL_FALSE), z(GL_FALSE) {}
  /**
   * Set-component constructor.
   */
  Vec3b(GLboolean _x, GLboolean _y, GLboolean _z)
  : x(_x), y(_y), z(_z) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec3b(GLboolean _x)
  : x(_x), y(_x), z(_x) {}
};

/**
 * \brief A 2D vector of bool values.
 */
struct Vec4b {
  GLboolean x; /**< the x component. */
  GLboolean y; /**< the y component. */
  GLboolean z; /**< the z component. */
  GLboolean w; /**< the w component. */

  Vec4b()
  : x(GL_FALSE), y(GL_FALSE), z(GL_FALSE), w(GL_FALSE) {}
  /**
   * Set-component constructor.
   */
  Vec4b(GLboolean _x, GLboolean _y, GLboolean _z, GLboolean _w)
  : x(_x), y(_y), z(_z), w(_w) {}
  /**
   * @param _x value that is applied to all components.
   */
  Vec4b(GLboolean _x)
  : x(_x), y(_x), z(_x), w(_x) {}
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
