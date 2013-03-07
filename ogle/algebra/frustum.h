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
 * \brief A portion of a pyramid that lies between two parallel planes cutting it.
 */
class Frustum {
public:
  /**
   * Update projection that defines near and far frustum plane.
   * @param fov field of view.
   * @param aspect the aspect ratio.
   * @param near distance to near plane.
   * @param far distance to far plane.
   */
  void setProjection(GLdouble fov, GLdouble aspect, GLdouble near, GLdouble far);
  /**
   * @return distance to far plane.
   */
  GLdouble far() const;
  /**
   * @return distance to near plane.
   */
  GLdouble near() const;

  /**
   * Computes the 8 points forming this Frustum.
   * @param origin the frustum origin.
   * @param dir the frustum direction.
   */
  void computePoints(const Vec3f &origin, const Vec3f &dir);
  /**
   * @return the 8 points forming this Frustum.
   */
  const Vec3f* points() const;

  /**
   * Split this frustum using the Practical Split Scheme.
   * @param n number of splits.
   * @param weight the split weight.
   * @return splitted frusta.
   * @see http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
   */
  vector<Frustum*> split(GLuint n, GLdouble weight) const;

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
} // namespace

#endif /* _FRUSTUM_H_ */
