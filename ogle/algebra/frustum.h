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

namespace ogle {

/**
 * A Frustum is a portion of a pyramid
 * that lies between two parallel planes cutting it.
 */
class Frustum {
public:
  GLdouble far() const;
  GLdouble near() const;

  const Vec3f* points() const;
  /**
   * (re)calculate the 8 points forming this Frustum.
   */
  void calculatePoints(const Vec3f &center, const Vec3f &viewDir, const Vec3f &up);

  /**
   * Split this frustum into n frustas by the Practical Split Scheme.
   * @see: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
   */
  vector<Frustum*> split(GLuint nFrustas, GLdouble splitWeight) const;
  /**
   * Set the projection used for this frustum.
   */
  void setProjection(GLdouble fov, GLdouble aspect, GLdouble near, GLdouble far);

private:
  GLdouble fov_;
  GLdouble aspect_;
  GLdouble far_;
  GLdouble near_;
  GLdouble nearPlaneHeight_;
  GLdouble nearPlaneWidth_;
  GLdouble farPlaneHeight_;
  GLdouble farPlaneWidth_;

  Vec3f points_[8];
};

}

#endif /* _FRUSTUM_H_ */
