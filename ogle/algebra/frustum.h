/*
 * frustum.h
 *
 *  Created on: 03.02.2011
 *      Author: daniel
 */

#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include <vector>
using namespace std;

#include <ogle/algebra/vector.h>

/**
 * A Frustum is a portion of a pyramid
 * that lies between two parallel planes cutting it.
 */
class Frustum {
public:
  Frustum();
  ~Frustum();

  /**
   * (re)calculate the 8 points forming this Frustum.
   */
  void calculatePoints (
      const Vec3f &center,
      const Vec3f &viewDir,
      const Vec3f &up);

  /**
   * Split this frustum into n frustas by the Practical Split Scheme.
   * @see: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
   */
  vector<Frustum*> split (
      unsigned int nFrustas,
      float splitWeight) const;

  /**
   * Set the projection used for this frustum.
   */
  void setProjection(
      float fov,
      float aspect,
      float near,
      float far);

  const Vec3f* points() const;

  float far() const {
    return far_;
  }
  float near() const {
    return near_;
  }

private:
  float fov_;
  float aspect_;
  float far_;
  float near_;
  float nearPlaneHeight_;
  float nearPlaneWidth_;
  float farPlaneHeight_;
  float farPlaneWidth_;

  Vec3f points_[8];
};

#endif /* _FRUSTUM_H_ */
