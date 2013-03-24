/*
 * matrix.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <ogle/algebra/vector.h>

namespace ogle {

/**
 * \brief A 3x3 matrix.
 */
struct Mat3f {
  GLfloat x[9]; /**< Matrix coefficients. */

  Mat3f() {}
  /**
   * Set-component constructor.
   */
  Mat3f(
      GLfloat x0, GLfloat x1, GLfloat x2,
      GLfloat x3, GLfloat x4, GLfloat x5,
      GLfloat x6, GLfloat x7, GLfloat x8)
  {
    x[0] = x0; x[1] = x1; x[2] = x2;
    x[3] = x3; x[4] = x4; x[5] = x5;
    x[6] = x6; x[7] = x7; x[8] = x8;
  }

  /**
   * @return the identity matrix.
   */
  static inline const Mat3f& identity()
  {
    static Mat3f id = Mat3f(
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 0.0, 1.0
    );
    return id;
  }
};

/**
 * \brief A 4x4 matrix.
 */
struct Mat4f {
  GLfloat x[16]; /**< Matrix coefficients. */

  Mat4f() {}
  /**
   * Set-component constructor.
   */
  Mat4f(
      GLfloat x0, GLfloat x1, GLfloat x2, GLfloat x3,
      GLfloat x4, GLfloat x5, GLfloat x6, GLfloat x7,
      GLfloat x8, GLfloat x9, GLfloat x10, GLfloat x11,
      GLfloat x12, GLfloat x13, GLfloat x14, GLfloat x15)
  {
    x[0 ] = x0;  x[1 ] = x1;  x[2 ] = x2;  x[3 ] = x3;
    x[4 ] = x4;  x[5 ] = x5;  x[6 ] = x6;  x[7 ] = x7;
    x[8 ] = x8;  x[9 ] = x9;  x[10] = x10; x[11] = x11;
    x[12] = x12; x[13] = x13; x[14] = x14; x[15] = x15;
  }

  /**
   * @return the identity matrix.
   */
  static inline const Mat4f& identity()
  {
    static Mat4f id = Mat4f(
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0
    );
    return id;
  }

  /**
   * Matrix that maps vectors from [-1,1] to [0,1].
   * @return the bias matrix.
   */
  static inline const Mat4f& bias()
  {
    static Mat4f m = Mat4f(
      0.5, 0.0, 0.0, 0.0,
      0.0, 0.5, 0.0, 0.0,
      0.0, 0.0, 0.5, 0.0,
      0.5, 0.5, 0.5, 1.0
    );
    return m;
  }

  /**
   * Access a single coefficient.
   * @param i row index.
   * @param j column index.
   * @return the coefficient.
   */
  GLfloat operator()(int i, int j) const
  {
    return x[i*4 + j];
  }

