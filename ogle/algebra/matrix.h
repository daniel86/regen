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
};
struct Mat4f {
  float x[16];
  float operator()(int i, int j) const
  {
    return x[i*4 + j];
  }
  Mat4f() {}
  Mat4f(
      float x0, float x1, float x2, float x3,
      float x4, float x5, float x6, float x7,
      float x8, float x9, float x10, float x11,
      float x12, float x13, float x14, float x15)
  {
    x[0] = x0; x[1] = x1; x[2] = x2; x[3] = x3;
    x[4] = x4; x[5] = x5; x[6] = x6; x[7] = x7;
    x[8] = x8; x[9] = x9; x[10] = x10; x[11] = x11;
    x[12] = x12; x[13] = x13; x[14] = x14; x[15] = x15;
  }
};

inline Mat3f identity3f()
{
  return (Mat3f) {
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 0.0, 1.0
  };
}
inline Mat4f identity4f()
{
  return (Mat4f) {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0
  };
}


istream& operator>>(istream& in, Mat3f &v);
istream& operator>>(istream& in, Mat4f &v);
ostream& operator<<(ostream& os, const Mat3f& m);
ostream& operator<<(ostream& os, const Mat4f& m);

inline Vec4f operator*(const Mat4f &mat, const Vec4f &v)
{
  return (Vec4f) {
      v.x*mat.x[0] + v.y*mat.x[1] + v.z*mat.x[2] + v.w*mat.x[3],
      v.x*mat.x[4] + v.y*mat.x[5] + v.z*mat.x[6] + v.w*mat.x[7],
      v.x*mat.x[8] + v.y*mat.x[9] + v.z*mat.x[10] + v.w*mat.x[11],
      v.x*mat.x[12] + v.y*mat.x[13] + v.z*mat.x[14] + v.w*mat.x[15]
  };
}
inline Vec4f operator*(const Mat4f &mat, const Vec3f &v)
{
  return (Vec4f) {
      v.x*mat.x[0] + v.y*mat.x[1] + v.z*mat.x[2] + mat.x[3],
      v.x*mat.x[4] + v.y*mat.x[5] + v.z*mat.x[6] + mat.x[7],
      v.x*mat.x[8] + v.y*mat.x[9] + v.z*mat.x[10] + mat.x[11],
      v.x*mat.x[12] + v.y*mat.x[13] + v.z*mat.x[14] + mat.x[15]
  };
}
inline Mat4f operator*(const Mat4f &a, const Mat4f &b)
{
  //(AB)_ij = sum A_ik*B_kj
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

inline Mat4f operator-(const Mat4f &a, const Mat4f &b)
{
  return (Mat4f) {
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
  };
}

inline void translateMat(Mat4f &mat4x4, const Vec3f &translation)
{
  mat4x4.x[12] += translation.x;
  mat4x4.x[13] += translation.y;
  mat4x4.x[14] += translation.z;
}
inline void setTranslationMat(Mat4f &mat4x4, const Vec3f &translation)
{
  mat4x4.x[12] = translation.x;
  mat4x4.x[13] = translation.y;
  mat4x4.x[14] = translation.z;
}

inline void scaleMat(Mat4f &mat4x4, const Vec3f &scale)
{
  mat4x4.x[0] *= scale.x;
  mat4x4.x[1] *= scale.y;
  mat4x4.x[2] *= scale.z;

  mat4x4.x[4] *= scale.x;
  mat4x4.x[5] *= scale.y;
  mat4x4.x[6] *= scale.z;

  mat4x4.x[8] *= scale.x;
  mat4x4.x[9] *= scale.y;
  mat4x4.x[10] *= scale.z;
}

inline void setForward(Mat4f &mat4x4, const Vec3f &vec3)
{
  mat4x4.x[8] = vec3.x;
  mat4x4.x[9] = vec3.y;
  mat4x4.x[10] = vec3.z;
}
inline void setUp(Mat4f &mat4x4, const Vec3f &vec3)
{
  mat4x4.x[4] = vec3.x;
  mat4x4.x[5] = vec3.y;
  mat4x4.x[6] = vec3.z;
}
inline void setRight(Mat4f &mat4x4, const Vec3f &vec3)
{
  mat4x4.x[0] = vec3.x;
  mat4x4.x[1] = vec3.y;
  mat4x4.x[2] = vec3.z;
}

inline Vec3f rotateVec3(const Mat4f &mat4x4, const Vec3f &v3)
{
  return toStruct3f( mat4x4*((Vec4f) { v3.x, v3.y, v3.z, 0.0 }) );
}

inline Vec3f transformVec3(const Mat4f &mat4x4, const Vec3f &v3)
{
  return toStruct3f( mat4x4*((Vec4f) { v3.x, v3.y, v3.z, 1.0 }) );
}

inline Vec4f transformVec4(const Mat4f &mat4x4, const Vec4f &v4)
{
  return mat4x4 * v4;
}

inline float determinant(const Mat4f &m)
{
  return
      m.x[ 0]*m.x[ 5]*m.x[10]*m.x[15] -
      m.x[ 0]*m.x[ 5]*m.x[11]*m.x[14] +
      m.x[ 0]*m.x[ 6]*m.x[11]*m.x[13] -
      m.x[ 0]*m.x[ 6]*m.x[ 9]*m.x[15] +
      //
      m.x[ 0]*m.x[ 7]*m.x[ 9]*m.x[14] -
      m.x[ 0]*m.x[ 7]*m.x[10]*m.x[13] -
      m.x[ 1]*m.x[ 6]*m.x[11]*m.x[12] +
      m.x[ 1]*m.x[ 6]*m.x[ 8]*m.x[15] -
      //
      m.x[ 1]*m.x[ 7]*m.x[ 8]*m.x[14] +
      m.x[ 1]*m.x[ 7]*m.x[10]*m.x[12] -
      m.x[ 1]*m.x[ 4]*m.x[10]*m.x[15] +
      m.x[ 1]*m.x[ 4]*m.x[11]*m.x[14] +
      //
      m.x[ 2]*m.x[ 7]*m.x[ 8]*m.x[13] -
      m.x[ 2]*m.x[ 7]*m.x[ 9]*m.x[12] +
      m.x[ 2]*m.x[ 4]*m.x[ 9]*m.x[15] -
      m.x[ 2]*m.x[ 4]*m.x[11]*m.x[13] +
      //
      m.x[ 2]*m.x[ 5]*m.x[11]*m.x[12] -
      m.x[ 2]*m.x[ 5]*m.x[ 8]*m.x[15] -
      m.x[ 3]*m.x[ 4]*m.x[ 9]*m.x[14] +
      m.x[ 3]*m.x[ 4]*m.x[10]*m.x[13] -
      //
      m.x[ 3]*m.x[ 5]*m.x[10]*m.x[12] +
      m.x[ 3]*m.x[ 5]*m.x[ 8]*m.x[14] -
      m.x[ 3]*m.x[ 6]*m.x[ 8]*m.x[13] +
      m.x[ 3]*m.x[ 6]*m.x[ 9]*m.x[12];
}

inline Mat4f inverse(const Mat4f& m)
{
    // Compute the reciprocal determinant
    float det = determinant(m);
    if (det == 0.0f)
    {
        // Matrix not invertible.
        return identity4f();
    }

    float invdet = 1.0f / det;

    Mat4f res;
    res.x[ 0] =  invdet * (
        m.x[ 5] * (m.x[10] * m.x[15] - m.x[11] * m.x[14]) +
        m.x[ 6] * (m.x[11] * m.x[13] - m.x[ 9] * m.x[15]) +
        m.x[ 7] * (m.x[ 9] * m.x[14] - m.x[10] * m.x[13]));

    res.x[ 1] = -invdet * (
        m.x[ 1] * (m.x[10] * m.x[15] - m.x[11] * m.x[14]) +
        m.x[ 2] * (m.x[11] * m.x[13] - m.x[ 9] * m.x[15]) +
        m.x[ 3] * (m.x[ 9] * m.x[14] - m.x[10] * m.x[13]));

    res.x[ 2] =  invdet * (
        m.x[ 1] * (m.x[ 6] * m.x[15] - m.x[ 7] * m.x[14]) +
        m.x[ 2] * (m.x[ 7] * m.x[13] - m.x[ 5] * m.x[15]) +
        m.x[ 3] * (m.x[ 5] * m.x[14] - m.x[ 6] * m.x[13]));

    res.x[ 3] = -invdet * (
        m.x[ 1] * (m.x[ 6] * m.x[11] - m.x[ 7] * m.x[10]) +
        m.x[ 2] * (m.x[ 7] * m.x[ 9] - m.x[ 5] * m.x[11]) +
        m.x[ 3] * (m.x[ 5] * m.x[10] - m.x[ 6] * m.x[ 9]));

    res.x[ 4] = -invdet * (
        m.x[ 4] * (m.x[10] * m.x[15] - m.x[11] * m.x[14]) +
        m.x[ 6] * (m.x[11] * m.x[12] - m.x[ 8] * m.x[15]) +
        m.x[ 7] * (m.x[ 8] * m.x[14] - m.x[10] * m.x[12]));

    res.x[ 5] =  invdet * (
        m.x[ 0] * (m.x[10] * m.x[15] - m.x[11] * m.x[14]) +
        m.x[ 2] * (m.x[11] * m.x[12] - m.x[ 8] * m.x[15]) +
        m.x[ 3] * (m.x[ 8] * m.x[14] - m.x[10] * m.x[12]));

    res.x[ 6] = -invdet * (
        m.x[ 0] * (m.x[ 6] * m.x[15] - m.x[ 7] * m.x[14]) +
        m.x[ 2] * (m.x[ 7] * m.x[12] - m.x[ 4] * m.x[15]) +
        m.x[ 3] * (m.x[ 4] * m.x[14] - m.x[ 6] * m.x[12]));

    res.x[ 7] =  invdet * (
        m.x[ 0] * (m.x[ 6] * m.x[11] - m.x[ 7] * m.x[10]) +
        m.x[ 2] * (m.x[ 7] * m.x[ 8] - m.x[ 4] * m.x[11]) +
        m.x[ 3] * (m.x[ 4] * m.x[10] - m.x[ 6] * m.x[ 8]));

    res.x[ 8] =  invdet * (
        m.x[ 4] * (m.x[ 9] * m.x[15] - m.x[11] * m.x[13]) +
        m.x[ 5] * (m.x[11] * m.x[12] - m.x[ 8] * m.x[15]) +
        m.x[ 7] * (m.x[ 8] * m.x[13] - m.x[ 9] * m.x[12]));

    res.x[ 9] = -invdet * (
        m.x[ 0] * (m.x[ 9] * m.x[15] - m.x[11] * m.x[13]) +
        m.x[ 1] * (m.x[11] * m.x[12] - m.x[ 8] * m.x[15]) +
        m.x[ 3] * (m.x[ 8] * m.x[13] - m.x[ 9] * m.x[12]));

    res.x[10] =  invdet * (
        m.x[ 0] * (m.x[ 5] * m.x[15] - m.x[ 7] * m.x[13]) +
        m.x[ 1] * (m.x[ 7] * m.x[12] - m.x[ 4] * m.x[15]) +
        m.x[ 3] * (m.x[ 4] * m.x[13] - m.x[ 5] * m.x[12]));

    res.x[11] = -invdet * (
        m.x[ 0] * (m.x[ 5] * m.x[11] - m.x[ 7] * m.x[ 9]) +
        m.x[ 1] * (m.x[ 7] * m.x[ 8] - m.x[ 4] * m.x[11]) +
        m.x[ 3] * (m.x[ 4] * m.x[ 9] - m.x[ 5] * m.x[ 8]));

    res.x[12] = -invdet * (
        m.x[ 4] * (m.x[ 9] * m.x[14] - m.x[10] * m.x[13]) +
        m.x[ 5] * (m.x[10] * m.x[12] - m.x[ 8] * m.x[14]) +
        m.x[ 6] * (m.x[ 8] * m.x[13] - m.x[ 9] * m.x[12]));

    res.x[13] =  invdet * (
        m.x[ 0] * (m.x[ 9] * m.x[14] - m.x[10] * m.x[13]) +
        m.x[ 1] * (m.x[10] * m.x[12] - m.x[ 8] * m.x[14]) +
        m.x[ 2] * (m.x[ 8] * m.x[13] - m.x[ 9] * m.x[12]));

    res.x[14] = -invdet * (
        m.x[ 0] * (m.x[ 5] * m.x[14] - m.x[ 6] * m.x[13]) +
        m.x[ 1] * (m.x[ 6] * m.x[12] - m.x[ 4] * m.x[14]) +
        m.x[ 2] * (m.x[ 4] * m.x[13] - m.x[ 5] * m.x[12]));

    res.x[15] =  invdet * (
        m.x[ 0] * (m.x[ 5] * m.x[10] - m.x[ 6] * m.x[ 9]) +
        m.x[ 1] * (m.x[ 6] * m.x[ 8] - m.x[ 4] * m.x[10]) +
        m.x[ 2] * (m.x[ 4] * m.x[ 9] - m.x[ 5] * m.x[ 8]));

    return res;
}

inline Mat4f transpose(const Mat4f& mat)
{
  Mat4f ret;
  for(unsigned int i=0; i<4; ++i) {
    for(unsigned int j=0; j<4; ++j) {
      ret.x[j*4 + i] = mat.x[i*4 + j];
    }
  }
  return ret;
}

inline Mat4f getCropMatrix(
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
inline Mat4f getOrthogonalProjectionMatrix(
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
inline Mat4f projectionMatrix(
    float fov, float aspect,
    float near, float far)
{
  float _x = fov*0.01745329/2.0f; // degree to RAD
  float f = cos(_x)/sin(_x);
  return Mat4f(
      f/aspect, 0.0,                     0.0,  0.0,
      0.0,        f,                     0.0,  0.0,
      0.0,      0.0,   (far+near)/(near-far), -1.0,
      0.0,      0.0, 2.0*far*near/(near-far),  0.0
  );
}

inline Mat4f projectionMatrixInverse(
    const Mat4f &proj)
{
  return Mat4f(
      1.0/proj.x[0],         0.0,  0.0,                    0.0,
      0.0,         1.0/proj.x[5],  0.0,                    0.0,
      0.0,                   0.0,  0.0,         1.0/proj.x[14],
      0.0,                   0.0, -1.0,  proj.x[10]/proj.x[14]
  );
}

/**
  * equivalent to glFrustum.
  * @see: http://www.opengl.org/sdk/docs/man/xhtml/glFrustum.xml
  */
inline Mat4f getFrustumMatrix(
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
inline Mat4f getLookAtMatrix(
    const Vec3f &position,
    const Vec3f &direction,
    const Vec3f &up)
{
  Vec3f t = -position;
  Vec3f f = direction; normalize(f);
  Vec3f s = cross(f, up); //normalize(s);
  Vec3f u = cross(s, f);
  return Mat4f(
           s.x,      u.x,      -f.x, 0.0,
           s.y,      u.y,      -f.y, 0.0,
           s.z,      u.z,      -f.z, 0.0,
      dot(s,t), dot(u,t), dot(-f,t), 1.0
  );
}


inline Mat4f translationMatrix(const Vec3f &translation)
{
  return Mat4f(
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      translation.x, translation.y, translation.z, 1.0
  );
}

/**
 * creates a new rotation matrix that rotates x/y/z axes.
 */
inline Mat4f xyzRotationMatrix(float x, float y, float z)
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

inline Mat4f transformMatrix(
    const Vec3f &rot,
    const Vec3f &translation,
    const Vec3f &scale)
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

inline Mat4f lookAtCameraInverse(const Mat4f &src)
{
  return Mat4f(
      src.x[0], src.x[4], src.x[8], 0.0,
      src.x[1], src.x[5], src.x[9], 0.0,
      src.x[2], src.x[6], src.x[10], 0.0,
      -(src.x[12] * src.x[0]) - (src.x[13] * src.x[1]) - (src.x[14] * src.x[2]),
      -(src.x[12] * src.x[4]) - (src.x[13] * src.x[5]) - (src.x[14] * src.x[6]),
      -(src.x[12] * src.x[8]) - (src.x[13] * src.x[9]) - (src.x[14] * src.x[10]),
      1.0
  );
}

#endif /* _MATRIX_H_ */
