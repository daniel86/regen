/*
Copyright (c) 2006-2012, assimp team
All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#ifndef QUATERNION_H_
#define QUATERNION_H_

#include <map>
using namespace std;

namespace ogle {

/**
 * http://en.wikipedia.org/wiki/Quaternion
 */
class Quaternion {
public:
  float w, x, y, z;

  Quaternion()
  : w(0.0f), x(0.0f), y(0.0f), z(0.0f) {}
  Quaternion(float _w, float _x, float _y, float _z)
  : w(_w), x(_x), y(_y), z(_z) {}

  ostream& operator<<(ostream& os)
  {
    return os << w << " " << x << " " << y << " " << z;
  }

  inline bool operator<(const Quaternion &b) const
  {
    return (x<b.x) || (y<b.y) || (z<b.z);
  }

  inline bool operator==(const Quaternion &b) const
  {
    return x == b.x && y == b.y && z == b.z && w == b.w;
  }

  inline bool operator!=(const Quaternion &b) const
  {
    return !(*this == b);
  }

  inline Quaternion operator* (const Quaternion& b) const
  {
    return Quaternion(
        w*b.w - x*b.x - y*b.y - z*b.z,
        w*b.x + x*b.w + y*b.z - z*b.y,
        w*b.y + y*b.w + z*b.x - x*b.z,
        w*b.z + z*b.w + x*b.y - y*b.x );
  }

  inline Mat4f calculateMatrix() const
  {
    Mat4f mat4x4 = Mat4f::identity();
    mat4x4.x[ 0] = -2.0f * (y * y + z * z) + 1.0f;
    mat4x4.x[ 1] =  2.0f * (x * y - z * w);
    mat4x4.x[ 2] =  2.0f * (x * z + y * w);
    mat4x4.x[ 4] =  2.0f * (x * y + z * w);
    mat4x4.x[ 5] = -2.0f * (x * x + z * z) + 1.0f;
    mat4x4.x[ 6] =  2.0f * (y * z - x * w);
    mat4x4.x[ 8] =  2.0f * (x * z - y * w);
    mat4x4.x[ 9] =  2.0f * (y * z + x * w);
    mat4x4.x[10] = -2.0f * (x * x + y * y) + 1.0f;
    return mat4x4;
  }

  /**
   * Performs a spherical interpolation between two quaternions
   * Implementation adopted from the gmtl project. All others I found on the net fail in some cases.
   * Congrats, gmtl!
   */
  inline void interpolate(
      const Quaternion& pStart,
      const Quaternion& pEnd,
      float pFactor)
  {
    // calc cosine theta
    float cosom =
        pStart.x * pEnd.x +
        pStart.y * pEnd.y +
        pStart.z * pEnd.z +
        pStart.w * pEnd.w;

    // adjust signs (if necessary)
    Quaternion end = pEnd;
    if(cosom < 0.0f)
    {
      cosom = -cosom;
      end.x = -end.x;   // Reverse all signs
      end.y = -end.y;
      end.z = -end.z;
      end.w = -end.w;
    }

    // Calculate coefficients
    float sclp, sclq;
    if( (1.0f - cosom) > 0.0001f) // 0.0001 -> some epsillon
    {
      // Standard case (slerp)
      float omega, sinom;
      omega = acos( cosom); // extract theta from dot product's cos theta
      sinom = sin( omega);
      sclp  = sin( (1.0f - pFactor) * omega) / sinom;
      sclq  = sin( pFactor * omega) / sinom;
    } else {
      // Very close, do linear interp (because it's faster)
      sclp = 1.0f - pFactor;
      sclq = pFactor;
    }

    x = sclp * pStart.x + sclq * end.x;
    y = sclp * pStart.y + sclq * end.y;
    z = sclp * pStart.z + sclq * end.z;
    w = sclp * pStart.w + sclq * end.w;
  }

  inline void normalize()
  {
    // compute the magnitude and divide through it
    const float mag = sqrt(x*x + y*y + z*z + w*w);
    if (mag)
    {
      const float invMag = 1.0f/mag;
      x *= invMag;
      y *= invMag;
      z *= invMag;
      w *= invMag;
    }
  }

  inline void conjugate()
  {
    x = -x; y = -y; z = -z;
  }

  inline Vec3f rotate (const Vec3f& v)
  {
    Quaternion q2( 0.0f,v.x,v.y,v.z );
    Quaternion q = *this;
    q.conjugate();
    q = q*q2*(*this);
    return (Vec3f) {q.x,q.y,q.z};
  }

  /**
   * Construction from euler angles.
   */
  inline void setEuler( float fPitch, float fYaw, float fRoll )
  {
    const float fSinPitch(sin(fPitch*0.5f));
    const float fCosPitch(cos(fPitch*0.5f));
    const float fSinYaw(sin(fYaw*0.5f));
    const float fCosYaw(cos(fYaw*0.5f));
    const float fSinRoll(sin(fRoll*0.5f));
    const float fCosRoll(cos(fRoll*0.5f));
    const float fCosPitchCosYaw(fCosPitch*fCosYaw);
    const float fSinPitchSinYaw(fSinPitch*fSinYaw);
    x = fSinRoll * fCosPitchCosYaw     - fCosRoll * fSinPitchSinYaw;
    y = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
    z = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
    w = fCosRoll * fCosPitchCosYaw     + fSinRoll * fSinPitchSinYaw;
  }

  /**
   * Construction from an axis-angle pair.
   */
  inline void setAxisAngle( const Vec3f &axis, float angle)
  {
    const float sin_a = sin( angle / 2 );
    const float cos_a = cos( angle / 2 );
    x = axis.x * sin_a;
    y = axis.y * sin_a;
    z = axis.z * sin_a;
    w = cos_a;
  }

  /**
   * Construction from am existing, normalized quaternion.
   */
  inline void setQuaternion( const Quaternion &normalized )
  {
    x = normalized.x;
    y = normalized.y;
    z = normalized.z;

    const float t = 1.0f - (x*x) - (y*y) - (z*z);
    if (t < 0.0f) w = 0.0f;
    else w = sqrt(t);
  }
};

}

#endif /* QUATERNION_H_ */