  /**
   * Matrix-Vector multiplication.
   * @param v the vector.
   * @return transformed vector.
   */
  inline Vec4f operator*(const Vec4f &v) const
  {
    return Vec4f(
        v.x*x[0 ] + v.y*x[1 ] + v.z*x[2 ] + v.w*x[3 ],
        v.x*x[4 ] + v.y*x[5 ] + v.z*x[6 ] + v.w*x[7 ],
        v.x*x[8 ] + v.y*x[9 ] + v.z*x[10] + v.w*x[11],
        v.x*x[12] + v.y*x[13] + v.z*x[14] + v.w*x[15]
    );
  }
  /**
   * Matrix-Vector multiplication.
   * @param v the vector.
   * @return transformed vector.
   */
  inline Vec4f operator*(const Vec3f &v) const
  {
    return Vec4f(
        v.x*x[0 ] + v.y*x[1 ] + v.z*x[2 ] + x[3 ],
        v.x*x[4 ] + v.y*x[5 ] + v.z*x[6 ] + x[7 ],
        v.x*x[8 ] + v.y*x[9 ] + v.z*x[10] + x[11],
        v.x*x[12] + v.y*x[13] + v.z*x[14] + x[15]
    );
  }
  /**
   * Matrix-Matrix multiplication.
   * @param b another matrix.
   * @return the matrix product.
   */
  inline Mat4f operator*(const Mat4f &b) const
  {
    //(AB)_ij = sum A_ik*B_kj
    const Mat4f &a = *this;
    return Mat4f(
        a(0,0)*b(0,0) + a(0,1)*b(1,0) + a(0,2)*b(2,0) + a(0,3)*b(3,0), // i=0, j=0
        a(0,0)*b(0,1) + a(0,1)*b(1,1) + a(0,2)*b(2,1) + a(0,3)*b(3,1), // i=0, j=1
        a(0,0)*b(0,2) + a(0,1)*b(1,2) + a(0,2)*b(2,2) + a(0,3)*b(3,2), // i=0, j=2
        a(0,0)*b(0,3) + a(0,1)*b(1,3) + a(0,2)*b(2,3) + a(0,3)*b(3,3), // i=0, j=3

        a(1,0)*b(0,0) + a(1,1)*b(1,0) + a(1,2)*b(2,0) + a(1,3)*b(3,0), // i=1, j=0
        a(1,0)*b(0,1) + a(1,1)*b(1,1) + a(1,2)*b(2,1) + a(1,3)*b(3,1), // i=1, j=1
        a(1,0)*b(0,2) + a(1,1)*b(1,2) + a(1,2)*b(2,2) + a(1,3)*b(3,2), // i=1, j=2
        a(1,0)*b(0,3) + a(1,1)*b(1,3) + a(1,2)*b(2,3) + a(1,3)*b(3,3), // i=1, j=3

        a(2,0)*b(0,0) + a(2,1)*b(1,0) + a(2,2)*b(2,0) + a(2,3)*b(3,0), // i=2, j=0
        a(2,0)*b(0,1) + a(2,1)*b(1,1) + a(2,2)*b(2,1) + a(2,3)*b(3,1), // i=2, j=1
        a(2,0)*b(0,2) + a(2,1)*b(1,2) + a(2,2)*b(2,2) + a(2,3)*b(3,2), // i=2, j=2
        a(2,0)*b(0,3) + a(2,1)*b(1,3) + a(2,2)*b(2,3) + a(2,3)*b(3,3), // i=2, j=3

        a(3,0)*b(0,0) + a(3,1)*b(1,0) + a(3,2)*b(2,0) + a(3,3)*b(3,0), // i=3, j=0
        a(3,0)*b(0,1) + a(3,1)*b(1,1) + a(3,2)*b(2,1) + a(3,3)*b(3,1), // i=3, j=1
        a(3,0)*b(0,2) + a(3,1)*b(1,2) + a(3,2)*b(2,2) + a(3,3)*b(3,2), // i=3, j=2
        a(3,0)*b(0,3) + a(3,1)*b(1,3) + a(3,2)*b(2,3) + a(3,3)*b(3,3)  // i=3, j=3
    );
  }
  /**
   * @param b another matrix.
   * @return this matrix minus other matrix.
   */
  inline Mat4f operator-(const Mat4f &b) const
  {
    const Mat4f &a = *this;
    return Mat4f(
        a(0,0)-b(0,0), // i=0, j=0
        a(0,1)-b(0,1), // i=0, j=1
        a(0,2)-b(0,2), // i=0, j=2
        a(0,3)-b(0,3), // i=0, j=3

        a(1,0)-b(1,0), // i=1, j=0
        a(1,1)-b(1,1), // i=1, j=1
        a(1,2)-b(1,2), // i=1, j=2
        a(1,3)-b(1,3), // i=1, j=3

        a(2,0)-b(2,0), // i=2, j=0
        a(2,1)-b(2,1), // i=2, j=1
        a(2,2)-b(2,2), // i=2, j=2
        a(2,3)-b(2,3), // i=2, j=3

        a(3,0)-b(3,0), // i=3, j=0
        a(3,1)-b(3,1), // i=3, j=1
        a(3,2)-b(3,2), // i=3, j=2
        a(3,3)-b(3,3)  // i=3, j=3
    );
  }

