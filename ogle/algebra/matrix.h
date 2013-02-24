/*
 * matrix.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <ogle/algebra/vector.h>

struct Mat3f {
  float x[9];
  Mat3f() {}
  Mat3f(
      float x0, float x1, float x2,
      float x3, float x4, float x5,
      float x6, float x7, float x8)
  {
    x[0] = x0; x[1] = x1; x[2] = x2;
    x[3] = x3; x[4] = x4; x[5] = x5;
    x[6] = x6; x[7] = x7; x[8] = x8;
  }

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

struct Mat4f {
  float x[16];
  Mat4f() {}
  Mat4f(
      float x0, float x1, float x2, float x3,
      float x4, float x5, float x6, float x7,
      float x8, float x9, float x10, float x11,
      float x12, float x13, float x14, float x15)
  {
    x[0 ] = x0;  x[1 ] = x1;  x[2 ] = x2;  x[3 ] = x3;
    x[4 ] = x4;  x[5 ] = x5;  x[6 ] = x6;  x[7 ] = x7;
    x[8 ] = x8;  x[9 ] = x9;  x[10] = x10; x[11] = x11;
    x[12] = x12; x[13] = x13; x[14] = x14; x[15] = x15;
  }

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

  float operator()(int i, int j) const
  {
    return x[i*4 + j];
  }

  inline Vec4f operator*(const Vec4f &v) const
  {
    return Vec4f(
        v.x*x[0 ] + v.y*x[1 ] + v.z*x[2 ] + v.w*x[3 ],
        v.x*x[4 ] + v.y*x[5 ] + v.z*x[6 ] + v.w*x[7 ],
        v.x*x[8 ] + v.y*x[9 ] + v.z*x[10] + v.w*x[11],
        v.x*x[12] + v.y*x[13] + v.z*x[14] + v.w*x[15]
    );
  }
  inline Vec4f operator*(const Vec3f &v) const
  {
    return Vec4f(
        v.x*x[0 ] + v.y*x[1 ] + v.z*x[2 ] + x[3 ],
        v.x*x[4 ] + v.y*x[5 ] + v.z*x[6 ] + x[7 ],
        v.x*x[8 ] + v.y*x[9 ] + v.z*x[10] + x[11],
        v.x*x[12] + v.y*x[13] + v.z*x[14] + x[15]
    );
  }
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

  inline void translate(const Vec3f &translation)
  {
    x[12] += translation.x;
    x[13] += translation.y;
    x[14] += translation.z;
  }
  inline void setTranslation(const Vec3f &translation)
  {
    x[12] = translation.x;
    x[13] = translation.y;
    x[14] = translation.z;
  }

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

  inline void setForward(const Vec3f &vec3)
  {
    x[8 ] = vec3.x;
    x[9 ] = vec3.y;
    x[10] = vec3.z;
  }
  inline void setUp(const Vec3f &vec3)
  {
    x[4 ] = vec3.x;
    x[5 ] = vec3.y;
    x[6 ] = vec3.z;
  }
  inline void setRight(const Vec3f &vec3)
  {
    x[0 ] = vec3.x;
    x[1 ] = vec3.y;
    x[2 ] = vec3.z;
  }

  inline Vec3f rotate(const Vec3f &v3) const
  {
    return ((*this)*(Vec4f(v3.x, v3.y, v3.z, 0.0))).toStruct3f();
  }

  inline Vec3f transform(const Vec3f &v3) const
  {
    return ((*this)*(Vec4f(v3.x, v3.y, v3.z, 1.0))).toStruct3f();
  }
  inline Vec4f transform(const Vec4f &v4) const
  {
    return (*this) * v4;
  }

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
  inline Mat4f projectionInverse() const
  {
    return Mat4f(
        1.0/x[0],      0.0,  0.0,          0.0,
        0.0,      1.0/x[5],  0.0,          0.0,
        0.0,           0.0,  0.0,    1.0/x[14],
        0.0,           0.0, -1.0,  x[10]/x[14]
    );
  }

  inline Mat4f transpose() const
  {
    Mat4f ret;
    for(unsigned int i=0; i<4; ++i) {
      for(unsigned int j=0; j<4; ++j) {
        ret.x[j*4 + i] = x[i*4 + j];
      }
    }
    return ret;
  }

  static inline Mat4f cropMatrix(
      float minX, float maxX,
      float minY, float maxY)
  {
    float scaleX = 2.0 / (maxX - minX);
    float scaleY = 2.0 / (maxY - minY);
    return Mat4f(
                               scaleX,                           0.0, 0.0, 0.0,
                                  0.0,                        scaleY, 0.0, 0.0,
                                  0.0,                           0.0, 1.0, 0.0,
        -0.5 * (maxX + minX) * scaleX, -0.5 * (maxY + minY) * scaleY, 0.0, 1.0
    );
  }

  /**
   * equivalent to glOrtho.
   * @see: http://www.opengl.org/sdk/docs/man/xhtml/glOrtho.xml
   */
  static inline Mat4f orthogonalMatrix(
      float l, float r,
      float b, float t,
      float n, float f)
  {
    return Mat4f(
        2.0 / (r-l),           0.0,           0.0, 0.0,
                0.0,   2.0 / (t-b),           0.0, 0.0,
                0.0,           0.0,    -2.0/(f-n), 0.0,
        -(r+l)/(r-l),  -(t+b)/(t-b), -(f+n)/(f-n), 1.0
    );
  }

  /**
   * equivalent to gluPerspective.
   * @see: http://www.opengl.org/sdk/docs/man/xhtml/gluPerspective.xml
   */
  static inline Mat4f projectionMatrix(
      float fov, float aspect, float near, float far)
  {
    float _x = fov*0.008726645; // degree to RAD
    float f = cos(_x)/sin(_x);
    return Mat4f(
        f/aspect, 0.0,                     0.0,  0.0,
        0.0,        f,                     0.0,  0.0,
        0.0,      0.0,   (far+near)/(near-far), -1.0,
        0.0,      0.0, 2.0*far*near/(near-far),  0.0
    );
  }

  /**
    * equivalent to glFrustum.
    * @see: http://www.opengl.org/sdk/docs/man/xhtml/glFrustum.xml
    */
  static inline Mat4f frustumMatrix(
      float left, float right,
      float bottom, float top,
      float near, float far)
  {
    return Mat4f(
        (2*near)/(right-left),                       0.0,                      0.0,  0.0,
                          0.0,     (2*near)/(top-bottom),                      0.0,  0.0,
    (right+left)/(right-left), (top+bottom)/(top-bottom),   -(far+near)/(far-near), -1.0,
                          0.0,                       0.0, -(2*far*near)/(far-near),  0.0
    );
  }

  /**
   * equivalent to gluLookAt.
   * @see: http://www.opengl.org/sdk/docs/man/xhtml/gluLookAt.xml
   * @note: up vector must be normalized
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
   * creates a new rotation matrix that rotates x/y/z axes.
   */
  static inline Mat4f rotationMatrix(float x, float y, float z)
  {
    float cx = cos(x), sx = sin(x);
    float cy = cos(y), sy = sin(y);
    float cz = cos(z), sz = sin(z);
    float sxsy = sx*sy;
    float cxsy = cx*sy;
    return Mat4f(
         cy*cz,  sxsy*cz+cx*sz, -cxsy*cz+sx*sz, 0.0,
        -cy*sz, -sxsy*sz+cx*cz,  cxsy*sz+sx*cz, 0.0,
            sy,         -sx*cy,          cx*cy, 0.0,
           0.0,            0.0,            0.0, 1.0
    );
  }

  static inline Mat4f transformationMatrix(
      const Vec3f &rot, const Vec3f &translation, const Vec3f &scale)
  {
    float cx = cos(rot.x), sx = sin(rot.x);
    float cy = cos(rot.y), sy = sin(rot.y);
    float cz = cos(rot.z), sz = sin(rot.z);
    float sxsy = sx*sy;
    float cxsy = cx*sy;
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

const Mat4f* getCubeLookAtMatrices();
Mat4f* getCubeLookAtMatrices(const Vec3f &pos);

#endif /* _MATRIX_H_ */
