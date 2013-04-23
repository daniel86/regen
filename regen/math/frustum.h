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

#include <regen/math/vector.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
/**
 * \brief A portion of a pyramid that lies between two parallel planes cutting it.
 */
class Frustum {
public:
  Frustum();
  /**
   * Update projection that defines near and far frustum plane.
   * @param fov field of view.
   * @param aspect the aspect ratio.
   * @param near distance to near plane.
   * @param far distance to far plane.
   */
  void setProjection(GLdouble fov, GLdouble aspect, GLdouble near, GLdouble far);
  /**
   * @return specifies the field of view angle, in degrees, in the y direction.
   */
  const ref_ptr<ShaderInput1f>& fov() const;
  /**
   * @return specifies the distance from the viewer to the near clipping plane (always positive).
   */
  const ref_ptr<ShaderInput1f>& near() const;
  /**
   * @return specifies the distance from the viewer to the far clipping plane (always positive).
   */
  const ref_ptr<ShaderInput1f>& far() const;
  /**
   * @return specifies the aspect ratio that determines the field of view in the x direction.
   */
  const ref_ptr<ShaderInput1f>& aspect() const;

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
  ref_ptr<ShaderInput1f> fov_;
  ref_ptr<ShaderInput1f> aspect_;
  ref_ptr<ShaderInput1f> far_;
  ref_ptr<ShaderInput1f> near_;
  GLdouble nearPlaneHeight_;
  GLdouble nearPlaneWidth_;
  GLdouble farPlaneHeight_;
  GLdouble farPlaneWidth_;

  Vec3f points_[8];
};
} // namespace

#endif /* _FRUSTUM_H_ */