  /**
   * Add translation component.
   * @param translation the translation vector.
   */
  inline void translate(const Vec3f &translation)
  {
    x[ 3] += translation.x;
    x[ 7] += translation.y;
    x[11] += translation.z;
  }
  /**
   * Set translation component.
   * @param translation the translation vector.
   */
  inline void setTranslation(const Vec3f &translation)
  {
    x[ 3] = translation.x;
    x[ 7] = translation.y;
    x[11] = translation.z;
  }

  /**
   * Scale this matrix.
   * @param scale the scale factors.
   */
  inline void scale(const Vec3f &scale)
  {
    x[0 ] *= scale.x;
    x[1 ] *= scale.y;
    x[2 ] *= scale.z;
    x[4 ] *= scale.x;
    x[5 ] *= scale.y;
    x[6 ] *= scale.z;
    x[8 ] *= scale.x;
    x[9 ] *= scale.y;
    x[10] *= scale.z;
  }

  /**
   * @param v input vector.
   * @return vector multiplied with matrix, ignoring the translation.
   */
  inline Vec3f rotate(const Vec3f &v) const
  {
    return ((*this)*(Vec4f(v.x,v.y,v.z,0.0))).toVec3f();
  }
  /**
   * @param v input vector.
   * @return vector multiplied with matrix.
   */
  inline Vec3f transform(const Vec3f &v) const
  {
    return ((*this)*(Vec4f(v.x,v.y,v.z,1.0))).toVec3f();
  }
  /**
   * @param v input vector.
   * @return vector multiplied with matrix.
   */
  inline Vec4f transform(const Vec4f &v) const
  {
    return (*this)*v;
  }

  /**
   * @return the matrix determinant.
   * @see http://en.wikipedia.org/wiki/Determinant
   */
  inline float determinant() const
  {
    return
        x[ 0]*x[ 5]*x[10]*x[15] -
        x[ 0]*x[ 5]*x[11]*x[14] +
        x[ 0]*x[ 6]*x[11]*x[13] -
        x[ 0]*x[ 6]*x[ 9]*x[15] +
        //
        x[ 0]*x[ 7]*x[ 9]*x[14] -
        x[ 0]*x[ 7]*x[10]*x[13] -
        x[ 1]*x[ 6]*x[11]*x[12] +
        x[ 1]*x[ 6]*x[ 8]*x[15] -
        //
        x[ 1]*x[ 7]*x[ 8]*x[14] +
        x[ 1]*x[ 7]*x[10]*x[12] -
        x[ 1]*x[ 4]*x[10]*x[15] +
        x[ 1]*x[ 4]*x[11]*x[14] +
        //
        x[ 2]*x[ 7]*x[ 8]*x[13] -
        x[ 2]*x[ 7]*x[ 9]*x[12] +
        x[ 2]*x[ 4]*x[ 9]*x[15] -
        x[ 2]*x[ 4]*x[11]*x[13] +
        //
        x[ 2]*x[ 5]*x[11]*x[12] -
        x[ 2]*x[ 5]*x[ 8]*x[15] -
        x[ 3]*x[ 4]*x[ 9]*x[14] +
        x[ 3]*x[ 4]*x[10]*x[13] -
        //
        x[ 3]*x[ 5]*x[10]*x[12] +
        x[ 3]*x[ 5]*x[ 8]*x[14] -
        x[ 3]*x[ 6]*x[ 8]*x[13] +
        x[ 3]*x[ 6]*x[ 9]*x[12];
  }

  /**
   * Slow but generic inverse computation using the determinant.
   * @return the inverse matrix.
   */
  inline Mat4f inverse() const
  {
      // Compute the reciprocal determinant
      float det = determinant();
      if (det == 0.0f)
      {
          // Matrix not invertible.
          return identity();
      }

      float invdet = 1.0f / det;

      Mat4f res;
      res.x[ 0] =  invdet * (
          x[ 5] * (x[10] * x[15] - x[11] * x[14]) +
          x[ 6] * (x[11] * x[13] - x[ 9] * x[15]) +
          x[ 7] * (x[ 9] * x[14] - x[10] * x[13]));

      res.x[ 1] = -invdet * (
          x[ 1] * (x[10] * x[15] - x[11] * x[14]) +
          x[ 2] * (x[11] * x[13] - x[ 9] * x[15]) +
          x[ 3] * (x[ 9] * x[14] - x[10] * x[13]));

      res.x[ 2] =  invdet * (
          x[ 1] * (x[ 6] * x[15] - x[ 7] * x[14]) +
          x[ 2] * (x[ 7] * x[13] - x[ 5] * x[15]) +
          x[ 3] * (x[ 5] * x[14] - x[ 6] * x[13]));

      res.x[ 3] = -invdet * (
          x[ 1] * (x[ 6] * x[11] - x[ 7] * x[10]) +
          x[ 2] * (x[ 7] * x[ 9] - x[ 5] * x[11]) +
          x[ 3] * (x[ 5] * x[10] - x[ 6] * x[ 9]));

      res.x[ 4] = -invdet * (
          x[ 4] * (x[10] * x[15] - x[11] * x[14]) +
          x[ 6] * (x[11] * x[12] - x[ 8] * x[15]) +
          x[ 7] * (x[ 8] * x[14] - x[10] * x[12]));

      res.x[ 5] =  invdet * (
          x[ 0] * (x[10] * x[15] - x[11] * x[14]) +
          x[ 2] * (x[11] * x[12] - x[ 8] * x[15]) +
          x[ 3] * (x[ 8] * x[14] - x[10] * x[12]));

      res.x[ 6] = -invdet * (
          x[ 0] * (x[ 6] * x[15] - x[ 7] * x[14]) +
          x[ 2] * (x[ 7] * x[12] - x[ 4] * x[15]) +
          x[ 3] * (x[ 4] * x[14] - x[ 6] * x[12]));

      res.x[ 7] =  invdet * (
          x[ 0] * (x[ 6] * x[11] - x[ 7] * x[10]) +
          x[ 2] * (x[ 7] * x[ 8] - x[ 4] * x[11]) +
          x[ 3] * (x[ 4] * x[10] - x[ 6] * x[ 8]));

      res.x[ 8] =  invdet * (
          x[ 4] * (x[ 9] * x[15] - x[11] * x[13]) +
          x[ 5] * (x[11] * x[12] - x[ 8] * x[15]) +
          x[ 7] * (x[ 8] * x[13] - x[ 9] * x[12]));

      res.x[ 9] = -invdet * (
          x[ 0] * (x[ 9] * x[15] - x[11] * x[13]) +
          x[ 1] * (x[11] * x[12] - x[ 8] * x[15]) +
          x[ 3] * (x[ 8] * x[13] - x[ 9] * x[12]));

      res.x[10] =  invdet * (
          x[ 0] * (x[ 5] * x[15] - x[ 7] * x[13]) +
          x[ 1] * (x[ 7] * x[12] - x[ 4] * x[15]) +
          x[ 3] * (x[ 4] * x[13] - x[ 5] * x[12]));

      res.x[11] = -invdet * (
          x[ 0] * (x[ 5] * x[11] - x[ 7] * x[ 9]) +
          x[ 1] * (x[ 7] * x[ 8] - x[ 4] * x[11]) +
          x[ 3] * (x[ 4] * x[ 9] - x[ 5] * x[ 8]));

      res.x[12] = -invdet * (
          x[ 4] * (x[ 9] * x[14] - x[10] * x[13]) +
          x[ 5] * (x[10] * x[12] - x[ 8] * x[14]) +
          x[ 6] * (x[ 8] * x[13] - x[ 9] * x[12]));

      res.x[13] =  invdet * (
          x[ 0] * (x[ 9] * x[14] - x[10] * x[13]) +
          x[ 1] * (x[10] * x[12] - x[ 8] * x[14]) +
          x[ 2] * (x[ 8] * x[13] - x[ 9] * x[12]));

      res.x[14] = -invdet * (
          x[ 0] * (x[ 5] * x[14] - x[ 6] * x[13]) +
          x[ 1] * (x[ 6] * x[12] - x[ 4] * x[14]) +
          x[ 2] * (x[ 4] * x[13] - x[ 5] * x[12]));

      res.x[15] =  invdet * (
          x[ 0] * (x[ 5] * x[10] - x[ 6] * x[ 9]) +
          x[ 1] * (x[ 6] * x[ 8] - x[ 4] * x[10]) +
          x[ 2] * (x[ 4] * x[ 9] - x[ 5] * x[ 8]));

      return res;
  }

  /**
   * Quick computation of look at matrix inverse.
   * @return the inverse matrix.
   */
  inline Mat4f lookAtInverse() const
  {
    return Mat4f(
        x[0], x[4], x[8], 0.0,
        x[1], x[5], x[9], 0.0,
        x[2], x[6], x[10], 0.0,
        -(x[12] * x[0]) - (x[13] * x[1]) - (x[14] * x[2]),
        -(x[12] * x[4]) - (x[13] * x[5]) - (x[14] * x[6]),
        -(x[12] * x[8]) - (x[13] * x[9]) - (x[14] * x[10]),
        1.0
    );
  }
  /**
   * Quick computation of projection matrix inverse.
   * @return the inverse matrix.
   */
  inline Mat4f projectionInverse() const
  {
    return Mat4f(
        1.0/x[0],      0.0,  0.0,          0.0,
        0.0,      1.0/x[5],  0.0,          0.0,
        0.0,           0.0,  0.0,    1.0/x[14],
        0.0,           0.0, -1.0,  x[10]/x[14]
    );
  }

  /**
   * @return the transpose matrix.
   * @see http://en.wikipedia.org/wiki/Transpose
   */
  inline Mat4f transpose() const
  {
    Mat4f ret;
    for(GLuint i=0; i<4; ++i) {
      for(GLuint j=0; j<4; ++j) {
        ret.x[j*4 + i] = x[i*4 + j];
      }
    }
    return ret;
  }

  /**
   * Computes matrix to increase x/y precision of orthogonal projection.
   * @param minX lower bound for x component.
   * @param maxX upper bound for x component.
   * @param minY lower bound for y component.
   * @param maxY upper bound for y component.
   * @return the crop matrix.
   */
  static inline Mat4f cropMatrix(
      GLfloat minX, GLfloat maxX,
      GLfloat minY, GLfloat maxY)
  {
    GLfloat scaleX = 2.0 / (maxX - minX);
    GLfloat scaleY = 2.0 / (maxY - minY);
    return Mat4f(
                               scaleX,                           0.0, 0.0, 0.0,
                                  0.0,                        scaleY, 0.0, 0.0,
                                  0.0,                           0.0, 1.0, 0.0,
        -0.5 * (maxX + minX) * scaleX, -0.5 * (maxY + minY) * scaleY, 0.0, 1.0
    );
  }

  /**
   * Compute a parallel projection matrix.
   * @param l specifies the coordinates for the left vertical clipping plane.
   * @param r specifies the coordinates for the right vertical clipping plane.
   * @param b specifies the coordinates for the bottom horizontal clipping plane.
   * @param t specifies the coordinates for the top horizontal clipping plane.
   * @param n Specify the distances to the nearer depth clipping planes.
   * @param f Specify the distances to the farther depth clipping planes.
   * @return the parallel projection matrix.
   * @note Equivalent to glOrtho.
   */
  static inline Mat4f orthogonalMatrix(
      GLfloat l, GLfloat r,
      GLfloat b, GLfloat t,
      GLfloat n, GLfloat f)
  {
    return Mat4f(
        2.0 / (r-l),           0.0,           0.0, 0.0,
                0.0,   2.0 / (t-b),           0.0, 0.0,
                0.0,           0.0,    -2.0/(f-n), 0.0,
        -(r+l)/(r-l),  -(t+b)/(t-b), -(f+n)/(f-n), 1.0
    );
  }

  /**
   * Compute a perspective projection matrix.
   * @param fov specifies the field of view angle, in degrees, in the y direction.
   * @param aspect specifies the aspect ratio that determines the field of view in the x direction.
   * @param near specifies the distance from the viewer to the near clipping plane (always positive).
   * @param far specifies the distance from the viewer to the far clipping plane (always positive).
   * @return the perspective projection matrix.
   * @note Equivalent to gluPerspective.
   */
  static inline Mat4f projectionMatrix(
      GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far)
  {
    GLfloat _x = fov*0.008726645; // degree to RAD
    GLfloat f = cos(_x)/sin(_x);
    return Mat4f(
        f/aspect, 0.0,                     0.0,  0.0,
        0.0,        f,                     0.0,  0.0,
        0.0,      0.0,   (far+near)/(near-far), -1.0,
        0.0,      0.0, 2.0*far*near/(near-far),  0.0
    );
  }

  /**
   * Compute a projection matrix.
   * @param left specifies the coordinates for the left vertical clipping planes.
   * @param right specifies the coordinates for the right vertical clipping planes.
   * @param bottom specifies the coordinates for the bottom horizontal clipping planes.
   * @param top specifies the coordinates for the top horizontal clipping planes.
   * @param near specifies the distances to the near depth clipping planes.
   * @param far specifies the distances to the far depth clipping planes.
   * @return the projection matrix.
   * @note Equivalent to glFrustum.
   */
  static inline Mat4f frustumMatrix(
      GLfloat left, GLfloat right,
      GLfloat bottom, GLfloat top,
      GLfloat near, GLfloat far)
  {
    return Mat4f(
        (2*near)/(right-left),                       0.0,                      0.0,  0.0,
                          0.0,     (2*near)/(top-bottom),                      0.0,  0.0,
    (right+left)/(right-left), (top+bottom)/(top-bottom),   -(far+near)/(far-near), -1.0,
                          0.0,                       0.0, -(2*far*near)/(far-near),  0.0
    );
  }

  /**
   * Compute view transformation matrix.
   * @param pos specifies the position of the eye point.
   * @param dir specifies the look at direction.
   * @param up specifies the direction of the up vector. must be normalized.
   * @return the view transformation matrix.
   * @note Equivalent to gluLookAt.
   */
  static inline Mat4f lookAtMatrix(
      const Vec3f &pos, const Vec3f &dir, const Vec3f &up)
  {
    Vec3f t = -pos;
    Vec3f f = dir; f.normalize();
    Vec3f s = f.cross(up); s.normalize();
    Vec3f u = s.cross(f);
    return Mat4f(
             s.x,      u.x,        -f.x, 0.0,
             s.y,      u.y,        -f.y, 0.0,
             s.z,      u.z,        -f.z, 0.0,
        s.dot(t), u.dot(t), (-f).dot(t), 1.0
    );
  }

  /**
   * Compute view transformation matrices.
   * @param pos cube center position.
   * @return 6 view transformation matrices, one for each cube face.
   * @note you have to call delete[] when you are done using the returned pointer.
   */
  static inline Mat4f* cubeLookAtMatrices(const Vec3f &pos)
  {
    Mat4f *views = new Mat4f[6];
    cubeLookAtMatrices(pos, views);
    return views;
  }
  /**
   * Compute view transformation matrices.
   * @param pos cube center position.
   * @param views matrix array.
   * @return 6 view transformation matrices, one for each cube face.
   * @note you have to call delete[] when you are done using the returned pointer.
   */
  static inline void cubeLookAtMatrices(const Vec3f &pos, Mat4f *views)
  {
    const Vec3f dir[6] = {
        Vec3f( 1.0f, 0.0f, 0.0f),
        Vec3f(-1.0f, 0.0f, 0.0f),
        Vec3f( 0.0f, 1.0f, 0.0f),
        Vec3f( 0.0f,-1.0f, 0.0f),
        Vec3f( 0.0f, 0.0f, 1.0f),
        Vec3f( 0.0f, 0.0f,-1.0f)
    };
    const Vec3f up[6] = {
        Vec3f( 0.0f, -1.0f, 0.0f),
        Vec3f( 0.0f, -1.0f, 0.0f),
        Vec3f( 0.0f, 0.0f,  1.0f),
        Vec3f( 0.0f, 0.0f, -1.0f),
        Vec3f( 0.0f, -1.0f, 0.0f),
        Vec3f( 0.0f, -1.0f, 0.0f)
    };
    for(register GLuint i=0; i<6; ++i) views[i] = Mat4f::lookAtMatrix(pos, dir[i], up[i]);
  }
  /**
   * Compute view transformation matrices with cube center at origin point (0,0,0).
   * @return 6 view transformation matrices, one for each cube face.
   */
  static inline const Mat4f* cubeLookAtMatrices()
  {
    static Mat4f *views = NULL;
    if(views==NULL) {
      views = Mat4f::cubeLookAtMatrices(Vec3f(0.0));
    }
    return views;
  }

  /**
   * Computes a rotation matrix.
   * @param x rotation of x axis.
   * @param y rotation of y axis.
   * @param z rotation of z axis.
   * @return the rotation matrix.
   */
  static inline Mat4f rotationMatrix(GLfloat x, GLfloat y, GLfloat z)
  {
    GLfloat cx = cos(x), sx = sin(x);
    GLfloat cy = cos(y), sy = sin(y);
    GLfloat cz = cos(z), sz = sin(z);
    GLfloat sxsy = sx*sy;
    GLfloat cxsy = cx*sy;
    return Mat4f(
         cy*cz,  sxsy*cz+cx*sz, -cxsy*cz+sx*sz, 0.0,
        -cy*sz, -sxsy*sz+cx*cz,  cxsy*sz+sx*cz, 0.0,
            sy,         -sx*cy,          cx*cy, 0.0,
           0.0,            0.0,            0.0, 1.0
    );
  }

  /**
   * Computes a transformation matrix with rotation, translation and scaling.
   * @param rot rotation of x/y/z axis.
   * @param translation translation vector.
   * @param scale scale factor for x/y/z components.
   * @return the transformation matrix.
   */
  static inline Mat4f transformationMatrix(
      const Vec3f &rot, const Vec3f &translation, const Vec3f &scale)
  {
    GLfloat cx = cos(rot.x), sx = sin(rot.x);
    GLfloat cy = cos(rot.y), sy = sin(rot.y);
    GLfloat cz = cos(rot.z), sz = sin(rot.z);
    GLfloat sxsy = sx*sy;
    GLfloat cxsy = cx*sy;
    return Mat4f(
        -scale.x*cy*cz,  -scale.y*(sxsy*cz+cx*sz),  scale.z*(cxsy*cz+sx*sz), translation.x,
         scale.x*cy*sz,   scale.y*(sxsy*sz+cx*cz), -scale.z*(cxsy*sz+sx*cz), translation.y,
           -scale.x*sy,             scale.y*sx*cy,           -scale.z*cx*cy, translation.z,
                   0.0,                       0.0,                      0.0, 1.0
    );
  }
};

istream& operator>>(istream& in, Mat3f &v);
istream& operator>>(istream& in, Mat4f &v);
ostream& operator<<(ostream& os, const Mat3f& m);
ostream& operator<<(ostream& os, const Mat4f& m);

} // namespace

#endif /* _MATRIX_H_ */
